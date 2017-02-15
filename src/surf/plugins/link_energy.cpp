/* Copyright (c) 2010, 2012-2016. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/link_energy.h"
#include "simgrid/simix.hpp"
#include "src/surf/plugins/link_energy.hpp"
#include <utility>

/** @addtogroup SURF_plugin_energy


This is the energy plugin, enabling to account for the dissipated energy in the simulated platform.

The energy consumption of a link depends directly of its current traffic load. Specify that consumption in your platform file as follows:

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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_energy, surf, "Logging specific to the SURF energy plugin");

using simgrid::energy::LinkEnergy;

namespace simgrid {
namespace energy {

simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergy> LinkEnergy::EXTENSION_ID;

/* Computes the consumption so far.  Called lazily on need. */
void LinkEnergy::update()
{
  double start_time = this->last_updated;
  double finish_time = surf_get_clock();
  double link_load;
  
  link_load = lmm_constraint_get_usage(host->pimpl_cpu->constraint()) / host->pimpl_cpu->getPstateSpeedCurrent();

  double previous_energy = this->total_energy;

  double instantaneous_consumption;
  if (link->isOff())
    instantaneous_consumption = this->watts_off;
  else
    instantaneous_consumption = this->getCurrentWattsValue(link_load);

  double energy_this_step = instantaneous_consumption*(finish_time-start_time);

  this->total_energy = previous_energy + energy_this_step;
  this->last_updated = finish_time;

  XBT_DEBUG(
      "[update_energy of %s] period=[%.2f-%.2f]; current power peak=%.0E flop/s; consumption change: %.2f J -> %.2f J",
      link->name(), start_time, finish_time, host->pimpl_cpu->speed_.peak, previous_energy, energy_this_step);
}

LinkEnergy::LinkEnergy(simgrid::s4u::Link *ptr) : link(ptr), last_updated(surf_get_clock())
{
  initWattsRangeList();

  const char* off_power_str = link->property("watt_off");
  if (off_power_str != nullptr) {
    char* msg       = bprintf("Invalid value for property watt_off of link %s: %%s", link->name());
    this->watts_off = xbt_str_parse_double(off_power_str, msg);
    xbt_free(msg);
  }
  /* watts_off is 0 by default */
}

LinkEnergy::~LinkEnergy()=default;



/** @brief Computes the power consumed by link according to its current load */
double LinkEnergy::getCurrentWattsValue(double link_load)
{
  xbt_assert(!power_range_watts_list.empty(), "No power range properties specified for link %s", link->name());

  /* idle_power corresponds to the idle power (link load = 0) */
  /* full_power is the power consumed at 100% link load       */
  auto range           = power_range_watts_list.at(0);
  double current_power = 0;
  double idle_power     = 0;
  double busy_power     = 0;
  double power_slope   = 0;

  if (link_load > 0) { /* Something is going on, the link is not idle */
    double idle_power = range.idle;
    double busy_power = range.busy;

    double power_slope;
      power_slope = (busy_power - idle_power);

    current_power = idle_power + (link_load / link.bandwidth()) * power_slope;
  }
  else { /* Our machine is idle, take the dedicated value! */
    current_power = range.idle;
  }

  XBT_DEBUG("[get_current_watts] idle_power=%f, busy_power=%f, slope=%f", idle_power, busy_power, power_slope);
  XBT_DEBUG("[get_current_watts] Current power (watts) = %f, load = %f", current_power, link_load);

  return current_power;
}

double LinkEnergy::getConsumedEnergy()
{
  if (last_updated < surf_get_clock()) // We need to simcall this as it modifies the environment
    simgrid::simix::kernelImmediate(std::bind(&LinkEnergy::update, this));

  return total_energy;
}

void LinkEnergy::initWattsRangeList()
{
  const char* power_values_str = link->property("watts");
  if (power_values_str == nullptr)
    return;

  xbt_dynar_t power_values = xbt_str_split(power_values_str, ":");

    xbt_assert(xbt_dynar_length(power_values) == 2,
               "Power properties incorrectly defined - could not retrieve idle and full power values for link %s",
               link->name());

    /* idle_power corresponds to the idle power (link load = 0) */
    /* busy_power is the power consumed at 100% link load       */
    char* msg_idle = bprintf("Invalid idle value for  on link %s: %%s", link->name());
    char* msg_busy  = bprintf("Invalid max value for  on link %s: %%s", link->name());
    PowerRange range(
      xbt_str_parse_double(xbt_dynar_get_as(power_values, 0, char*), msg_idle),
      xbt_str_parse_double(xbt_dynar_get_as(power_values, 1, char*), msg_busy)
    );
    power_range_watts_list.push_back(range);
    xbt_free(msg_idle);
    xbt_free(msg_busy);

    xbt_dynar_free(&power_values);
  
}

}
}

/* **************************** events  callback *************************** */
static void onCreation(simgrid::s4u::Link& link) {
  link.extension_set(new LinkEnergy(&link));
}

static void onActionStateChange(simgrid::surf::NetworkAction *action, simgrid::surf::Action::State previous) {
  for (simgrid::surf::Cpu* cpu : action->cpus()) {
    simgrid::s4u::link* link = cpu->getHost();
    if (host == nullptr)
      continue;

    // Get the link_energy extension for the relevant link
    LinkEnergy* link_energy = link->extension<LinkEnergy>();

    if (link_energy->last_updated < surf_get_clock())
      link_energy->update();
  }
}

static void onLinkStateChange(simgrid::s4u::Link &link) {

  LinkEnergy *link_energy = link.extension<LinkEnergy>();

  if(link_energy->last_updated < surf_get_clock())
    link_energy->update();
}

static void onLinkDestruction(simgrid::s4u::Link& link) {

  LinkEnergy *link_energy = link.extension<LinkEnergy>();
  link_energy->update();
  XBT_INFO("Total energy of link %s: %f Joules", link.cname(), link_energy->getConsumedEnergy());
}

/* **************************** Public interface *************************** */
/** \ingroup SURF_plugin_energy
 * \brief Enable energy plugin
 * \details Enable energy plugin to get joules consumption of each cpu. You should call this function before #MSG_init().
 */
void sg_link_energy_plugin_init()
{
  if (HostEnergy::EXTENSION_ID.valid())
    return;

  LinkEnergy::EXTENSION_ID = simgrid::s4u::Link::extension_create<LinkEnergy>();

  simgrid::s4u::Link::onCreation.connect(&onCreation);
  simgrid::s4u::Link::onStateChange.connect(&onLinkStateChange);
  simgrid::s4u::Link::onDestruction.connect(&onLinkDestruction);
  simgrid::surf::NetworkAction::onStateChange.connect(&onActionStateChange);
}

/** @brief Returns the total energy consumed by the link so far (in Joules)
 *
 *  See also @ref SURF_plugin_energy.
 */
double sg_link_get_consumed_energy(sg_link_t link) {
  xbt_assert(LinkEnergy::EXTENSION_ID.valid(),
    "The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
  return link->extension<LinkEnergy>()->getConsumedEnergy();
}

