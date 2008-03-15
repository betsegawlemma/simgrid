/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "network_common.h"
#include "surf/random_mgr.h"
#include "xbt/dict.h"
#include "xbt/str.h"
#include "xbt/log.h"

typedef struct network_card_Constant {
  char *name;
  int id;
} s_network_card_Constant_t, *network_card_Constant_t;

typedef struct surf_action_network_Constant {
  s_surf_action_t generic_action;
  double latency;
  double lat_init;
  int suspended;
  network_card_Constant_t src;
  network_card_Constant_t dst;
} s_surf_action_network_Constant_t, *surf_action_network_Constant_t;

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);
static random_data_t random_latency = NULL;
static int card_number = 0;
static int host_number = 0;

static void network_card_free(void *nw_card)
{
  free(((network_card_Constant_t) nw_card)->name);
  free(nw_card);
}

static int network_card_new(const char *card_name)
{
  network_card_Constant_t card =
      xbt_dict_get_or_null(network_card_set, card_name);

  if (!card) {
    card = xbt_new0(s_network_card_Constant_t, 1);
    card->name = xbt_strdup(card_name);
    card->id = card_number++;
    xbt_dict_set(network_card_set, card_name, card, network_card_free);
  }
  return card->id;
}

static int src_id = -1;
static int dst_id = -1;

static void parse_route_set_endpoints(void)
{
  src_id = network_card_new(A_surfxml_route_src);
  dst_id = network_card_new(A_surfxml_route_dst);
  route_action = A_surfxml_route_action;
  route_link_list = xbt_dynar_new(sizeof(char *), &free_string);
}

static void parse_route_set_route(void)
{
  char *name;
  if (src_id != -1 && dst_id != -1) {
    name = bprintf("%x#%x",src_id, dst_id);
    manage_route(route_table, name, route_action, 0);
    free(name);    
  }
}

static void count_hosts(void)
{
   host_number++;
}

static void define_callbacks(const char *file)
{
  /* Figuring out the network links */
  surfxml_add_callback(STag_surfxml_host_cb_list, &count_hosts);
  surfxml_add_callback(STag_surfxml_prop_cb_list, &parse_properties);
  surfxml_add_callback(STag_surfxml_route_cb_list, &parse_route_set_endpoints);
  surfxml_add_callback(ETag_surfxml_link_c_ctn_cb_list, &parse_route_elem);
  surfxml_add_callback(ETag_surfxml_route_cb_list, &parse_route_set_route);
  surfxml_add_callback(STag_surfxml_platform_cb_list, &init_data);
  surfxml_add_callback(STag_surfxml_set_cb_list, &parse_sets);
  surfxml_add_callback(STag_surfxml_route_c_multi_cb_list, &parse_route_multi_set_endpoints);
  surfxml_add_callback(ETag_surfxml_route_c_multi_cb_list, &parse_route_multi_set_route);
  surfxml_add_callback(STag_surfxml_foreach_cb_list, &parse_foreach);
  surfxml_add_callback(STag_surfxml_cluster_cb_list, &parse_cluster);
  surfxml_add_callback(STag_surfxml_trace_cb_list, &parse_trace_init);
  surfxml_add_callback(ETag_surfxml_trace_cb_list, &parse_trace_finalize);
  surfxml_add_callback(STag_surfxml_trace_c_connect_cb_list, &parse_trace_c_connect);
}

static void *name_service(const char *name)
{
  network_card_Constant_t card = xbt_dict_get_or_null(network_card_set, name);
  return card;
}

static const char *get_resource_name(void *resource_id)
{
  return ((network_card_Constant_t) resource_id)->name;
}

static int resource_used(void *resource_id)
{
  return 0;
}

static int action_free(surf_action_t action)
{
  action->using--;
  if (!action->using) {
    xbt_swag_remove(action, action->state_set);
    free(action);
    return 1;
  }
  return 0;
}

static void action_use(surf_action_t action)
{
  action->using++;
}

static void action_cancel(surf_action_t action)
{
  return;
}

static void action_recycle(surf_action_t action)
{
  return;
}

static void action_change_state(surf_action_t action,
				e_surf_action_state_t state)
{
  surf_action_change_state(action, state);
  return;
}

static double share_resources(double now)
{
  surf_action_network_Constant_t action = NULL;
  xbt_swag_t running_actions =
      surf_network_model->common_public->states.running_action_set;
  double min = -1.0;

  xbt_swag_foreach(action, running_actions) {
    if (action->latency > 0) {
      if (min < 0)
	min = action->latency;
      else if (action->latency < min)
	min = action->latency;
    }
  }

  return min;
}

static void update_actions_state(double now, double delta)
{
  surf_action_network_Constant_t action = NULL;
  surf_action_network_Constant_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_network_model->common_public->states.running_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    if (action->latency > 0) {
      if (action->latency > delta) {
	double_update(&(action->latency), delta);
      } else {
	action->latency = 0.0;
      }
    }
    double_update(&(action->generic_action.remains),
		  action->generic_action.cost * delta/action->lat_init);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);

    if (action->generic_action.remains <= 0) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
	       (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } 
  }

  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type,
				  double value)
{
  DIE_IMPOSSIBLE;
}

static surf_action_t communicate(void *src, void *dst, double size,
				 double rate)
{
  surf_action_network_Constant_t action = NULL;
  network_card_Constant_t card_src = src;
  network_card_Constant_t card_dst = dst;

  XBT_IN4("(%s,%s,%g,%g)", card_src->name, card_dst->name, size, rate);

  action = xbt_new0(s_surf_action_network_Constant_t, 1);

  action->generic_action.using = 1;
  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.model_type =
      (surf_model_t) surf_network_model;
  action->suspended = 0;

  action->latency = random_generate(random_latency);
  action->lat_init = action->latency;

  if(action->latency<=0.0)
    action->generic_action.state_set =
      surf_network_model->common_public->states.done_action_set;
  else
    action->generic_action.state_set =
      surf_network_model->common_public->states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);


  XBT_OUT;

  return (surf_action_t) action;
}

/* returns an array of link_Constant_t */
static const void **get_route(void *src, void *dst)
{
  xbt_assert0(0, "Calling this function does not make any sense");
  return (const void **) NULL;
}

static int get_route_size(void *src, void *dst)
{
  xbt_assert0(0, "Calling this function does not make any sense");
  return 0;
}

static const char *get_link_name(const void *link)
{
  DIE_IMPOSSIBLE;
}

static double get_link_bandwidth(const void *link)
{
  DIE_IMPOSSIBLE;
}

static double get_link_latency(const void *link)
{
  DIE_IMPOSSIBLE;
}

static xbt_dict_t get_properties(void *link)
{
  DIE_IMPOSSIBLE;
}

static void action_suspend(surf_action_t action)
{
  ((surf_action_network_Constant_t) action)->suspended = 1;
}

static void action_resume(surf_action_t action)
{
  if (((surf_action_network_Constant_t) action)->suspended)
    ((surf_action_network_Constant_t) action)->suspended = 0;
}

static int action_is_suspended(surf_action_t action)
{
  return ((surf_action_network_Constant_t) action)->suspended;
}

static void action_set_max_duration(surf_action_t action, double duration)
{
  action->max_duration = duration;
}

static void finalize(void)
{
  xbt_dict_free(&network_card_set);
  xbt_swag_free(surf_network_model->common_public->states.
		ready_action_set);
  xbt_swag_free(surf_network_model->common_public->states.
		running_action_set);
  xbt_swag_free(surf_network_model->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_network_model->common_public->states.
		done_action_set);
  free(surf_network_model->common_public);
  free(surf_network_model->common_private);
  free(surf_network_model->extension_public);

  free(surf_network_model);
  surf_network_model = NULL;

  card_number = 0;
}

static void surf_network_model_init_internal(void)
{
  s_surf_action_t action;

  surf_network_model = xbt_new0(s_surf_network_model_t, 1);

  surf_network_model->common_private =
      xbt_new0(s_surf_model_private_t, 1);
  surf_network_model->common_public =
      xbt_new0(s_surf_model_public_t, 1);
  surf_network_model->extension_public =
      xbt_new0(s_surf_network_model_extension_public_t, 1);

  surf_network_model->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_model->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_model->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_model->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_network_model->common_public->name_service = name_service;
  surf_network_model->common_public->get_resource_name =
      get_resource_name;
  surf_network_model->common_public->action_get_state =
      surf_action_get_state;
  surf_network_model->common_public->action_get_start_time =
      surf_action_get_start_time;
  surf_network_model->common_public->action_get_finish_time =
      surf_action_get_finish_time;
  surf_network_model->common_public->action_free = action_free;
  surf_network_model->common_public->action_use = action_use;
  surf_network_model->common_public->action_cancel = action_cancel;
  surf_network_model->common_public->action_recycle = action_recycle;
  surf_network_model->common_public->action_change_state =
      action_change_state;
  surf_network_model->common_public->action_set_data =
      surf_action_set_data;
  surf_network_model->common_public->name = "network";

  surf_network_model->common_private->resource_used = resource_used;
  surf_network_model->common_private->share_resources = share_resources;
  surf_network_model->common_private->update_actions_state =
      update_actions_state;
  surf_network_model->common_private->update_resource_state =
      update_resource_state;
  surf_network_model->common_private->finalize = finalize;

  surf_network_model->common_public->suspend = action_suspend;
  surf_network_model->common_public->resume = action_resume;
  surf_network_model->common_public->is_suspended = action_is_suspended;
  surf_cpu_model->common_public->set_max_duration =
      action_set_max_duration;

  surf_network_model->extension_public->communicate = communicate;
  surf_network_model->extension_public->get_route = get_route;
  surf_network_model->extension_public->get_route_size = get_route_size;
  surf_network_model->extension_public->get_link_name = get_link_name;
  surf_network_model->extension_public->get_link_bandwidth =
      get_link_bandwidth;
  surf_network_model->extension_public->get_link_latency =
      get_link_latency;

  surf_network_model->common_public->get_properties =  get_properties;

  network_card_set = xbt_dict_new();

  if(!random_latency) 
    random_latency = random_new(RAND, 100, 0.0, 1.0, .125, .034);
}

void surf_network_model_init_Constant(const char *filename)
{

  if (surf_network_model)
    return;
  surf_network_model_init_internal();
  define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_network_model);

  update_model_description(surf_network_model_description,
			      surf_network_model_description_size,
			      "Constant",
			      (surf_model_t) surf_network_model);
}
