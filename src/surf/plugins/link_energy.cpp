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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_link_energy, surf, "Logging specific to the SURF LinkEnergy plugin");


namespace simgrid {
namespace energy {

class PowerRange {
  public:
  double idle;
  double busy;

  PowerRange(double idle, double busy) : idle(idle), busy(busy) {}
};

class LinkEnergy {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergy> EXTENSION_ID;

  explicit LinkEnergy(simgrid::s4u::Link *ptr);
  ~LinkEnergy();

  double getCurrentWattsValue(double link_load);
  double getConsumedEnergy();
  void update();

private:
  void initWattsRangeList();
  simgrid::s4u::Link *link = nullptr;
  simgrid::s4u::Link *up_link = nullptr;
  simgrid::s4u::Link *down_link = nullptr;
  std::vector<PowerRange> power_range_watts_list;   /*< List of (idle_power,busy_power) pairs*/
public:
  double watts_off = 0.0; /*< Consumption when the link is turned off (shutdown) */
  double total_energy = 0.0; /*< Total energy consumed by the host */
  double last_updated;       /*< Timestamp of the last energy update event*/
};

simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergy> LinkEnergy::EXTENSION_ID;

/* Computes the consumption so far.  Called lazily on need. */
void LinkEnergy::update()
{
  double start_time = this->last_updated;
  double finish_time = surf_get_clock();
  double link_load;
  double uplink_load;
  double downlink_load;

  double previous_energy = this->total_energy;

  double instantaneous_consumption;

  const char* lnk_down = strstr(link->name(),"_DOWN");
  const char* lnk_up = strstr(link->name(),"_UP");

  if (this->link->isOff()){

    instantaneous_consumption = this->watts_off;

  }else if(lnk_down){

    this->up_link->extension<LinkEnergy>()->update();
    return;

  }else if(lnk_up){

     uplink_load = lmm_constraint_get_usage(this->up_link->pimpl_->constraint());
     downlink_load =  lmm_constraint_get_usage(this->down_link->pimpl_->constraint());

    link_load = downlink_load + uplink_load;
    instantaneous_consumption = this->getCurrentWattsValue(link_load);

  }else{

    link_load = lmm_constraint_get_usage(this->link->pimpl_->constraint());
    instantaneous_consumption = this->getCurrentWattsValue(link_load);

  }

  double energy_this_step = instantaneous_consumption*(finish_time-start_time);

  this->total_energy = previous_energy + energy_this_step;
  this->last_updated = finish_time;

  XBT_DEBUG(
      "[update_energy of %s] period=[%.2f-%.2f]; current power peak=%.0E flop/s; consumption change: %.2f J -> %.2f J",
      this->link->name(), start_time, finish_time, this->link->bandwidth(), previous_energy, energy_this_step);
}

LinkEnergy::LinkEnergy(simgrid::s4u::Link *ptr) : link(ptr), last_updated(surf_get_clock())
{
  initWattsRangeList();

  char *lnk_name;
  lnk_name = const_cast<char*>(this->link->name());
  char *lnk_down = strstr(lnk_name,"_DOWN");
  char *lnk_up = strstr(lnk_name,"_UP");

  if (lnk_down){

    this->down_link = this->link->byName(lnk_name);
    strncpy(lnk_down,"_UP",4);
    this->up_link = this->link->byName(lnk_name);
    
   }else if(lnk_up){

    this->up_link = this->link->byName(lnk_name);
    strncpy(lnk_up,"_DOWN",6);
    this->down_link = this->link->byName(lnk_name);

  } else {
	  this->up_link = this->link;
  }
 
    
  const char* off_power_str = this->link->property("watt_off");

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
  xbt_assert(!power_range_watts_list.empty(), "No power range properties specified for link %s", this->link->name());

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

    current_power = idle_power + (link_load / link->bandwidth()) * power_slope;
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
  if (this->last_updated < surf_get_clock()) // We need to simcall this as it modifies the environment
    simgrid::simix::kernelImmediate(std::bind(&LinkEnergy::update, this));

  return this->total_energy;
}

void LinkEnergy::initWattsRangeList()
{
  const char* power_values_str = this->link->property("watts");
  if (power_values_str == nullptr)
    return;

  xbt_dynar_t power_values = xbt_str_split(power_values_str, ":");

    xbt_assert(xbt_dynar_length(power_values) == 2,
               "Power properties incorrectly defined - could not retrieve idle and full power values for link %s",
               this->link->name());

    /* idle_power corresponds to the idle power (link load = 0) */
    /* busy_power is the power consumed at 100% link load       */
    char* msg_idle = bprintf("Invalid idle value for  on link %s: %%s", this->link->name());
    char* msg_busy  = bprintf("Invalid max value for  on link %s: %%s", this->link->name());
    PowerRange range(
      xbt_str_parse_double(xbt_dynar_get_as(power_values, 0, char*), msg_idle),
      xbt_str_parse_double(xbt_dynar_get_as(power_values, 1, char*), msg_busy)
    );
    this->power_range_watts_list.push_back(range);
    xbt_free(msg_idle);
    xbt_free(msg_busy);

    xbt_dynar_free(&power_values);
  
}

}
}

using simgrid::energy::LinkEnergy;

/* **************************** events  callback *************************** */
static void onCreation(simgrid::s4u::Link& link) {
  link.extension_set(new LinkEnergy(&link));
}

static void onActionStateChange(simgrid::surf::NetworkAction* action, simgrid::surf::Action::State previous)
{
  for (simgrid::surf::LinkImpl* link : action->links()) {

    if (link == nullptr)
      continue;

    // Get the link_energy extension for the relevant link
    simgrid::s4u::Link* lnk = dynamic_cast<simgrid::s4u::Link*>(link);
    LinkEnergy* link_energy = lnk->extension<LinkEnergy>();

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
  XBT_INFO("Total energy of link %s: %f Joules", link.name(), link_energy->getConsumedEnergy());
}

/* **************************** Public interface *************************** */
SG_BEGIN_DECL()
/** \ingroup SURF_plugin_energy
 * \brief Enable energy plugin
 * \details Enable energy plugin to get joules consumption of each cpu. You should call this function before #MSG_init().
 */
void sg_link_energy_plugin_init()
{
  if (LinkEnergy::EXTENSION_ID.valid())
    return;

  LinkEnergy::EXTENSION_ID = simgrid::s4u::Link::extension_create<LinkEnergy>();

  simgrid::s4u::Link::onCreation.connect(&onCreation);
  simgrid::s4u::Link::onStateChange.connect(&onLinkStateChange);
  simgrid::s4u::Link::onDestruction.connect(&onLinkDestruction);
  simgrid::s4u::Link::onCommunicationStateChange.connect(&onActionStateChange);//simgrid::surf::NetworkAction::onStateChange.connect(&onActionStateChange);
  
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
SG_END_DECL()
