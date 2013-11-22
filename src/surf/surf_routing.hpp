#include "surf.hpp"
#include <float.h>

#ifndef NETWORK_ROUTING_HPP_
#define NETWORK_ROUTING_HPP_

void routing_model_create( void *loopback);

/* ************************************************************************** */
/* ************************* GRAPH EXPORTING FUNCTIONS ********************** */
xbt_node_t new_xbt_graph_node (xbt_graph_t graph, const char *name, xbt_dict_t nodes);
xbt_edge_t new_xbt_graph_edge (xbt_graph_t graph, xbt_node_t s, xbt_node_t d, xbt_dict_t edges);

/***********
 * Classes *
 ***********/
struct As;
typedef As *AsPtr;

class RoutingModelDescription;
typedef RoutingModelDescription *RoutingModelDescriptionPtr;

struct RoutingEdge;
typedef RoutingEdge *RoutingEdgePtr;

class Onelink;
typedef Onelink *OnelinkPtr;

class RoutingPlatf;
typedef RoutingPlatf *RoutingPlatfPtr;


/*FIXME:class RoutingModelDescription {
  const char *p_name;
  const char *p_desc;
  AsPtr create();
  void end(AsPtr as);
};*/

struct As {
public:
  xbt_dynar_t p_indexNetworkElm;
  xbt_dict_t p_bypassRoutes;    /* store bypass routes */
  routing_model_description_t p_modelDesc;
  e_surf_routing_hierarchy_t p_hierarchy;
  char *p_name;
  AsPtr p_routingFather;
  xbt_dict_t p_routingSons;
  RoutingEdgePtr p_netElem;
  xbt_dynar_t p_linkUpDownList;

  As(){};
  virtual ~As(){
	xbt_free(p_name);
  };

  virtual void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t into, double *latency)=0;
  virtual xbt_dynar_t getOneLinkRoutes()=0;
  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)=0;
  virtual sg_platf_route_cbarg_t getBypassRoute(RoutingEdgePtr src, RoutingEdgePtr dst, double *lat)=0;

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  virtual int parsePU(RoutingEdgePtr elm)=0; /* A host or a router, whatever */
  virtual int parseAS( RoutingEdgePtr elm)=0;
  virtual void parseRoute(sg_platf_route_cbarg_t route)=0;
  virtual void parseASroute(sg_platf_route_cbarg_t route)=0;
  virtual void parseBypassroute(sg_platf_route_cbarg_t e_route)=0;
};

struct RoutingEdge {
public:
  ~RoutingEdge() { xbt_free(p_name);};
  AsPtr p_rcComponent;
  e_surf_network_element_type_t p_rcType;
  int m_id;
  char *p_name;
};

/*
 * Link of lenght 1, alongside with its source and destination. This is mainly usefull in the bindings to gtnets and ns3
 */
class Onelink {
public:
  Onelink(void *link, RoutingEdgePtr src, RoutingEdgePtr dst)
    : p_src(src), p_dst(dst), p_link(link) {};
  RoutingEdgePtr p_src;
  RoutingEdgePtr p_dst;
  void *p_link;
};

class RoutingPlatf {
public:
  ~RoutingPlatf();
  AsPtr p_root;
  void *p_loopback;
  xbt_dynar_t p_lastRoute;
  xbt_dynar_t getOneLinkRoutes(void);
  xbt_dynar_t recursiveGetOneLinkRoutes(AsPtr rc);
  void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, xbt_dynar_t * links, double *latency);
};

#endif /* NETWORK_ROUTING_HPP_ */
