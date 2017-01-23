/* Copyright (c) 2009-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/dict.h>
#include <xbt/graph.h>
#include <xbt/log.h>

#include "src/kernel/routing/EmptyZone.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_none, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

EmptyZone::EmptyZone(NetZone* father, const char* name) : NetZoneImpl(father, name)
{
}

EmptyZone::~EmptyZone() = default;

void EmptyZone::getLocalRoute(NetPoint* /*src*/, NetPoint* /*dst*/, sg_platf_route_cbarg_t /*res*/, double* /*lat*/)
{
}

void EmptyZone::getGraph(xbt_graph_t /*graph*/, xbt_dict_t /*nodes*/, xbt_dict_t /*edges*/)
{
  XBT_ERROR("No routing no graph");
}
}
}
}
