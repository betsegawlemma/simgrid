/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "xbt/module.h"

static xbt_heap_float_t NOW=0;

xbt_dynar_t resource_list = NULL;
tmgr_history_t history = NULL;
lmm_system_t maxmin_system = NULL;

e_surf_action_state_t surf_action_get_state(surf_action_t action)
{
  surf_action_state_t action_state = &(action->resource_type->common_public->states); 
  
  if(action->state_set == action_state->ready_action_set)
    return SURF_ACTION_READY;
  if(action->state_set == action_state->running_action_set)
    return SURF_ACTION_RUNNING;
  if(action->state_set == action_state->failed_action_set)
    return SURF_ACTION_FAILED;
  if(action->state_set == action_state->done_action_set)
    return SURF_ACTION_DONE;
  return SURF_ACTION_NOT_IN_THE_SYSTEM;
}

void surf_action_free(surf_action_t * action)
{
  (*action)->resource_type->common_public->action_cancel(*action);
  xbt_free(*action);
  *action=NULL;
}

void surf_action_change_state(surf_action_t action, e_surf_action_state_t state)
{
  surf_action_state_t action_state = &(action->resource_type->common_public->states); 

  xbt_swag_remove(action, action->state_set);

  if(state == SURF_ACTION_READY) 
    action->state_set = action_state->ready_action_set;
  else if(state == SURF_ACTION_RUNNING)
    action->state_set = action_state->running_action_set;
  else if(state == SURF_ACTION_FAILED)
    action->state_set = action_state->failed_action_set;
  else if(state == SURF_ACTION_DONE)
    action->state_set = action_state->done_action_set;
  else action->state_set = NULL;

  if(action->state_set) xbt_swag_insert(action, action->state_set);
}

void surf_init(int *argc, char **argv)
{
  xbt_init(argc, argv);
  if(!resource_list) resource_list = xbt_dynar_new(sizeof(surf_resource_private_t), NULL);
  if(!history) history = tmgr_history_new();
  if(!maxmin_system) maxmin_system = lmm_system_new();
}

void surf_finalize(void)
{ 
  int i;
  surf_resource_t resource = NULL;

  xbt_dynar_foreach (resource_list,i,resource) {
    resource->common_private->finalize();
  }

  if(maxmin_system) {
    lmm_system_free(maxmin_system);
    maxmin_system = NULL;
  }
  if(history) {
    tmgr_history_free(history);
    history = NULL;
  }
  if(resource_list) xbt_dynar_free(&resource_list);

  tmgr_finalize();
}

xbt_heap_float_t surf_solve(void)
{
  static int first_run = 1;

  xbt_heap_float_t min = -1.0;
  xbt_heap_float_t next_event_date = -1.0;
  xbt_heap_float_t resource_next_action_end = -1.0;
  xbt_maxmin_float_t value = -1.0;
  surf_resource_object_t resource_obj = NULL;
  surf_resource_t resource = NULL;
  tmgr_trace_event_t event = NULL;
  int i;

  if(first_run) {
    while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
      if(next_event_date > NOW) break;
      while ((event = tmgr_history_get_next_event_leq(history, next_event_date,
						      &value, (void **) &resource_obj))) {
	resource_obj->resource->common_private->update_resource_state(resource_obj,
								      event, value);
      }
    }
    xbt_dynar_foreach (resource_list,i,resource) {
      resource->common_private->update_actions_state(NOW, 0.0);
    }
    first_run = 0;
    return 0.0;
  }

  min = -1.0;

  xbt_dynar_foreach (resource_list,i,resource) {
    resource_next_action_end = resource->common_private->share_resources(NOW);
    if(((min<0.0) || (resource_next_action_end<min)) && (resource_next_action_end>=0.0))
      min = resource_next_action_end;
  }

  if(min<0.0) return 0.0;

  while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
    if(next_event_date > NOW+min) break;
    while ((event=tmgr_history_get_next_event_leq(history, next_event_date,
						  &value, (void **) &resource_obj))) {
      if(resource_obj->resource->common_private->resource_used(resource_obj)) {
	min = next_event_date-NOW;
      }
      /* update state of resource_obj according to new value. Does not touch lmm.
         It will be modified if needed when updating actions */
      resource_obj->resource->common_private->update_resource_state(resource_obj,
								    event, value);
    }
  }


  xbt_dynar_foreach (resource_list,i,resource) {
    resource->common_private->update_actions_state(NOW, min);
  }

  NOW=NOW+min;

  return min;
}

xbt_heap_float_t surf_get_clock(void)
{
  return NOW;
}
