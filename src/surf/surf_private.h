/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef _SURF_SURF_PRIVATE_H
#define _SURF_SURF_PRIVATE_H

#include "surf/surf.h"
#include "surf/maxmin.h"
#include "surf/trace_mgr.h"
#include "xbt/log.h"
#include "surf/surfxml_parse.h"
#include "surf/random_mgr.h"
#include "instr/instr_private.h"
#include "surf/surfxml_parse_values.h"

#define NO_MAX_DURATION -1.0

/* user-visible parameters */
extern double sg_tcp_gamma;
extern double sg_sender_gap;
extern double sg_latency_factor;
extern double sg_bandwidth_factor;
extern double sg_weight_S_parameter;
extern int sg_maxmin_selective_update;
extern int sg_network_fullduplex;
#ifdef HAVE_GTNETS
extern double sg_gtnets_jitter;
extern int sg_gtnets_jitter_seed;
#endif


extern const char *surf_action_state_names[6];

typedef struct surf_model_private {
  int (*resource_used) (void *resource_id);
  /* Share the resources to the actions and return in how much time
     the next action may terminate */
  double (*share_resources) (double now);
  /* Update the actions' state */
  void (*update_actions_state) (double now, double delta);
  void (*update_resource_state) (void *id, tmgr_trace_event_t event_type,
                                 double value, double time);
  void (*finalize) (void);
} s_surf_model_private_t;

double generic_maxmin_share_resources(xbt_swag_t running_actions,
                                      size_t offset,
                                      lmm_system_t sys,
                                      void (*solve) (lmm_system_t));

/* Generic functions common to all models */
void surf_action_init(void);
void surf_action_exit(void);
e_surf_action_state_t surf_action_state_get(surf_action_t action);      /* cannot declare inline since we use a pointer to it */
double surf_action_get_start_time(surf_action_t action);        /* cannot declare inline since we use a pointer to it */
double surf_action_get_finish_time(surf_action_t action);       /* cannot declare inline since we use a pointer to it */
void surf_action_free(surf_action_t * action);
void surf_action_state_set(surf_action_t action,
                           e_surf_action_state_t state);
void surf_action_data_set(surf_action_t action, void *data);    /* cannot declare inline since we use a pointer to it */
FILE *surf_fopen(const char *name, const char *mode);

extern tmgr_history_t history;
extern xbt_dynar_t surf_path;

void surf_config_init(int *argc, char **argv);
void surf_config_finalize(void);
void surf_config(const char *name, va_list pa);

void net_action_recycle(surf_action_t action);
double net_action_get_remains(surf_action_t action);
#ifdef HAVE_LATENCY_BOUND_TRACKING
int net_get_link_latency_limited(surf_action_t action);
#endif
void net_action_set_max_duration(surf_action_t action, double duration);
/*
 * Returns the initial path. On Windows the initial path is
 * the current directory for the current process in the other
 * case the function returns "./" that represents the current
 * directory on Unix/Linux platforms.
 */
const char *__surf_get_initial_path(void);

/* The __surf_is_absolute_file_path() returns 1 if
 * file_path is a absolute file path, in the other
 * case the function returns 0.
 */
int __surf_is_absolute_file_path(const char *file_path);

/*
 * Link of lenght 1, alongside with its source and destination. This is mainly usefull in the bindings to gtnets and ns3
 */
typedef struct s_onelink {
  char *src;
  char *dst;
  void *link_ptr;
} s_onelink_t, *onelink_t;

/**
 * Routing logic
 */
typedef struct s_as *AS_t;

typedef struct s_model_type {
  const char *name;
  const char *desc;
  AS_t (*create) ();
  void (*end) (AS_t as);
} s_routing_model_description_t, *routing_model_description_t;

typedef struct s_route {
  xbt_dynar_t link_list;
  char *src_gateway;
  char *dst_gateway;
} s_route_t, *route_t;

/* This enum used in the routing structure helps knowing in which situation we are. */
typedef enum {
  SURF_ROUTING_NULL = 0,   /**< Undefined type                                   */
  SURF_ROUTING_BASE,       /**< Base case: use simple link lists for routing     */
  SURF_ROUTING_RECURSIVE   /**< Recursive case: also return gateway informations */
} e_surf_routing_hierarchy_t;

typedef struct s_as {
  xbt_dict_t to_index;			/* char* -> network_element_t */
  xbt_dict_t bypassRoutes;		/* store bypass routes */
  routing_model_description_t model_desc;
  e_surf_routing_hierarchy_t hierarchy;
  char *name;
  struct s_as *routing_father;
  xbt_dict_t routing_sons;

  route_t(*get_route) (AS_t as,
                                const char *src, const char *dst);
  double(*get_latency) (AS_t as,
                        const char *src, const char *dst,
                        route_t e_route);
  xbt_dynar_t(*get_onelink_routes) (AS_t as);
  route_t(*get_bypass_route) (AS_t as, const char *src, const char *dst);
  void (*finalize) (AS_t as);


  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  void (*parse_PU) (AS_t as, const char *name); /* A host or a router, whatever */
  void (*parse_AS) (AS_t as, const char *name);
  void (*parse_route) (AS_t as, const char *src,
                     const char *dst, route_t route);
  void (*parse_ASroute) (AS_t as, const char *src,
                       const char *dst, route_t route);
  void (*parse_bypassroute) (AS_t as, const char *src,
                           const char *dst, route_t e_route);
} s_as_t;

typedef struct s_network_element_info {
  AS_t rc_component;
  e_surf_network_element_type_t rc_type;
} s_network_element_info_t, *network_element_info_t;

typedef int *network_element_t;

struct s_routing_global {
  AS_t root;
  void *loopback;
  size_t size_of_link;
  xbt_dynar_t(*get_route_no_cleanup) (const char *src, const char *dst);
  xbt_dynar_t(*get_onelink_routes) (void);
};

XBT_PUBLIC(void) routing_model_create(size_t size_of_link, void *loopback);
XBT_PUBLIC(void) routing_exit(void);
XBT_PUBLIC(void) routing_register_callbacks(void);

XBT_PUBLIC(xbt_dynar_t) routing_get_route(const char *src, const char *dst);
XBT_PUBLIC(void) routing_get_route_and_latency(const char *src, const char *dst, //FIXME too much functions avail?
                              xbt_dynar_t * route, double *latency, int cleanup);

/**
 * Resource protected methods
 */
static XBT_INLINE xbt_dict_t surf_resource_properties(const void *resource);

XBT_PUBLIC(void) surfxml_bufferstack_push(int new);
XBT_PUBLIC(void) surfxml_bufferstack_pop(int new);

XBT_PUBLIC_DATA(int) surfxml_bufferstack_size;

#endif                          /* _SURF_SURF_PRIVATE_H */
