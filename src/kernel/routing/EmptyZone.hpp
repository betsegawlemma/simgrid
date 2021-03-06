/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_NONE_HPP_
#define SURF_ROUTING_NONE_HPP_

#include "src/kernel/routing/NetZoneImpl.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief NetZone with no routing, useful with the constant network model
 *
 *  Such netzones never contain any link, and the latency is always left unchanged:
 *  the constant time network model computes this latency externally.
 */

class XBT_PRIVATE EmptyZone : public NetZoneImpl {
public:
  explicit EmptyZone(NetZone* father, const char* name);
  ~EmptyZone() override;

  void getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t into, double* latency) override
  {
    /* There can't be route in an Empty zone */
  }

  void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) override;
};
}
}
} // namespace

#endif /* SURF_ROUTING_NONE_HPP_ */
