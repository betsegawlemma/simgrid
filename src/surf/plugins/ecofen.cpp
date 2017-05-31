/* Copyright (c) 2010, 2012-2016. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/link_energy.h"
#include "simgrid/simix.hpp"
#include "src/surf/network_ns3.hpp"
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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ecofen, surf,
		"Logging specific to the SURF NS3LinkEnergy plugin");

namespace simgrid {
namespace plugin {

class LinkPowerRange {
public:
	double idle;
	double busy;

	LinkPowerRange(double idle, double busy) :
			idle(idle), busy(busy) {
	}
};

class Ecofen {
public:
	static simgrid::xbt::Extension<simgrid::s4u::Link, Ecofen> EXTENSION_ID;

	explicit Ecofen(simgrid::s4u::Link *ptr);
	~Ecofen();

	double getLastUpdated();
	double getCurrntTime();
	void initWattsRangeList();
	double getLinkUsage();
	double getLinkIdlePower();
	void update();

private:

	void updateLinkUsage();

	simgrid::s4u::Link *link { };
	simgrid::s4u::Link *up_link { };
	simgrid::s4u::Link *down_link { };

	std::vector<LinkPowerRange> power_range_watts_list { };

	double last_updated { 0.0 }; /*< Timestamp of the last energy update event*/
	double link_usage { 0.0 };

};

simgrid::xbt::Extension<simgrid::s4u::Link, Ecofen> Ecofen::EXTENSION_ID;

Ecofen::Ecofen(simgrid::s4u::Link *ptr) :
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

Ecofen::~Ecofen() = default;

/* Computes the consumption so far.  Called lazily on need. */
void Ecofen::update() {

	updateLinkUsage();
	return;

}

void Ecofen::updateLinkUsage() {

	double uplink_usage;
	double downlink_usage;

	const char* lnk_down = strstr(link->name(), "_DOWN");
	const char* lnk_up = strstr(link->name(), "_UP");

	if (lnk_down) {

		this->up_link->extension<Ecofen>()->updateLinkUsage();

	} else if (lnk_up) {

		uplink_usage = lmm_constraint_get_usage(
				this->up_link->pimpl_->constraint());

		downlink_usage = lmm_constraint_get_usage(
				this->down_link->pimpl_->constraint());

		this->link_usage = downlink_usage + uplink_usage;

	} else {

		this->link_usage = lmm_constraint_get_usage(
				this->up_link->pimpl_->constraint());

	}
}

void Ecofen::initWattsRangeList() {

	if (!power_range_watts_list.empty())
		return;

	xbt_assert(power_range_watts_list.empty(),
			"Power properties incorrectly defined - "
					"could not retrieve idle and busy power values for link %s",
			this->up_link->name());

	const char* all_power_values_str = this->up_link->property("watts");

	if (all_power_values_str == nullptr)
		return;

	std::vector < std::string > all_power_values;
	boost::split(all_power_values, all_power_values_str, boost::is_any_of(","));

	for (auto current_power_values_str : all_power_values) {
		/* retrieve the power values associated */
		std::vector < std::string > current_power_values;
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

		this->power_range_watts_list.push_back(
				LinkPowerRange(idleVal, busyVal));
	}

}

double Ecofen::getLastUpdated() {
	return this->last_updated;
}

double Ecofen::getCurrntTime() {
	return surf_get_clock();
}

double Ecofen::getLinkUsage() {
	return this->link_usage;
}
double Ecofen::getLinkIdlePower(){
	return this->power_range_watts_list[0].idle;
}
}
}

using simgrid::plugin::Ecofen;

void init_ecofen(simgrid::s4u::Link &link){
	Ecofen *link_energy = link.extension<Ecofen>();
	double byteEnergy = 3.423; //byte energy in nJoule
	double linkIdlePower = link_energy->getLinkIdlePower(); //idle power for a link
	XBT_INFO("idle power %f",linkIdlePower);
	ecofen_init_basic_model(0.0,byteEnergy); // node idle power and byte enery value in nJoule
	ecofen_init_linear_model(linkIdlePower); //idle power for the net device
	ecofen_init_log(1.0, 3.0); //log time interval and log end time
}

/* **************************** events  callback *************************** */
static void onCreation(simgrid::s4u::Link& link) {
	XBT_DEBUG("onCreation is called for link: %s", link.name());
	link.extension_set(new Ecofen(&link));
	//init_ecofen(link);
}

static void onCommunicate(simgrid::surf::NetworkAction* action,
		simgrid::s4u::Host* src, simgrid::s4u::Host* dst) {
	XBT_INFO("onCommunicate is called src %s -> dst %s", src->cname(),dst->cname());

	/*for (simgrid::surf::LinkImpl* link : action->links()) {

		if (link == nullptr)
			continue;

		// Get the link_energy extension for the relevant link
		Ecofen* link_energy = link->piface_.extension<Ecofen>();
		link_energy->initWattsRangeList();
		link_energy->update();
	}*/
}

static void onLinkStateChange(simgrid::s4u::Link &link) {
	XBT_DEBUG("onLinkStateChange is called for link: %s", link.name());

	Ecofen *link_energy = link.extension<Ecofen>();
	link_energy->update();
}

static void onLinkDestruction(simgrid::s4u::Link& link) {
	XBT_DEBUG("onLinkDestruction is called for link: %s", link.name());
}

static void onSimulationEnd() {
	XBT_DEBUG("onSimulationEnd is called ...");
}
/* **************************** Public interface *************************** */
SG_BEGIN_DECL()
/** \ingroup SURF_plugin_energy
 * \brief Enable energy plugin
 * \details Enable energy plugin to get joules consumption of each cpu. You should call this function before #MSG_init().
 */
void ns3_link_energy_plugin_init() {
	if (Ecofen::EXTENSION_ID.valid())
	return;

	Ecofen::EXTENSION_ID =
	simgrid::s4u::Link::extension_create<Ecofen>();

	simgrid::s4u::Link::onCreation.connect(&onCreation);
	simgrid::s4u::Link::onDestruction.connect(&onLinkDestruction);
	simgrid::s4u::Link::onCommunicate.connect(&onCommunicate);
	simgrid::s4u::onSimulationEnd.connect(&onSimulationEnd);

}

/** @brief Returns the total energy consumed by the link so far (in Joules)
 *
 *  See also @ref SURF_plugin_energy.
 */
double ns3_link_get_consumed_energy(sg_link_t link) {
	xbt_assert(Ecofen::EXTENSION_ID.valid(),
			"The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");

	return 0.0;
}

double ns3_link_get_consumed_power(sg_link_t link) {
	xbt_assert(Ecofen::EXTENSION_ID.valid(),
			"The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
	return 0.0;
}

double ns3_link_get_usage(sg_link_t link) {
	xbt_assert(Ecofen::EXTENSION_ID.valid(),
			"The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
	return 0.0;
}

void ns3_on_simulation_end() {
	onSimulationEnd();
}
SG_END_DECL()

