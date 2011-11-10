/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <pcre.h>               /* regular expression library */

#include "simgrid/platf_interface.h"    // platform creation API internal interface

#include "surf_routing_private.h"
#include "surf/surf_routing.h"
#include "surf/surfxml_parse_values.h"

xbt_lib_t host_lib;
int ROUTING_HOST_LEVEL;         //Routing level
int SURF_CPU_LEVEL;             //Surf cpu level
int SURF_WKS_LEVEL;             //Surf workstation level
int SIMIX_HOST_LEVEL;           //Simix level
int MSG_HOST_LEVEL;             //Msg level
int SD_HOST_LEVEL;              //Simdag level
int COORD_HOST_LEVEL;           //Coordinates level
int NS3_HOST_LEVEL;             //host node for ns3

xbt_lib_t link_lib;
int SD_LINK_LEVEL;              //Simdag level
int SURF_LINK_LEVEL;            //Surf level
int NS3_LINK_LEVEL;             //link for ns3

xbt_lib_t as_router_lib;
int ROUTING_ASR_LEVEL;          //Routing level
int COORD_ASR_LEVEL;            //Coordinates level
int NS3_ASR_LEVEL;              //host node for ns3

static xbt_dict_t random_value = NULL;

/* Global vars */
routing_global_t global_routing = NULL;
AS_t current_routing = NULL;

/* global parse functions */
xbt_dynar_t link_list = NULL;   /* temporary store of current list link of a route */
static const char *src = NULL;  /* temporary store the source name of a route */
static const char *dst = NULL;  /* temporary store the destination name of a route */
static char *gw_src = NULL;     /* temporary store the gateway source name of a route */
static char *gw_dst = NULL;     /* temporary store the gateway destination name of a route */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route, surf, "Routing part of surf");

static void routing_parse_peer(sg_platf_peer_cbarg_t peer);     /* peer bypass */
static void routing_parse_Srandom(void);        /* random bypass */

static char *replace_random_parameter(char *chaine);
static void routing_parse_postparse(void);

/* this lines are only for replace use like index in the model table */
typedef enum {
  SURF_MODEL_FULL = 0,
  SURF_MODEL_FLOYD,
  SURF_MODEL_DIJKSTRA,
  SURF_MODEL_DIJKSTRACACHE,
  SURF_MODEL_NONE,
  SURF_MODEL_RULEBASED,
  SURF_MODEL_VIVALDI,
  SURF_MODEL_CLUSTER
} e_routing_types;

struct s_model_type routing_models[] = {
  {"Full",
   "Full routing data (fast, large memory requirements, fully expressive)",
   model_full_create, model_full_end},
  {"Floyd",
   "Floyd routing data (slow initialization, fast lookup, lesser memory requirements, shortest path routing only)",
   model_floyd_create, model_floyd_end},
  {"Dijkstra",
   "Dijkstra routing data (fast initialization, slow lookup, small memory requirements, shortest path routing only)",
   model_dijkstra_create, model_dijkstra_both_end},
  {"DijkstraCache",
   "Dijkstra routing data (fast initialization, fast lookup, small memory requirements, shortest path routing only)",
   model_dijkstracache_create, model_dijkstra_both_end},
  {"none", "No routing (usable with Constant network only)",
   model_none_create,  NULL},
  {"RuleBased", "Rule-Based routing data (...)",
   model_rulebased_create, NULL},
  {"Vivaldi", "Vivaldi routing",
   model_vivaldi_create, NULL},
  {"Cluster", "Cluster routing",
   model_cluster_create, NULL},
  {NULL, NULL, NULL, NULL}
};

/**
 * \brief Add a "host" to the network element list
 */
static void parse_S_host(sg_platf_host_cbarg_t host)
{
  network_element_info_t info = NULL;
  if (current_routing->hierarchy == SURF_ROUTING_NULL)
    current_routing->hierarchy = SURF_ROUTING_BASE;
  xbt_assert(!xbt_lib_get_or_null(host_lib, host->id, ROUTING_HOST_LEVEL),
             "Reading a host, processing unit \"%s\" already exists", host->id);

  (*(current_routing->parse_PU)) (current_routing, host->id);
  info = xbt_new0(s_network_element_info_t, 1);
  info->rc_component = current_routing;
  info->rc_type = SURF_NETWORK_ELEMENT_HOST;
  xbt_lib_set(host_lib, host->id, ROUTING_HOST_LEVEL, (void *) info);
  if (host->coord && strcmp(host->coord, "")) {
    if (!COORD_HOST_LEVEL)
      xbt_die
          ("To use coordinates, you must set configuration 'coordinates' to 'yes'");
    xbt_dynar_t ctn = xbt_str_split_str(host->coord, " ");
    xbt_dynar_shrink(ctn, 0);
    xbt_lib_set(host_lib, host->id, COORD_HOST_LEVEL, (void *) ctn);
  }
}

/**
 * \brief Add a "router" to the network element list
 */
static void parse_S_router(sg_platf_router_cbarg_t router)
{
  network_element_info_t info = NULL;
  if (current_routing->hierarchy == SURF_ROUTING_NULL)
    current_routing->hierarchy = SURF_ROUTING_BASE;
  xbt_assert(!xbt_lib_get_or_null(as_router_lib, router->id, ROUTING_ASR_LEVEL),
             "Reading a router, processing unit \"%s\" already exists",
             router->id);

  (*(current_routing->parse_PU)) (current_routing, router->id);
  info = xbt_new0(s_network_element_info_t, 1);
  info->rc_component = current_routing;
  info->rc_type = SURF_NETWORK_ELEMENT_ROUTER;

  xbt_lib_set(as_router_lib, router->id, ROUTING_ASR_LEVEL, (void *) info);
  if (strcmp(router->coord, "")) {
    if (!COORD_ASR_LEVEL)
      xbt_die
          ("To use coordinates, you must set configuration 'coordinates' to 'yes'");
    xbt_dynar_t ctn = xbt_str_split_str(router->coord, " ");
    xbt_dynar_shrink(ctn, 0);
    xbt_lib_set(as_router_lib, router->id, COORD_ASR_LEVEL, (void *) ctn);
  }
}


/**
 * \brief Set the end points for a route
 */
static void routing_parse_S_route(void)
{
  if (src != NULL && dst != NULL && link_list != NULL)
    THROWF(arg_error, 0, "Route between %s to %s can not be defined",
           A_surfxml_route_src, A_surfxml_route_dst);
  src = A_surfxml_route_src;
  dst = A_surfxml_route_dst;
  xbt_assert(strlen(src) > 0 || strlen(dst) > 0,
             "Some limits are null in the route between \"%s\" and \"%s\"",
             src, dst);
  link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

/**
 * \brief Set the end points and gateways for a ASroute
 */
static void routing_parse_S_ASroute(void)
{
  if (src != NULL && dst != NULL && link_list != NULL)
    THROWF(arg_error, 0, "Route between %s to %s can not be defined",
           A_surfxml_ASroute_src, A_surfxml_ASroute_dst);
  src = A_surfxml_ASroute_src;
  dst = A_surfxml_ASroute_dst;
  gw_src = A_surfxml_ASroute_gw_src;
  gw_dst = A_surfxml_ASroute_gw_dst;
  xbt_assert(strlen(src) > 0 || strlen(dst) > 0 || strlen(gw_src) > 0
             || strlen(gw_dst) > 0,
             "Some limits are null in the route between \"%s\" and \"%s\"",
             src, dst);
  link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

/**
 * \brief Set the end points for a bypassRoute
 */
static void routing_parse_S_bypassRoute(void)
{
  if (src != NULL && dst != NULL && link_list != NULL)
    THROWF(arg_error, 0,
           "Bypass Route between %s to %s can not be defined",
           A_surfxml_bypassRoute_src, A_surfxml_bypassRoute_dst);
  src = A_surfxml_bypassRoute_src;
  dst = A_surfxml_bypassRoute_dst;
  gw_src = A_surfxml_bypassRoute_gw_src;
  gw_dst = A_surfxml_bypassRoute_gw_dst;
  xbt_assert(strlen(src) > 0 || strlen(dst) > 0 || strlen(gw_src) > 0
             || strlen(gw_dst) > 0,
             "Some limits are null in the route between \"%s\" and \"%s\"",
             src, dst);
  link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

/**
 * \brief Set a new link on the actual list of link for a route or ASroute from XML
 */

static void routing_parse_link_ctn(void)
{
  char *link_id;
  switch (A_surfxml_link_ctn_direction) {
  case AU_surfxml_link_ctn_direction:
  case A_surfxml_link_ctn_direction_NONE:
    link_id = xbt_strdup(A_surfxml_link_ctn_id);
    break;
  case A_surfxml_link_ctn_direction_UP:
    link_id = bprintf("%s_UP", A_surfxml_link_ctn_id);
    break;
  case A_surfxml_link_ctn_direction_DOWN:
    link_id = bprintf("%s_DOWN", A_surfxml_link_ctn_id);
    break;
  }
  xbt_dynar_push(link_list, &link_id);
}

/**
 * \brief Store the route by calling the set_route function of the current routing component
 */
static void routing_parse_E_route(void)
{
  route_extended_t route = xbt_new0(s_route_extended_t, 1);
  route->generic_route.link_list = link_list;
  xbt_assert(current_routing->parse_route,
             "no defined method \"set_route\" in \"%s\"",
             current_routing->name);
  (*(current_routing->parse_route)) (current_routing, src, dst, route);
  link_list = NULL;
  src = NULL;
  dst = NULL;
}

/**
 * \brief Store the ASroute by calling the set_ASroute function of the current routing component
 */
static void routing_parse_E_ASroute(void)
{
  route_extended_t e_route = xbt_new0(s_route_extended_t, 1);
  e_route->generic_route.link_list = link_list;
  e_route->src_gateway = xbt_strdup(gw_src);
  e_route->dst_gateway = xbt_strdup(gw_dst);
  xbt_assert(current_routing->parse_ASroute,
             "no defined method \"set_ASroute\" in \"%s\"",
             current_routing->name);
  (*(current_routing->parse_ASroute)) (current_routing, src, dst, e_route);
  link_list = NULL;
  src = NULL;
  dst = NULL;
  gw_src = NULL;
  gw_dst = NULL;
}

/**
 * \brief Store the bypass route by calling the set_bypassroute function of the current routing component
 */
static void routing_parse_E_bypassRoute(void)
{
  route_extended_t e_route = xbt_new0(s_route_extended_t, 1);
  e_route->generic_route.link_list = link_list;
  e_route->src_gateway = xbt_strdup(gw_src);
  e_route->dst_gateway = xbt_strdup(gw_dst);
  xbt_assert(current_routing->parse_bypassroute,
             "Bypassing mechanism not implemented by routing '%s'",
             current_routing->name);
  (*(current_routing->parse_bypassroute)) (current_routing, src, dst, e_route);
  link_list = NULL;
  src = NULL;
  dst = NULL;
  gw_src = NULL;
  gw_dst = NULL;
}

/**
 * \brief Make a new routing component to the platform
 *
 * Add a new autonomous system to the platform. Any elements (such as host,
 * router or sub-AS) added after this call and before the corresponding call
 * to sg_platf_new_AS_close() will be added to this AS.
 *
 * Once this function was called, the configuration concerning the used
 * models cannot be changed anymore.
 *
 * @param AS_id name of this autonomous system. Must be unique in the platform
 * @param wanted_routing_type one of Full, Floyd, Dijkstra or similar. Full list in the variable routing_models, in src/surf/surf_routing.c
 */
void routing_AS_begin(const char *AS_id, const char *wanted_routing_type)
{
  AS_t new_routing;
  routing_model_description_t model = NULL;
  int cpt;

  /* search the routing model */
  for (cpt = 0; routing_models[cpt].name; cpt++)
    if (!strcmp(wanted_routing_type, routing_models[cpt].name))
      model = &routing_models[cpt];
  /* if its not exist, error */
  if (model == NULL) {
    fprintf(stderr, "Routing model %s not found. Existing models:\n",
            wanted_routing_type);
    for (cpt = 0; routing_models[cpt].name; cpt++)
      fprintf(stderr, "   %s: %s\n", routing_models[cpt].name,
              routing_models[cpt].desc);
    xbt_die(NULL);
  }

  /* make a new routing component */
  new_routing = (AS_t) (*(model->create)) ();
  new_routing->routing = model;
  new_routing->hierarchy = SURF_ROUTING_NULL;
  new_routing->name = xbt_strdup(AS_id);
  new_routing->routing_sons = xbt_dict_new();

  if (current_routing == NULL && global_routing->root == NULL) {

    /* it is the first one */
    new_routing->routing_father = NULL;
    global_routing->root = new_routing;

  } else if (current_routing != NULL && global_routing->root != NULL) {

    xbt_assert(!xbt_dict_get_or_null
               (current_routing->routing_sons, AS_id),
               "The AS \"%s\" already exists", AS_id);
    /* it is a part of the tree */
    new_routing->routing_father = current_routing;
    /* set the father behavior */
    if (current_routing->hierarchy == SURF_ROUTING_NULL)
      current_routing->hierarchy = SURF_ROUTING_RECURSIVE;
    /* add to the sons dictionary */
    xbt_dict_set(current_routing->routing_sons, AS_id,
                 (void *) new_routing, NULL);
    /* add to the father element list */
    (*(current_routing->parse_AS)) (current_routing, AS_id);
  } else {
    THROWF(arg_error, 0, "All defined components must be belong to a AS");
  }
  /* set the new current component of the tree */
  current_routing = new_routing;
}

/**
 * \brief Specify that the current description of AS is finished
 *
 * Once you've declared all the content of your AS, you have to close
 * it with this call. Your AS is not usable until you call this function.
 *
 * @fixme: this call is not as robust as wanted: bad things WILL happen
 * if you call it twice for the same AS, or if you forget calling it, or
 * even if you add stuff to a closed AS
 *
 */
void routing_AS_end()
{

  if (current_routing == NULL) {
    THROWF(arg_error, 0, "Close an AS, but none was under construction");
  } else {
    network_element_info_t info = NULL;
    xbt_assert(!xbt_lib_get_or_null
               (as_router_lib, current_routing->name, ROUTING_ASR_LEVEL),
               "The AS \"%s\" already exists", current_routing->name);
    info = xbt_new0(s_network_element_info_t, 1);
    info->rc_component = current_routing->routing_father;
    info->rc_type = SURF_NETWORK_ELEMENT_AS;
    xbt_lib_set(as_router_lib, current_routing->name, ROUTING_ASR_LEVEL,
                (void *) info);

    if (current_routing->routing->end)
      (*(current_routing->routing->end)) (current_routing);
    current_routing = current_routing->routing_father;
  }
}

/* Aux Business methods */

/**
 * \brief Get the AS father and the first elements of the chain
 *
 * \param src the source host name 
 * \param dst the destination host name
 * 
 * Get the common father of the to processing units, and the first different 
 * father in the chain
 */
static void elements_father(const char *src, const char *dst,
                            AS_t * res_father,
                            AS_t * res_src,
                            AS_t * res_dst)
{
  xbt_assert(src && dst, "bad parameters for \"elements_father\" method");
#define ELEMENTS_FATHER_MAXDEPTH 16     /* increase if it is not enough */
  AS_t src_as, dst_as;
  AS_t path_src[ELEMENTS_FATHER_MAXDEPTH];
  AS_t path_dst[ELEMENTS_FATHER_MAXDEPTH];
  int index_src = 0;
  int index_dst = 0;
  AS_t current;
  AS_t current_src;
  AS_t current_dst;
  AS_t father;

  /* (1) find the as where the src and dst are located */
  network_element_info_t src_data = xbt_lib_get_or_null(host_lib, src,
                                                        ROUTING_HOST_LEVEL);
  network_element_info_t dst_data = xbt_lib_get_or_null(host_lib, dst,
                                                        ROUTING_HOST_LEVEL);
  if (!src_data)
    src_data = xbt_lib_get_or_null(as_router_lib, src, ROUTING_ASR_LEVEL);
  if (!dst_data)
    dst_data = xbt_lib_get_or_null(as_router_lib, dst, ROUTING_ASR_LEVEL);
  src_as = src_data->rc_component;
  dst_as = dst_data->rc_component;

  xbt_assert(src_as && dst_as,
             "Ask for route \"from\"(%s) or \"to\"(%s) no found", src, dst);

  /* (2) find the path to the root routing component */
  for (current = src_as; current != NULL; current = current->routing_father) {
    if (index_src >= ELEMENTS_FATHER_MAXDEPTH)
      xbt_die("ELEMENTS_FATHER_MAXDEPTH should be increased for path_src");
    path_src[index_src++] = current;
  }
  for (current = dst_as; current != NULL; current = current->routing_father) {
    if (index_dst >= ELEMENTS_FATHER_MAXDEPTH)
      xbt_die("ELEMENTS_FATHER_MAXDEPTH should be increased for path_dst");
    path_dst[index_dst++] = current;
  }

  /* (3) find the common father */
  do {
    current_src = path_src[--index_src];
    current_dst = path_dst[--index_dst];
  } while (index_src > 0 && index_dst > 0 && current_src == current_dst);

  /* (4) they are not in the same routing component, make the path */
  if (current_src == current_dst)
    father = current_src;
  else
    father = path_src[index_src + 1];

  /* (5) result generation */
  *res_father = father;         /* first the common father of src and dst */
  *res_src = current_src;       /* second the first different father of src */
  *res_dst = current_dst;       /* three  the first different father of dst */

#undef ELEMENTS_FATHER_MAXDEPTH
}

/* Global Business methods */

/**
 * \brief Recursive function for get_route_latency
 *
 * \param src the source host name 
 * \param dst the destination host name
 * \param *e_route the route where the links are stored
 * \param *latency the latency, if needed
 * 
 * This function is called by "get_route" and "get_latency". It allows to walk
 * recursively through the routing components tree.
 */
static void _get_route_latency(const char *src, const char *dst,
                               xbt_dynar_t * route, double *latency)
{
  XBT_DEBUG("Solve route/latency  \"%s\" to \"%s\"", src, dst);
  xbt_assert(src && dst, "bad parameters for \"_get_route_latency\" method");

  AS_t common_father;
  AS_t src_father;
  AS_t dst_father;
  elements_father(src, dst, &common_father, &src_father, &dst_father);

  if (src_father == dst_father) {       /* SURF_ROUTING_BASE */

    route_extended_t e_route = NULL;
    if (route) {
      e_route = common_father->get_route(common_father, src, dst);
      xbt_assert(e_route, "no route between \"%s\" and \"%s\"", src, dst);
      *route = e_route->generic_route.link_list;
    }
    if (latency) {
      *latency = common_father->get_latency(common_father, src, dst, e_route);
      xbt_assert(*latency >= 0.0,
                 "latency error on route between \"%s\" and \"%s\"", src, dst);
    }
    if (e_route) {
      xbt_free(e_route->src_gateway);
      xbt_free(e_route->dst_gateway);
      xbt_free(e_route);
    }

  } else {                      /* SURF_ROUTING_RECURSIVE */

    route_extended_t e_route_bypass = NULL;
    if (common_father->get_bypass_route)
      e_route_bypass = common_father->get_bypass_route(common_father, src, dst);

    xbt_assert(!latency || !e_route_bypass,
               "Bypass cannot work yet with get_latency");

    route_extended_t e_route_cnt = e_route_bypass
        ? e_route_bypass : common_father->get_route(common_father,
                                                    src_father->name,
                                                    dst_father->name);

    xbt_assert(e_route_cnt, "no route between \"%s\" and \"%s\"",
               src_father->name, dst_father->name);

    xbt_assert((e_route_cnt->src_gateway == NULL) ==
               (e_route_cnt->dst_gateway == NULL),
               "bad gateway for route between \"%s\" and \"%s\"", src, dst);

    if (route) {
      *route = xbt_dynar_new(global_routing->size_of_link, NULL);
    }
    if (latency) {
      *latency = common_father->get_latency(common_father,
                                            src_father->name, dst_father->name,
                                            e_route_cnt);
      xbt_assert(*latency >= 0.0,
                 "latency error on route between \"%s\" and \"%s\"",
                 src_father->name, dst_father->name);
    }

    void *link;
    unsigned int cpt;

    if (strcmp(src, e_route_cnt->src_gateway)) {
      double latency_src;
      xbt_dynar_t route_src;

      _get_route_latency(src, e_route_cnt->src_gateway,
                         (route ? &route_src : NULL),
                         (latency ? &latency_src : NULL));
      if (route) {
        xbt_assert(route_src, "no route between \"%s\" and \"%s\"",
                   src, e_route_cnt->src_gateway);
        xbt_dynar_foreach(route_src, cpt, link) {
          xbt_dynar_push(*route, &link);
        }
        xbt_dynar_free(&route_src);
      }
      if (latency) {
        xbt_assert(latency_src >= 0.0,
                   "latency error on route between \"%s\" and \"%s\"",
                   src, e_route_cnt->src_gateway);
        *latency += latency_src;
      }
    }

    if (route) {
      xbt_dynar_foreach(e_route_cnt->generic_route.link_list, cpt, link) {
        xbt_dynar_push(*route, &link);
      }
    }

    if (strcmp(e_route_cnt->dst_gateway, dst)) {
      double latency_dst;
      xbt_dynar_t route_dst;

      _get_route_latency(e_route_cnt->dst_gateway, dst,
                         (route ? &route_dst : NULL),
                         (latency ? &latency_dst : NULL));
      if (route) {
        xbt_assert(route_dst, "no route between \"%s\" and \"%s\"",
                   e_route_cnt->dst_gateway, dst);
        xbt_dynar_foreach(route_dst, cpt, link) {
          xbt_dynar_push(*route, &link);
        }
        xbt_dynar_free(&route_dst);
      }
      if (latency) {
        xbt_assert(latency_dst >= 0.0,
                   "latency error on route between \"%s\" and \"%s\"",
                   e_route_cnt->dst_gateway, dst);
        *latency += latency_dst;
      }
    }

    generic_free_extended_route(e_route_cnt);
  }
}

/**
 * \brief Generic function for get_route, get_route_no_cleanup, and get_latency
 */
static void get_route_latency(const char *src, const char *dst,
                              xbt_dynar_t * route, double *latency, int cleanup)
{
  _get_route_latency(src, dst, route, latency);
  xbt_assert(!route || *route, "no route between \"%s\" and \"%s\"", src, dst);
  xbt_assert(!latency || *latency >= 0.0,
             "latency error on route between \"%s\" and \"%s\"", src, dst);
  if (route) {
    xbt_dynar_free(&global_routing->last_route);
    global_routing->last_route = cleanup ? *route : NULL;
  }
}

/**
 * \brief Generic method: find a route between hosts
 *
 * \param src the source host name 
 * \param dst the destination host name
 * 
 * walk through the routing components tree and find a route between hosts
 * by calling the differents "get_route" functions in each routing component.
 * No need to free the returned dynar. It will be freed at the next call.
 */
static xbt_dynar_t get_route(const char *src, const char *dst)
{
  xbt_dynar_t route = NULL;
  get_route_latency(src, dst, &route, NULL, 1);
  return route;
}

/**
 * \brief Generic method: find a route between hosts
 *
 * \param src the source host name
 * \param dst the destination host name
 *
 * same as get_route, but return NULL if any exception is raised.
 */
static xbt_dynar_t get_route_or_null(const char *src, const char *dst)
{
  xbt_dynar_t route = NULL;
  xbt_ex_t exception;
  TRY {
    get_route_latency(src, dst, &route, NULL, 1);
  } CATCH(exception) {
    xbt_ex_free(exception);
    return NULL;
  }
  return route;
}

/**
 * \brief Generic method: find a route between hosts
 *
 * \param src the source host name
 * \param dst the destination host name
 *
 * walk through the routing components tree and find a route between hosts
 * by calling the differents "get_route" functions in each routing component.
 * Leaves the caller the responsability to clean the returned dynar.
 */
static xbt_dynar_t get_route_no_cleanup(const char *src, const char *dst)
{
  xbt_dynar_t route = NULL;
  get_route_latency(src, dst, &route, NULL, 0);
  return route;
}

static double get_latency(const char *src, const char *dst) {
  double latency = -1.0;
  get_route_latency(src, dst, NULL, &latency, 0);
  return latency;
}


static xbt_dynar_t recursive_get_onelink_routes(AS_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(onelink_t), xbt_free);

  //adding my one link routes
  unsigned int cpt;
  void *link;
  xbt_dynar_t onelink_mine = rc->get_onelink_routes(rc);
  if (onelink_mine) {
    xbt_dynar_foreach(onelink_mine, cpt, link) {
      xbt_dynar_push(ret, &link);
    }
  }
  //recursing
  char *key;
  xbt_dict_cursor_t cursor = NULL;
  AS_t rc_child;
  xbt_dict_foreach(rc->routing_sons, cursor, key, rc_child) {
    xbt_dynar_t onelink_child = recursive_get_onelink_routes(rc_child);
    if (onelink_child) {
      xbt_dynar_foreach(onelink_child, cpt, link) {
        xbt_dynar_push(ret, &link);
      }
    }
  }
  return ret;
}

static xbt_dynar_t get_onelink_routes(void)
{
  return recursive_get_onelink_routes(global_routing->root);
}

e_surf_network_element_type_t get_network_element_type(const char *name)
{
  network_element_info_t rc = NULL;

  rc = xbt_lib_get_or_null(host_lib, name, ROUTING_HOST_LEVEL);
  if (rc)
    return rc->rc_type;

  rc = xbt_lib_get_or_null(as_router_lib, name, ROUTING_ASR_LEVEL);
  if (rc)
    return rc->rc_type;

  return SURF_NETWORK_ELEMENT_NULL;
}

/**
 * \brief Generic method: create the global routing schema
 * 
 * Make a global routing structure and set all the parsing functions.
 */
void routing_model_create(size_t size_of_links, void *loopback)
{
  /* config the uniq global routing */
  global_routing = xbt_new0(s_routing_global_t, 1);
  global_routing->root = NULL;
  global_routing->get_route = get_route;
  global_routing->get_route_or_null = get_route_or_null;
  global_routing->get_latency = get_latency;
  global_routing->get_route_no_cleanup = get_route_no_cleanup;
  global_routing->get_onelink_routes = get_onelink_routes;
  global_routing->get_route_latency = get_route_latency;
  global_routing->get_network_element_type = get_network_element_type;
  global_routing->loopback = loopback;
  global_routing->size_of_link = size_of_links;
  global_routing->last_route = NULL;
  /* no current routing at moment */
  current_routing = NULL;
}


/* ************************************************** */
/* ********** PATERN FOR NEW ROUTING **************** */

/* The minimal configuration of a new routing model need the next functions,
 * also you need to set at the start of the file, the new model in the model
 * list. Remember keep the null ending of the list.
 */
/*** Routing model structure ***/
// typedef struct {
//   s_routing_component_t generic_routing;
//   /* things that your routing model need */
// } s_routing_component_NEW_t,*routing_component_NEW_t;

/*** Parse routing model functions ***/
// static void model_NEW_set_processing_unit(routing_component_t rc, const char* name) {}
// static void model_NEW_set_autonomous_system(routing_component_t rc, const char* name) {}
// static void model_NEW_set_route(routing_component_t rc, const char* src, const char* dst, route_t route) {}
// static void model_NEW_set_ASroute(routing_component_t rc, const char* src, const char* dst, route_extended_t route) {}
// static void model_NEW_set_bypassroute(routing_component_t rc, const char* src, const char* dst, route_extended_t e_route) {}

/*** Business methods ***/
// static route_extended_t NEW_get_route(routing_component_t rc, const char* src,const char* dst) {return NULL;}
// static route_extended_t NEW_get_bypass_route(routing_component_t rc, const char* src,const char* dst) {return NULL;}
// static void NEW_finalize(routing_component_t rc) { xbt_free(rc);}

/*** Creation routing model functions ***/
// static void* model_NEW_create(void) {
//   routing_component_NEW_t new_component =  xbt_new0(s_routing_component_NEW_t,1);
//   new_component->generic_routing.set_processing_unit = model_NEW_set_processing_unit;
//   new_component->generic_routing.set_autonomous_system = model_NEW_set_autonomous_system;
//   new_component->generic_routing.set_route = model_NEW_set_route;
//   new_component->generic_routing.set_ASroute = model_NEW_set_ASroute;
//   new_component->generic_routing.set_bypassroute = model_NEW_set_bypassroute;
//   new_component->generic_routing.get_route = NEW_get_route;
//   new_component->generic_routing.get_bypass_route = NEW_get_bypass_route;
//   new_component->generic_routing.finalize = NEW_finalize;
//   /* initialization of internal structures */
//   return new_component;
// } /* mandatory */
// static void  model_NEW_load(void) {}   /* mandatory */
// static void  model_NEW_unload(void) {} /* mandatory */
// static void  model_NEW_end(void) {}    /* mandatory */

/* ************************************************************************** */
/* ************************* GENERIC PARSE FUNCTIONS ************************ */


static void routing_parse_cluster(void)
{
  char *host_id, *groups, *link_id = NULL;
  xbt_dict_t patterns = NULL;

  s_sg_platf_host_cbarg_t host;
  s_sg_platf_link_cbarg_t link;

  if (strcmp(struct_cluster->availability_trace, "")
      || strcmp(struct_cluster->state_trace, "")) {
    patterns = xbt_dict_new();
    xbt_dict_set(patterns, "id", xbt_strdup(struct_cluster->id), free);
    xbt_dict_set(patterns, "prefix", xbt_strdup(struct_cluster->prefix), free);
    xbt_dict_set(patterns, "suffix", xbt_strdup(struct_cluster->suffix), free);
  }

  unsigned int iter;
  int start, end, i;
  xbt_dynar_t radical_elements;
  xbt_dynar_t radical_ends;

  XBT_DEBUG("<AS id=\"%s\"\trouting=\"Cluster\">", struct_cluster->id);
  sg_platf_new_AS_begin(struct_cluster->id, "Cluster");

  //Make all hosts
  radical_elements = xbt_str_split(struct_cluster->radical, ",");
  xbt_dynar_foreach(radical_elements, iter, groups) {
    memset(&host, 0, sizeof(host));

    radical_ends = xbt_str_split(groups, "-");
    start = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 0, char *));

    switch (xbt_dynar_length(radical_ends)) {
    case 1:
      end = start;
      break;
    case 2:
      end = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 1, char *));
      break;
    default:
      surf_parse_error("Malformed radical");
      break;
    }
    for (i = start; i <= end; i++) {
      host_id =
          bprintf("%s%d%s", struct_cluster->prefix, i, struct_cluster->suffix);
      link_id = bprintf("%s_link_%d", struct_cluster->id, i);

      XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%f\">", host_id,
                struct_cluster->power);
      host.id = host_id;
      if (strcmp(struct_cluster->availability_trace, "")) {
        xbt_dict_set(patterns, "radical", bprintf("%d", i), xbt_free);
        char *tmp_availability_file =
            xbt_str_varsubst(struct_cluster->availability_trace, patterns);
        XBT_DEBUG("\tavailability_file=\"%s\"", tmp_availability_file);
        host.power_trace = tmgr_trace_new(tmp_availability_file);
        xbt_free(tmp_availability_file);
      } else {
        XBT_DEBUG("\tavailability_file=\"\"");
      }
      if (strcmp(struct_cluster->state_trace, "")) {
        char *tmp_state_file =
            xbt_str_varsubst(struct_cluster->state_trace, patterns);
        XBT_DEBUG("\tstate_file=\"%s\"", tmp_state_file);
        host.state_trace = tmgr_trace_new(tmp_state_file);
        xbt_free(tmp_state_file);
      } else {
        XBT_DEBUG("\tstate_file=\"\"");
      }

      host.power_peak = struct_cluster->power;
      host.power_scale = 1.0;
      host.core_amount = struct_cluster->core_amount;
      host.initial_state = SURF_RESOURCE_ON;
      host.coord = "";
      sg_platf_new_host(&host);
      XBT_DEBUG("</host>");

      XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id,
                struct_cluster->bw, struct_cluster->lat);

      memset(&link, 0, sizeof(link));
      link.id = link_id;
      link.bandwidth = struct_cluster->bw;
      link.latency = struct_cluster->lat;
      link.state = SURF_RESOURCE_ON;

      switch (struct_cluster->sharing_policy) {
      case A_surfxml_cluster_sharing_policy_SHARED:
        link.policy = SURF_LINK_SHARED;
        break;
      case A_surfxml_cluster_sharing_policy_FULLDUPLEX:
        link.policy = SURF_LINK_FULLDUPLEX;
        break;
      case A_surfxml_cluster_sharing_policy_FATPIPE:
        link.policy = SURF_LINK_FATPIPE;
        break;
      default:
        surf_parse_error(bprintf
                         ("Invalid cluster sharing policy for cluster %s",
                          struct_cluster->id));
        break;
      }
      sg_platf_new_link(&link);

      surf_parsing_link_up_down_t info =
          xbt_new0(s_surf_parsing_link_up_down_t, 1);
      if (link.policy == SURF_LINK_FULLDUPLEX) {
        char *tmp_link = bprintf("%s_UP", link_id);
        info->link_up =
            xbt_lib_get_or_null(link_lib, tmp_link, SURF_LINK_LEVEL);
        free(tmp_link);
        tmp_link = bprintf("%s_DOWN", link_id);
        info->link_down =
            xbt_lib_get_or_null(link_lib, tmp_link, SURF_LINK_LEVEL);
        free(tmp_link);
      } else {
        info->link_up = xbt_lib_get_or_null(link_lib, link_id, SURF_LINK_LEVEL);
        info->link_down = info->link_up;
      }
      surf_routing_cluster_add_link(host_id, info);

      xbt_free(link_id);
      xbt_free(host_id);
    }

    xbt_dynar_free(&radical_ends);
  }
  xbt_dynar_free(&radical_elements);

  // Add a router. It is magically used thanks to the way in which surf_routing_cluster is written, and it's very useful to connect clusters together
  XBT_DEBUG(" ");
  XBT_DEBUG("<router id=\"%s\"/>", struct_cluster->router_id);
  s_sg_platf_router_cbarg_t router;
  char *newid = NULL;
  memset(&router, 0, sizeof(router));
  router.id = struct_cluster->router_id;
  router.coord = "";
  if (!router.id || !strcmp(router.id, ""))
    router.id = newid =
        bprintf("%s%s_router%s", struct_cluster->prefix, struct_cluster->id,
                struct_cluster->suffix);
  sg_platf_new_router(&router);
  free(newid);

  //Make the backbone
  if ((struct_cluster->bb_bw != 0) && (struct_cluster->bb_lat != 0)) {
    char *link_backbone = bprintf("%s_backbone", struct_cluster->id);
    XBT_DEBUG("<link\tid=\"%s\" bw=\"%f\" lat=\"%f\"/>", link_backbone,
              struct_cluster->bb_bw, struct_cluster->bb_lat);

    memset(&link, 0, sizeof(link));
    link.id = link_backbone;
    link.bandwidth = struct_cluster->bb_bw;
    link.latency = struct_cluster->bb_lat;
    link.state = SURF_RESOURCE_ON;

    switch (struct_cluster->bb_sharing_policy) {
    case A_surfxml_cluster_bb_sharing_policy_FATPIPE:
      link.policy = SURF_LINK_FATPIPE;
      break;
    case A_surfxml_cluster_bb_sharing_policy_SHARED:
      link.policy = SURF_LINK_SHARED;
      break;
    default:
      surf_parse_error(bprintf
                       ("Invalid bb sharing policy in cluster %s",
                        struct_cluster->id));
      break;
    }

    sg_platf_new_link(&link);

    surf_parsing_link_up_down_t info =
        xbt_new0(s_surf_parsing_link_up_down_t, 1);
    info->link_up =
        xbt_lib_get_or_null(link_lib, link_backbone, SURF_LINK_LEVEL);
    info->link_down = info->link_up;
    surf_routing_cluster_add_link(struct_cluster->id, info);

    free(link_backbone);
  }

  XBT_DEBUG(" ");

  char *new_suffix = xbt_strdup("");

  radical_elements = xbt_str_split(struct_cluster->suffix, ".");
  xbt_dynar_foreach(radical_elements, iter, groups) {
    if (strcmp(groups, "")) {
      char *old_suffix = new_suffix;
      new_suffix = bprintf("%s\\.%s", old_suffix, groups);
      free(old_suffix);
    }
  }

  xbt_dynar_free(&radical_elements);
  xbt_free(new_suffix);

  XBT_DEBUG("</AS>");
  sg_platf_new_AS_end();
  XBT_DEBUG(" ");
  xbt_dict_free(&patterns); // no op if it were never set
}

static void routing_parse_postparse(void) {
  xbt_dict_free(&random_value);
}

static void routing_parse_peer(sg_platf_peer_cbarg_t peer)
{
  static int AX_ptr = 0;
  char *host_id = NULL;
  char *router_id, *link_router, *link_backbone, *link_id_up, *link_id_down;

  static unsigned int surfxml_buffer_stack_stack_ptr = 1;
  static unsigned int surfxml_buffer_stack_stack[1024];

  surfxml_buffer_stack_stack[0] = 0;

  surfxml_bufferstack_push(1);

  XBT_DEBUG("<AS id=\"%s\"\trouting=\"Full\">", peer->id);
  sg_platf_new_AS_begin(peer->id, "Full");

  XBT_DEBUG(" ");
  host_id = HOST_PEER(peer->id);
  router_id = ROUTER_PEER(peer->id);
  link_id_up = LINK_UP_PEER(peer->id);
  link_id_down = LINK_DOWN_PEER(peer->id);

  link_router = bprintf("%s_link_router", peer->id);
  link_backbone = bprintf("%s_backbone", peer->id);

  XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%f\"/>", host_id, peer->power);
  s_sg_platf_host_cbarg_t host;
  memset(&host, 0, sizeof(host));
  host.initial_state = SURF_RESOURCE_ON;
  host.id = host_id;
  host.power_peak = peer->power;
  host.power_scale = 1.0;
  host.power_trace = peer->availability_trace;
  host.state_trace = peer->state_trace;
  host.core_amount = 1;
  sg_platf_new_host(&host);


  XBT_DEBUG("<router id=\"%s\"\tcoordinates=\"%s\"/>", router_id, peer->coord);
  s_sg_platf_router_cbarg_t router;
  memset(&router, 0, sizeof(router));
  router.id = router_id;
  router.coord = peer->coord;
  sg_platf_new_router(&router);

  XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id_up,
            peer->bw_in, peer->lat);
  s_sg_platf_link_cbarg_t link;
  memset(&link, 0, sizeof(link));
  link.state = SURF_RESOURCE_ON;
  link.policy = SURF_LINK_SHARED;
  link.id = link_id_up;
  link.bandwidth = peer->bw_in;
  link.latency = peer->lat;
  sg_platf_new_link(&link);

  // FIXME: dealing with full duplex is not the role of this piece of code, I'd say [Mt]
  // Instead, it should be created fullduplex, and the models will do what's needed in this case
  XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id_down,
            peer->bw_out, peer->lat);
  link.id = link_id_down;
  sg_platf_new_link(&link);

  XBT_DEBUG(" ");

  // begin here
  XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", host_id, router_id);
  XBT_DEBUG("symmetrical=\"NO\">");
  SURFXML_BUFFER_SET(route_src, host_id);
  SURFXML_BUFFER_SET(route_dst, router_id);
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
  SURFXML_START_TAG(route);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", link_id_up);
  SURFXML_BUFFER_SET(link_ctn_id, link_id_up);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  //Opposite Route
  XBT_DEBUG("<route\tsrc=\"%s\"\tdst=\"%s\"", router_id, host_id);
  XBT_DEBUG("symmetrical=\"NO\">");
  SURFXML_BUFFER_SET(route_src, router_id);
  SURFXML_BUFFER_SET(route_dst, host_id);
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_NO;
  SURFXML_START_TAG(route);

  XBT_DEBUG("<link_ctn\tid=\"%s\"/>", link_id_down);
  SURFXML_BUFFER_SET(link_ctn_id, link_id_down);
  A_surfxml_link_ctn_direction = A_surfxml_link_ctn_direction_NONE;
  SURFXML_START_TAG(link_ctn);
  SURFXML_END_TAG(link_ctn);

  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  XBT_DEBUG("</AS>");
  sg_platf_new_AS_end();
  XBT_DEBUG(" ");

  //xbt_dynar_free(&tab_elements_num);
  free(host_id);
  free(router_id);
  free(link_router);
  free(link_backbone);
  free(link_id_up);
  free(link_id_down);
  surfxml_bufferstack_pop(1);
}

static void routing_parse_Srandom(void)
{
  double mean, std, min, max, seed;
  char *random_id = A_surfxml_random_id;
  char *random_radical = A_surfxml_random_radical;
  char *rd_name = NULL;
  char *rd_value;
  mean = surf_parse_get_double(A_surfxml_random_mean);
  std = surf_parse_get_double(A_surfxml_random_std_deviation);
  min = surf_parse_get_double(A_surfxml_random_min);
  max = surf_parse_get_double(A_surfxml_random_max);
  seed = surf_parse_get_double(A_surfxml_random_seed);

  double res = 0;
  int i = 0;
  random_data_t random = xbt_new0(s_random_data_t, 1);
  char *tmpbuf;

  xbt_dynar_t radical_elements;
  unsigned int iter;
  char *groups;
  int start, end;
  xbt_dynar_t radical_ends;

  random->generator = A_surfxml_random_generator;
  random->seed = seed;
  random->min = min;
  random->max = max;

  /* Check user stupidities */
  if (max < min)
    THROWF(arg_error, 0, "random->max < random->min (%f < %f)", max, min);
  if (mean < min)
    THROWF(arg_error, 0, "random->mean < random->min (%f < %f)", mean, min);
  if (mean > max)
    THROWF(arg_error, 0, "random->mean > random->max (%f > %f)", mean, max);

  /* normalize the mean and standard deviation before storing */
  random->mean = (mean - min) / (max - min);
  random->std = std / (max - min);

  if (random->mean * (1 - random->mean) < random->std * random->std)
    THROWF(arg_error, 0, "Invalid mean and standard deviation (%f and %f)",
           random->mean, random->std);

  XBT_DEBUG
      ("id = '%s' min = '%f' max = '%f' mean = '%f' std_deviatinon = '%f' generator = '%d' seed = '%ld' radical = '%s'",
       random_id, random->min, random->max, random->mean, random->std,
       random->generator, random->seed, random_radical);

  if (xbt_dict_size(random_value) == 0)
    random_value = xbt_dict_new();

  if (!strcmp(random_radical, "")) {
    res = random_generate(random);
    rd_value = bprintf("%f", res);
    xbt_dict_set(random_value, random_id, rd_value, free);
  } else {
    radical_elements = xbt_str_split(random_radical, ",");
    xbt_dynar_foreach(radical_elements, iter, groups) {
      radical_ends = xbt_str_split(groups, "-");
      switch (xbt_dynar_length(radical_ends)) {
      case 1:
        xbt_assert(!xbt_dict_get_or_null(random_value, random_id),
                   "Custom Random '%s' already exists !", random_id);
        res = random_generate(random);
        tmpbuf =
            bprintf("%s%d", random_id,
                    atoi(xbt_dynar_getfirst_as(radical_ends, char *)));
        xbt_dict_set(random_value, tmpbuf, bprintf("%f", res), free);
        xbt_free(tmpbuf);
        break;

      case 2:
        start = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 0, char *));
        end = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 1, char *));
        for (i = start; i <= end; i++) {
          xbt_assert(!xbt_dict_get_or_null(random_value, random_id),
                     "Custom Random '%s' already exists !", bprintf("%s%d",
                                                                    random_id,
                                                                    i));
          res = random_generate(random);
          tmpbuf = bprintf("%s%d", random_id, i);
          xbt_dict_set(random_value, tmpbuf, bprintf("%f", res), free);
          xbt_free(tmpbuf);
        }
        break;
      default:
        XBT_INFO("Malformed radical");
        break;
      }
      res = random_generate(random);
      rd_name = bprintf("%s_router", random_id);
      rd_value = bprintf("%f", res);
      xbt_dict_set(random_value, rd_name, rd_value, free);

      xbt_dynar_free(&radical_ends);
    }
    free(rd_name);
    xbt_dynar_free(&radical_elements);
  }
}

void routing_register_callbacks()
{
  sg_platf_host_add_cb(parse_S_host);
  sg_platf_router_add_cb(parse_S_router);

  surfxml_add_callback(STag_surfxml_random_cb_list, &routing_parse_Srandom);

  surfxml_add_callback(STag_surfxml_route_cb_list, &routing_parse_S_route);
  surfxml_add_callback(STag_surfxml_ASroute_cb_list, &routing_parse_S_ASroute);
  surfxml_add_callback(STag_surfxml_bypassRoute_cb_list,
                       &routing_parse_S_bypassRoute);

  surfxml_add_callback(ETag_surfxml_link_ctn_cb_list, &routing_parse_link_ctn);

  surfxml_add_callback(ETag_surfxml_route_cb_list, &routing_parse_E_route);
  surfxml_add_callback(ETag_surfxml_ASroute_cb_list, &routing_parse_E_ASroute);
  surfxml_add_callback(ETag_surfxml_bypassRoute_cb_list,
                       &routing_parse_E_bypassRoute);

  surfxml_add_callback(STag_surfxml_cluster_cb_list, &routing_parse_cluster);

  sg_platf_peer_add_cb(routing_parse_peer);
  sg_platf_postparse_add_cb(routing_parse_postparse);

#ifdef HAVE_TRACING
  instr_routing_define_callbacks();
#endif
}

/**
 * \brief Recursive function for finalize
 *
 * \param rc the source host name
 *
 * This fuction is call by "finalize". It allow to finalize the
 * AS or routing components. It delete all the structures.
 */
static void finalize_rec(AS_t as) {
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  AS_t elem;

  xbt_dict_foreach(as->routing_sons, cursor, key, elem)
  finalize_rec(elem);

  xbt_dict_free(&as->routing_sons);
  xbt_free(as->name);
  as->finalize(as);
}

/** \brief Frees all memory allocated by the routing module */
void routing_exit(void) {
  if (!global_routing)
    return;
  finalize_rec(global_routing->root);
  xbt_dynar_free(&global_routing->last_route);
  xbt_free(global_routing);
}
