/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_ENERGY_H_
#define SIMGRID_PLUGINS_ENERGY_H_

#include <xbt/base.h>
#include <simgrid/forward.h>

SG_BEGIN_DECL()

XBT_PUBLIC(void) sg_host_energy_plugin_init();
XBT_PUBLIC(double) sg_host_get_consumed_energy(sg_host_t host);
XBT_PUBLIC(double) sg_host_get_wattmin_at(sg_host_t host, int pstate);
XBT_PUBLIC(double) sg_host_get_wattmax_at(sg_host_t host, int pstate);

#define MSG_host_energy_plugin_init() sg_host_energy_plugin_init()
#define MSG_host_get_consumed_energy(host) sg_host_get_consumed_energy(host)
#define MSG_host_get_wattmin_at(host,pstate) sg_host_get_wattmin_at(host,pstate)
#define MSG_host_get_wattmax_at(host,pstate) sg_host_get_wattmax_at(host,pstate)

XBT_PUBLIC(void) sg_link_energy_plugin_init();
XBT_PUBLIC(double) sg_link_get_consumed_energy(sg_link_t link);
XBT_PUBLIC(double) sg_link_get_consumped_power(sg_link_t link);
XBT_PUBLIC(double) sg_link_get_usage(sg_link_t link);
XBT_PUBLIC(void) sg_on_simulation_end();

#define MSG_link_energy_plugin_init() sg_link_energy_plugin_init()
#define MSG_link_get_consumed_energy(link) sg_link_get_consumed_energy(link)

SG_END_DECL()

#endif
