/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MCER_IGNORE_H
#define SIMGRID_MCER_IGNORE_H

#include <xbt/dynar.h>

#include "mc/datatypes.h"
#include "mc/mc_process.h"

#include "xbt/misc.h"           /* SG_BEGIN_DECL */

SG_BEGIN_DECL();

XBT_INTERNAL void MCer_ignore_global_variable(const char *var_name);
XBT_INTERNAL void MC_heap_region_ignore_insert(mc_heap_ignore_region_t region);
XBT_INTERNAL void MC_heap_region_ignore_remove(void *address, size_t size);

SG_END_DECL();

#endif