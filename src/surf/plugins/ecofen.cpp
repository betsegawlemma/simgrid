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
	void init_ecofen(double netDeviceIdleValue);
	void init_ns3();

private:

	simgrid::s4u::Link *link { };
	simgrid::s4u::Link *up_link { };
	simgrid::s4u::Link *down_link { };

	std::vector<LinkPowerRange> power_range_watts_list { };

	double last_updated { 0.0 }; /*< Timestamp of the last energy update event*/

};

simgrid::xbt::Extension<simgrid::s4u::Link, Ecofen> Ecofen::EXTENSION_ID;

Ecofen::Ecofen(simgrid::s4u::Link *ptr) :
		link(ptr), last_updated(surf_get_clock()) {
	/*
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
	 initWattsRangeList();
	 xbt_free(lnk_name);
	 xbt_free(lnk_down);
	 xbt_free(lnk_up);
	 */
}

Ecofen::~Ecofen() = default;

/* Computes the consumption so far.  Called lazily on need. */

void Ecofen::initWattsRangeList() {

	if (!power_range_watts_list.empty())
		return;

	xbt_assert(power_range_watts_list.empty(),
			"Power properties incorrectly defined - "
					"could not retrieve idle and busy power values for link %s",
			this->up_link->name());

	const char* all_power_values_str = this->up_link->property("watt_range");
	XBT_DEBUG("watt range %s", all_power_values_str);
	XBT_DEBUG("ECOFEN initWattsRangeList is called %s", this->up_link->name());
	if (all_power_values_str == nullptr)
		return;
	XBT_DEBUG("ECOFEN 3 initWattsRangeList is called");
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
		XBT_DEBUG("Ecofen: initWattsRangeList: idle %f, busy %f", idleVal,
				busyVal);
		xbt_free(idle);
		xbt_free(busy);

		this->power_range_watts_list.push_back(
				LinkPowerRange(idleVal, busyVal));
		XBT_DEBUG("ECOFEN 3 initWattsRangeList is called");

	}

}

}
}

using simgrid::plugin::Ecofen;

/* **************************** events  callback *************************** */

/* **************************** Public interface *************************** */
SG_BEGIN_DECL()
/** \ingroup SURF_plugin_energy
 * \brief Enable energy plugin
 * \details Enable energy plugin to get joules consumption of each cpu. You should call this function before #MSG_init().
 */

void init_ns3() {
	ns3_initialize("default");
	ns3_simulator(10);
}

void init_ecofen(double netDeviceIdleValue) {

	double byteEnergy = 3.423; //byte energy in nJoule
	ecofen_init_basic_model(0.0, byteEnergy); // node idle power and byte energy value in nJoule
	ecofen_init_log(1.0, 15.0); //log time interval and log end time
	ecofen_init_linear_model(netDeviceIdleValue); //idle power for the net device
}

void ns3_link_energy_plugin_init() {
	init_ecofen(10.3581);
}

SG_END_DECL()

