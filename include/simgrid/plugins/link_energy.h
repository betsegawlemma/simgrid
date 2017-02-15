/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_ENERGY_H_
#define SIMGRID_PLUGINS_ENERGY_H_

#include <xbt/base.h>
#include <simgrid/forward.h>

SG_BEGIN_DECL()

XBT_PUBLIC(void) sg_link_energy_plugin_init();
XBT_PUBLIC(double) sg_link_get_consumed_energy(sg_link_t link);

#define MSG_link_energy_plugin_init() sg_link_energy_plugin_init()
#define MSG_link_get_consumed_energy(link) sg_link_get_consumed_energy(link)

SG_END_DECL()

#endif
