/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../cpu_interface.hpp"
#include <map>

#ifndef CALLBACK_HPP_
#define CALLBACK_HPP_

class CpuEnergy;

extern std::map<Cpu*, CpuEnergy*> *surf_energy;

class CpuEnergy {
public:
  CpuEnergy(Cpu *ptr);
  ~CpuEnergy();

  double getCurrentWattsValue(double cpu_load);
  double getConsumedEnergy();
  double getWattMinAt(int pstate);
  double getWattMaxAt(int pstate);

  xbt_dynar_t getWattsRangeList();
  xbt_dynar_t power_range_watts_list;   /*< List of (min_power,max_power) pairs corresponding to each cpu pstate */
  double watts_off;                      /*< Consumption when the machine is turned off (shutdown) */
  double total_energy;					/*< Total energy consumed by the host */
  double last_updated;					/*< Timestamp of the last energy update event*/
  Cpu *cpu;
};

#endif /* CALLBACK_HPP_ */
