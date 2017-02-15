/* energy.hpp: internal interface to the energy plugin                      */

/* Copyright (c) 2014-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include <utility>

#include "src/surf/HostImpl.hpp"

#ifndef ENERGY_CALLBACK_HPP_
#define ENERGY_CALLBACK_HPP_

namespace simgrid {
namespace energy {

class XBT_PRIVATE HostEnergy;

class PowerRange {
  public: 
  double idle;
  double busy;

  PowerRange(double idle, double busy) : idle(idle), buys(busy) {
  }
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
  std::vector<PowerRange> power_range_watts_list;   /*< List of (idle_power,busy_power) pairs*/
public:
  double watts_off = 0.0; /*< Consumption when the link is turned off (shutdown) */
  double total_energy = 0.0; /*< Total energy consumed by the host */
  double last_updated;       /*< Timestamp of the last energy update event*/
};

}
}

#endif /* ENERGY_CALLBACK_HPP_ */
