/* Copyright (c) 2010, 2012-2016. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/energy.h"
#include "simgrid/simix.hpp"
#include "src/surf/network_interface.hpp"
#include "simgrid/s4u/Engine.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>
#include <utility>
#include <vector>
#include <map>

/** @addtogroup SURF_plugin_energy


 This is the energy plugin, enabling to account for the dissipated energy in the simulated platform.

 The energy consumption of a link depends directly on its current traffic load. Specify that consumption in your platform file as follows:

 \verbatim
 <link id="SWITCH1" bandwidth="125000000" latency="5E-5" sharing_policy="SHARED" >
 <prop id="watts" value="100.0:200.0" />
 <prop id="watt_off" value="10" />
 </link>
 \endverbatim

 The first property means that when your link is switched on, but without anything to do, it will dissipate 100 Watts.
 If it's fully loaded, it will dissipate 200 Watts. If its load is at 50%, then it will dissipate 150 Watts.
 The second property means that when your host is turned off, it will dissipate only 10 Watts (please note that these
 values are arbitrary).

 To simulate the energy-related elements, first call the simgrid#energy#sg_link_energy_plugin_init() before your #MSG_init(),
 and then use the following function to retrieve the consumption of a given link: MSG_link_get_consumed_energy().
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_link_energy, surf,
		"Logging specific to the SURF LinkEnergy plugin");

namespace simgrid {
namespace energy {

class PowerRange {
public:
	double watt_idle;
	double watt_busy;

	PowerRange(double watt_idle, double watt_busy) :
			watt_idle(watt_idle), watt_busy(watt_busy) {
	}
};

class LinkEnergy {
public:
	static simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergy> EXTENSION_ID;

	explicit LinkEnergy(simgrid::s4u::Link *ptr);
	~LinkEnergy();

	double getCurrentWattsValue();
	double getALinkTotalPower(sg_link_t link);
	void deletePowerValue(sg_link_t link);
	double getLastUpdated();
	void update();

private:

	void initWattsRangeList();
	void updateLinkLoad();
	double computeDynamicPower();
	void computeTotalPower();

	simgrid::s4u::Link *link { };
	simgrid::s4u::Link *up_link { };
	simgrid::s4u::Link *down_link { };

	std::vector<PowerRange> power_range_watts_list { };
	std::map<const char*, double> a_link_total_power { };

	double last_updated { 0.0 }; /*< Timestamp of the last energy update event*/
	double link_load { 0.0 };

};

simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergy> LinkEnergy::EXTENSION_ID;

LinkEnergy::LinkEnergy(simgrid::s4u::Link *ptr) :
		link(ptr), last_updated(surf_get_clock()) {

	char *lnk_name = xbt_strdup(this->link->name());
	char *lnk_down = strstr(lnk_name, "_DOWN");
	char *lnk_up = strstr(lnk_name, "_UP");

	if (lnk_down) {

		this->down_link = this->link->byName(lnk_name);
		strncpy(lnk_down, "_UP", 4);
		this->up_link = this->link->byName(lnk_name);

	} else if (lnk_up) {

		this->up_link = this->link->byName(lnk_name);
		strncpy(lnk_up, "_DOWN", 6);
		this->down_link = this->link->byName(lnk_name);

	} else {
		this->up_link = this->link;
	}
	xbt_free(lnk_name);
	xbt_free(lnk_down);
	xbt_free(lnk_up);
}

LinkEnergy::~LinkEnergy() = default;

/* Computes the consumption so far.  Called lazily on need. */
void LinkEnergy::update() {

	updateLinkLoad();

	double start_time = this->last_updated;
	double finish_time = surf_get_clock();

	computeTotalPower();
	double alink_total_power = getALinkTotalPower(this->up_link);

	this->last_updated = finish_time;

	XBT_INFO(
			"[update: Link: %s] period=[%.2f-%.2f], bandwidth %f, load %f, a_link_total_power %f",
			this->link->name(), start_time, finish_time,
			this->link->bandwidth(), this->link_load, alink_total_power);
}

void LinkEnergy::updateLinkLoad() {

	double uplink_load;
	double downlink_load;

	const char* lnk_down = strstr(link->name(), "_DOWN");
	const char* lnk_up = strstr(link->name(), "_UP");

	if (lnk_down) {

		this->up_link->extension<LinkEnergy>()->updateLinkLoad();

	} else if (lnk_up) {

		uplink_load = lmm_constraint_get_usage(
				this->up_link->pimpl_->constraint());

		downlink_load = lmm_constraint_get_usage(
				this->down_link->pimpl_->constraint());

		this->link_load = downlink_load + uplink_load;

	} else {

		this->link_load = lmm_constraint_get_usage(
				this->up_link->pimpl_->constraint());

	}
}

void LinkEnergy::initWattsRangeList() {

	if (!power_range_watts_list.empty())
		return;

	xbt_assert(power_range_watts_list.empty(),
			"Power properties incorrectly defined - "
					"could not retrieve idle and busy power values for link %s",
			this->up_link->name());

	const char* all_power_values_str = this->up_link->property("watts");

	if (all_power_values_str == nullptr)
		return;

	std::vector<std::string> all_power_values;
	boost::split(all_power_values, all_power_values_str, boost::is_any_of(","));

	for (auto current_power_values_str : all_power_values) {
		/* retrieve the power values associated */
		std::vector<std::string> current_power_values;
		boost::split(current_power_values, current_power_values_str,
				boost::is_any_of(":"));
		xbt_assert(current_power_values.size() == 2,
				"Power properties incorrectly defined - "
						"could not retrieve idle and busy power values for link %s",
				this->up_link->name());

		/* min_power corresponds to the idle power (link load = 0) */
		/* max_power is the power consumed at 100% link load       */
		char* idle = bprintf("Invalid idle power value for link%s",
				this->up_link->name());
		char* busy = bprintf("Invalid busy power value for %s",
				this->up_link->name());

		double idleVal = xbt_str_parse_double(
				(current_power_values.at(0)).c_str(), idle);
		double busyVal = xbt_str_parse_double(
				(current_power_values.at(1)).c_str(), busy);
		xbt_free(idle);
		xbt_free(busy);

		this->power_range_watts_list.push_back(PowerRange(idleVal, busyVal));
		// set the idle value for each link
		this->a_link_total_power[this->up_link->name()] = idleVal;
	}

}

double LinkEnergy::computeDynamicPower() {

	double dynamic_power = 0.0;

	xbt_assert(!power_range_watts_list.empty(),
			"No power range properties specified for link %s",
			this->up_link->name());

	auto range = power_range_watts_list[0];
	double watt_busy = range.watt_busy;
	double watt_idle = range.watt_idle;
	double power_slope = watt_busy - watt_idle;

	if (this->link_load > 0) { /* Something is going on, the link is not idle */

		dynamic_power = (this->link_load / this->up_link->bandwidth())
				* power_slope;

	} else { /* Our machine is idle, take the dedicated value! */

		dynamic_power = 0.0;
	}

	XBT_INFO(
			"[computeDynamicPower:%s] idle_power=%f, busy_power=%f, slope=%f, dynamic_power=%f, link_load=%f",
			this->up_link->name(), watt_idle, watt_busy, power_slope,
			dynamic_power, this->link_load);

	return dynamic_power;
}

void LinkEnergy::computeTotalPower() {

	initWattsRangeList();

	if (!strcmp(this->up_link->name(), "__loopback__"))
		return;

	this->a_link_total_power[this->up_link->name()] += computeDynamicPower();

}

double LinkEnergy::getALinkTotalPower(sg_link_t link) {
	return this->a_link_total_power[link->name()];
}

void LinkEnergy::deletePowerValue(sg_link_t link) {
	a_link_total_power.erase(link->name());
}

double LinkEnergy::getLastUpdated() {
	return this->last_updated;
}

}
}

using simgrid::energy::LinkEnergy;

/* **************************** events  callback *************************** */
static void onCreation(simgrid::s4u::Link& link) {
	XBT_INFO("onCreation is called");
	link.extension_set(new LinkEnergy(&link));
}

static void onActionStateChange(simgrid::surf::NetworkAction* action) {
	XBT_INFO("onActionStateChange is called");
	for (simgrid::surf::LinkImpl* link : action->links()) {

		if (link == nullptr)
			continue;

		// Get the link_energy extension for the relevant link
		LinkEnergy* link_energy = link->piface_.extension<LinkEnergy>();
		link_energy->update();
	}
}
static void onLinkStateChange(simgrid::s4u::Link &link) {
	XBT_INFO("onLinkStateChange is called");

	LinkEnergy *link_energy = link.extension<LinkEnergy>();
	link_energy->update();
}

static void onLinkDestruction(simgrid::s4u::Link& link) {
	XBT_INFO("onLinkDestruction is called");

	LinkEnergy *link_energy = link.extension<LinkEnergy>();
	link_energy->update();
	link_energy->deletePowerValue(&link);
	XBT_INFO("onLinkDestruction: Total power of link: %s is: %f Watt", link.name(),link_energy->getALinkTotalPower(&link));
}
static void onCommunicate(simgrid::surf::NetworkAction* action,
		simgrid::s4u::Host* src, simgrid::s4u::Host* dst) {
	XBT_INFO("onCommunicate is called");
	for (simgrid::surf::LinkImpl* link : action->links()) {

		if (link == nullptr)
			continue;

		// Get the link_energy extension for the relevant link
		LinkEnergy* link_energy = link->piface_.extension<LinkEnergy>();

		link_energy->update();
	}
}

static void onSimulationEnd() {
	simgrid::s4u::Link* link = nullptr;
	sg_link_t* link_list = link->listLink();
	int link_count = link->linkCount();
	double total_power = 0.0; // Total power consumption (whole platform)
	double used_links_power = 0.0; // Power consumed by links who participated in communication task
	for (int i = 0; i < link_count; i++) {
		if (link_list[i] != nullptr) {

			bool link_was_used =
					link_list[i]->extension<LinkEnergy>()->getLastUpdated()
							!= 0.0;
			double a_link_total_power =
					link_list[i]->extension<LinkEnergy>()->getALinkTotalPower(
							link_list[i]);
			total_power += a_link_total_power;
			if (link_was_used)
				used_links_power += a_link_total_power;
		}
	}
	XBT_INFO(
			"onSimulationEnd: Total power: %f watts (used links power: %f watts; unused/idle links power: %f)",
			total_power, used_links_power, total_power - used_links_power);
	xbt_free(link_list);

}
/* **************************** Public interface *************************** */
SG_BEGIN_DECL()
/** \ingroup SURF_plugin_energy
 * \brief Enable energy plugin
 * \details Enable energy plugin to get joules consumption of each cpu. You should call this function before #MSG_init().
 */
void sg_link_energy_plugin_init() {
	if (LinkEnergy::EXTENSION_ID.valid())
		return;

	LinkEnergy::EXTENSION_ID =
			simgrid::s4u::Link::extension_create<LinkEnergy>();

	simgrid::s4u::Link::onCreation.connect(&onCreation);
	simgrid::s4u::Link::onStateChange.connect(&onLinkStateChange);
	simgrid::s4u::Link::onDestruction.connect(&onLinkDestruction);
	simgrid::s4u::Link::onCommunicationStateChange.connect(
			&onActionStateChange);
	simgrid::s4u::Link::onCommunicate.connect(&onCommunicate);
	simgrid::s4u::onSimulationEnd.connect(&onSimulationEnd);

}

/** @brief Returns the total energy consumed by the link so far (in Joules)
 *
 *  See also @ref SURF_plugin_energy.
 */
double sg_link_get_consumed_energy(sg_link_t link) {
	xbt_assert(LinkEnergy::EXTENSION_ID.valid(),
			"The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
	LinkEnergy *link_energy = link->extension<LinkEnergy>();
	return link_energy->getALinkTotalPower(link);
}
SG_END_DECL()
