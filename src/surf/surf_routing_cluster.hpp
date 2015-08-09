/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_CLUSTER_HPP_
#define SURF_ROUTING_CLUSTER_HPP_

#include "surf_routing_none.hpp"
#include "network_interface.hpp"

/***********
 * Classes *
 ***********/
class AsCluster;


/* ************************************************** */
/* **************  Cluster ROUTING   **************** */

class AsCluster: public AsNone {
public:
  AsCluster();

  virtual void getRouteAndLatency(RoutingEdge *src, RoutingEdge *dst, sg_platf_route_cbarg_t into, double *latency);
  //xbt_dynar_t getOneLinkRoutes();
  //void parseRoute(sg_platf_route_cbarg_t route);
  //void parseASroute(sg_platf_route_cbarg_t route);

  void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges);
  //sg_platf_route_cbarg_t getBypassRoute(RoutingEdge *src, RoutingEdge *dst, double *lat);

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  int parsePU(RoutingEdge *elm); /* A host or a router, whatever */
  int parseAS(RoutingEdge *elm);
  virtual void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position);
  Link* p_backbone;
  void *p_loopback;
  RoutingEdge *p_router;
  int p_has_limiter;
  int p_has_loopback;
  int p_nb_links_per_node;

};


#endif /* SURF_ROUTING_CLUSTER_HPP_ */
