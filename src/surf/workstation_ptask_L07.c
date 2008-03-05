/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "surf_private.h"
/* extern lmm_system_t maxmin_system; */

typedef enum {
  SURF_WORKSTATION_RESOURCE_CPU,
  SURF_WORKSTATION_RESOURCE_LINK,
} e_surf_workstation_model_type_t;

/**************************************/
/********* cpu object *****************/
/**************************************/
typedef struct cpu_L07 {
  surf_model_t model;	/* Do not move this field: must match model_obj_t */
  xbt_dict_t properties;                /* Do not move this field: must match link_L07_t */
  e_surf_workstation_model_type_t type;	/* Do not move this field: must match link_L07_t */
  char *name;			        /* Do not move this field: must match link_L07_t */
  lmm_constraint_t constraint;	        /* Do not move this field: must match link_L07_t */
  double power_scale;
  double power_current;
  tmgr_trace_event_t power_event;
  e_surf_cpu_state_t state_current;
  tmgr_trace_event_t state_event;
  int id;			/* cpu and network card are a single object... */
} s_cpu_L07_t, *cpu_L07_t;

/**************************************/
/*********** network object ***********/
/**************************************/

typedef struct link_L07 {
  surf_model_t model;	/* Do not move this field: must match model_obj_t */
  xbt_dict_t properties;                /* Do not move this field: must match link_L07_t */
  e_surf_workstation_model_type_t type;	/* Do not move this field: must match cpu_L07_t */
  char *name;	  		        /* Do not move this field: must match cpu_L07_t */
  lmm_constraint_t constraint;	        /* Do not move this field: must match cpu_L07_t */
  double lat_current;
  tmgr_trace_event_t lat_event;
  double bw_current;
  tmgr_trace_event_t bw_event;
  e_surf_link_state_t state_current;
  tmgr_trace_event_t state_event;
} s_link_L07_t, *link_L07_t;


typedef struct s_route_L07 {
  link_L07_t *links;
  int size;
} s_route_L07_t, *route_L07_t;

/**************************************/
/*************** actions **************/
/**************************************/
typedef struct surf_action_workstation_L07 {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
  int workstation_nb;
  cpu_L07_t *workstation_list;
  double *computation_amount;
  double *communication_amount;
  double latency;
  double rate;
  int suspended;
} s_surf_action_workstation_L07_t, *surf_action_workstation_L07_t;


XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_workstation);

static int nb_workstation = 0;
static s_route_L07_t *routing_table = NULL;
#define ROUTE(i,j) routing_table[(i)+(j)*nb_workstation]
static link_L07_t loopback = NULL;
static xbt_dict_t parallel_task_link_set = NULL;
lmm_system_t ptask_maxmin_system = NULL;


static void update_action_bound(surf_action_workstation_L07_t action)
{
  int workstation_nb = action->workstation_nb;
  double lat_current = 0.0;
  double lat_bound = -1.0;
  int i, j, k;
  
  for (i = 0; i < workstation_nb; i++) {
    for (j = 0; j < workstation_nb; j++) {
      cpu_L07_t card_src = action->workstation_list[i];
      cpu_L07_t card_dst = action->workstation_list[j];
      int route_size = ROUTE(card_src->id, card_dst->id).size;
      link_L07_t *route = ROUTE(card_src->id, card_dst->id).links;
      double lat = 0.0;
      
      if (action->communication_amount[i * workstation_nb + j] > 0) {
	for (k = 0; k < route_size; k++) {
	  lat += route[k]->lat_current;
	}
	lat_current=MAX(lat_current,lat*action->communication_amount[i * workstation_nb + j]);
      }
    }
  }
  lat_bound = SG_TCP_CTE_GAMMA / (2.0 * lat_current);
  DEBUG2("action (%p) : lat_bound = %g", action, lat_bound);
  if ((action->latency == 0.0) && (action->suspended == 0)) {
    if (action->rate < 0)
      lmm_update_variable_bound(ptask_maxmin_system, action->variable,
				lat_bound);
    else
      lmm_update_variable_bound(ptask_maxmin_system, action->variable,
				min(action->rate,lat_bound));
  }
}

/**************************************/
/******* Resource Public     **********/
/**************************************/

static void *name_service(const char *name)
{
  return xbt_dict_get_or_null(workstation_set, name);
}

static const char *get_resource_name(void *resource_id)
{
  /* We can freely cast as a cpu_L07_t because it has the same
     prefix as link_L07_t. However, only cpu_L07_t
     will theoretically be given as an argument here. */

  return ((cpu_L07_t) resource_id)->name;
}
static xbt_dict_t get_properties(void *r) {
  /* We can freely cast as a cpu_L07_t since it has the same prefix than link_L07_t */
 return ((cpu_L07_t) r)->properties;
}

/* action_get_state is inherited from the surf module */

static void action_use(surf_action_t action)
{
  action->using++;
  return;
}

static int action_free(surf_action_t action)
{
  action->using--;

  if (!action->using) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_workstation_L07_t) action)->variable)
      lmm_variable_free(ptask_maxmin_system,
			((surf_action_workstation_L07_t) action)->
			variable);
    free(((surf_action_workstation_L07_t)action)->workstation_list);
    free(((surf_action_workstation_L07_t)action)->communication_amount);
    free(((surf_action_workstation_L07_t)action)->computation_amount);
    free(action);
    return 1;
  }
  return 0;
}

static void action_cancel(surf_action_t action)
{
  surf_action_change_state(action, SURF_ACTION_FAILED);
  return;
}

static void action_recycle(surf_action_t action)
{
  DIE_IMPOSSIBLE;
  return;
}

/* action_change_state is inherited from the surf module */
/* action_set_data is inherited from the surf module */

static void action_suspend(surf_action_t action)
{
  XBT_IN1("(%p))", action);
  if (((surf_action_workstation_L07_t) action)->suspended != 2) {
    ((surf_action_workstation_L07_t) action)->suspended = 1;
    lmm_update_variable_weight(ptask_maxmin_system,
			       ((surf_action_workstation_L07_t)
				action)->variable, 0.0);
  }
  XBT_OUT;
}

static void action_resume(surf_action_t action)
{
  surf_action_workstation_L07_t act = (surf_action_workstation_L07_t) action;

  XBT_IN1("(%p)", act);
  if (act->suspended != 2) {
    lmm_update_variable_weight(ptask_maxmin_system,act->variable, 1.0);
    act->suspended = 0;
  }
  XBT_OUT;
}

static int action_is_suspended(surf_action_t action)
{
  return (((surf_action_workstation_L07_t) action)->suspended == 1);
}

static void action_set_max_duration(surf_action_t action, double duration)
{				/* FIXME: should inherit */
  XBT_IN2("(%p,%g)", action, duration);
  action->max_duration = duration;
  XBT_OUT;
}


static void action_set_priority(surf_action_t action, double priority)
{				/* FIXME: should inherit */
  XBT_IN2("(%p,%g)", action, priority);
  action->priority = priority;
  XBT_OUT;
}

/**************************************/
/******* Resource Private    **********/
/**************************************/

static int resource_used(void *resource_id)
{
  /* We can freely cast as a link_L07_t because it has
     the same prefix as cpu_L07_t */
  return lmm_constraint_used(ptask_maxmin_system,
			     ((link_L07_t) resource_id)->
			     constraint);

}

static double share_resources(double now)
{
  s_surf_action_workstation_L07_t s_action;
  surf_action_workstation_L07_t action = NULL;

  xbt_swag_t running_actions =
      surf_workstation_model->common_public->states.running_action_set;
  double min = generic_maxmin_share_resources(running_actions,
					      xbt_swag_offset(s_action,
							      variable),
					      ptask_maxmin_system,
					      bottleneck_solve);

  xbt_swag_foreach(action, running_actions) {
    if (action->latency > 0) {
      if (min < 0) {
	min = action->latency;
	DEBUG3("Updating min (value) with %p (start %f): %f", action,
	       action->generic_action.start, min);
      } else if (action->latency < min) {
	min = action->latency;
	DEBUG3("Updating min (latency) with %p (start %f): %f", action,
	       action->generic_action.start, min);
      }
    }
  }

  DEBUG1("min value : %f", min);

  return min;
}

static void update_actions_state(double now, double delta)
{
  double deltap = 0.0;
  surf_action_workstation_L07_t action = NULL;
  surf_action_workstation_L07_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_workstation_model->common_public->states.running_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    deltap = delta;
    if (action->latency > 0) {
      if (action->latency > deltap) {
	double_update(&(action->latency), deltap);
	deltap = 0.0;
      } else {
	double_update(&(deltap), action->latency);
	action->latency = 0.0;
      }
      if ((action->latency == 0.0) && (action->suspended == 0)) {
	update_action_bound(action);
	lmm_update_variable_weight(ptask_maxmin_system,action->variable, 1.0);
      }
    }
    DEBUG3("Action (%p) : remains (%g) updated by %g.",
	   action, action->generic_action.remains,
	   lmm_variable_getvalue(action->variable) * delta);
    double_update(&(action->generic_action.remains),
		  lmm_variable_getvalue(action->variable) * delta);

    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);

    DEBUG2("Action (%p) : remains (%g).",
	   action, action->generic_action.remains);
    if ((action->generic_action.remains <= 0) &&
	(lmm_get_variable_weight(action->variable) > 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
	       (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      surf_action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else {
      /* Need to check that none of the model has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;
      void *constraint_id = NULL;

      while ((cnst =
	      lmm_get_cnst_from_var(ptask_maxmin_system, action->variable,
				    i++))) {
	constraint_id = lmm_constraint_id(cnst);

/* 	if(((link_L07_t)constraint_id)->type== */
/* 	   SURF_WORKSTATION_RESOURCE_LINK) { */
/* 	  DEBUG2("Checking for link %s (%p)", */
/* 		 ((link_L07_t)constraint_id)->name, */
/* 		 ((link_L07_t)constraint_id)); */
/* 	} */
/* 	if(((cpu_L07_t)constraint_id)->type== */
/* 	   SURF_WORKSTATION_RESOURCE_CPU) { */
/* 	  DEBUG3("Checking for cpu %s (%p) : %s", */
/* 		 ((cpu_L07_t)constraint_id)->name, */
/* 		 ((cpu_L07_t)constraint_id), */
/* 		 ((cpu_L07_t)constraint_id)->state_current==SURF_CPU_OFF?"Off":"On"); */
/* 	} */

	if (((((link_L07_t) constraint_id)->type ==
	      SURF_WORKSTATION_RESOURCE_LINK) &&
	     (((link_L07_t) constraint_id)->state_current ==
	      SURF_LINK_OFF)) ||
	    ((((cpu_L07_t) constraint_id)->type ==
	      SURF_WORKSTATION_RESOURCE_CPU) &&
	     (((cpu_L07_t) constraint_id)->state_current ==
	      SURF_CPU_OFF))) {
	  DEBUG1("Action (%p) Failed!!", action);
	  action->generic_action.finish = surf_get_clock();
	  surf_action_change_state((surf_action_t) action,
				   SURF_ACTION_FAILED);
	  break;
	}
      }
    }
  }
  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type,
				  double value)
{
  cpu_L07_t cpu = id;
  link_L07_t nw_link = id;

  if (nw_link->type == SURF_WORKSTATION_RESOURCE_LINK) {
    DEBUG2("Updating link %s (%p)", nw_link->name, nw_link);
    if (event_type == nw_link->bw_event) {
      nw_link->bw_current = value;
      lmm_update_constraint_bound(ptask_maxmin_system, nw_link->constraint,
				  nw_link->bw_current);
    } else if (event_type == nw_link->lat_event) {
      lmm_variable_t var = NULL;
      surf_action_workstation_L07_t action = NULL;

      nw_link->lat_current = value;
      while (lmm_get_var_from_cnst
	     (ptask_maxmin_system, nw_link->constraint, &var)) {
	

	action = lmm_variable_id(var);
	update_action_bound(action);
      }

    } else if (event_type == nw_link->state_event) {
      if (value > 0)
	nw_link->state_current = SURF_LINK_ON;
      else
	nw_link->state_current = SURF_LINK_OFF;
    } else {
      CRITICAL0("Unknown event ! \n");
      xbt_abort();
    }
    return;
  } else if (cpu->type == SURF_WORKSTATION_RESOURCE_CPU) {
    DEBUG3("Updating cpu %s (%p) with value %g", cpu->name, cpu, value);
    if (event_type == cpu->power_event) {
      cpu->power_current = value;
      lmm_update_constraint_bound(ptask_maxmin_system, cpu->constraint,
				  cpu->power_current * cpu->power_scale);
    } else if (event_type == cpu->state_event) {
      if (value > 0)
	cpu->state_current = SURF_CPU_ON;
      else
	cpu->state_current = SURF_CPU_OFF;
    } else {
      CRITICAL0("Unknown event ! \n");
      xbt_abort();
    }
    return;
  } else {
    DIE_IMPOSSIBLE;
  }
  return;
}

static void finalize(void)
{
  int i, j;

  xbt_dict_free(&link_set);
  xbt_dict_free(&workstation_set);
  if (parallel_task_link_set != NULL) {
    xbt_dict_free(&parallel_task_link_set);
  }
  xbt_swag_free(surf_workstation_model->common_public->states.
		ready_action_set);
  xbt_swag_free(surf_workstation_model->common_public->states.
		running_action_set);
  xbt_swag_free(surf_workstation_model->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_workstation_model->common_public->states.
		done_action_set);

  free(surf_workstation_model->common_public);
  free(surf_workstation_model->common_private);
  free(surf_workstation_model->extension_public);

  free(surf_workstation_model);
  surf_workstation_model = NULL;

  for (i = 0; i < nb_workstation; i++)
    for (j = 0; j < nb_workstation; j++)
      free(ROUTE(i, j).links);
  free(routing_table);
  routing_table = NULL;
  nb_workstation = 0;

  if (ptask_maxmin_system) {
    lmm_system_free(ptask_maxmin_system);
    ptask_maxmin_system = NULL;
  }
}

/**************************************/
/******* Resource Private    **********/
/**************************************/

static e_surf_cpu_state_t resource_get_state(void *cpu)
{
  return ((cpu_L07_t) cpu)->state_current;
}

static double get_speed(void *cpu, double load)
{
  return load * (((cpu_L07_t) cpu)->power_scale);
}

static double get_available_speed(void *cpu)
{
  return ((cpu_L07_t) cpu)->power_current;
}

static surf_action_t execute_parallel_task(int workstation_nb,
					   void **workstation_list,
					   double *computation_amount,
					   double *communication_amount,
					   double amount, double rate)
{
  surf_action_workstation_L07_t action = NULL;
  int i, j, k;
  int nb_link = 0;
  int nb_host = 0;
  double latency = 0.0;

  if (parallel_task_link_set == NULL)
    parallel_task_link_set = xbt_dict_new();

  xbt_dict_reset(parallel_task_link_set);

  /* Compute the number of affected resources... */
  for (i = 0; i < workstation_nb; i++) {
    for (j = 0; j < workstation_nb; j++) {
      cpu_L07_t card_src = workstation_list[i];
      cpu_L07_t card_dst = workstation_list[j];
      int route_size = ROUTE(card_src->id, card_dst->id).size;
      link_L07_t *route = ROUTE(card_src->id, card_dst->id).links;
      double lat = 0.0;

      if (communication_amount[i * workstation_nb + j] > 0)
	for (k = 0; k < route_size; k++) {
	  lat += route[k]->lat_current;
	  xbt_dict_set(parallel_task_link_set, route[k]->name,
		       route[k], NULL);
	}
      latency=MAX(latency,lat);
    }
  }

  nb_link = xbt_dict_length(parallel_task_link_set);
  xbt_dict_reset(parallel_task_link_set);

  for (i = 0; i < workstation_nb; i++)
    if (computation_amount[i] > 0)
      nb_host++;

  action = xbt_new0(s_surf_action_workstation_L07_t, 1);
  DEBUG3("Creating a parallel task (%p) with %d cpus and %d links.",
	 action, workstation_nb, nb_link);
  action->generic_action.using = 1;
  action->generic_action.cost = amount;
  action->generic_action.remains = amount;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.model_type =
      (surf_model_t) surf_workstation_model;
  action->suspended = 0;	/* Should be useless because of the
				   calloc but it seems to help valgrind... */
  action->workstation_nb = workstation_nb;
  action->workstation_list = (cpu_L07_t *)workstation_list;
  action->computation_amount = computation_amount;
  action->communication_amount = communication_amount;
  action->latency = latency;
  action->generic_action.state_set =
      surf_workstation_model->common_public->states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;

  if (action->rate > 0)
    action->variable =
	lmm_variable_new(ptask_maxmin_system, action, 1.0, -1.0,
			 workstation_nb + nb_link);
  else
    action->variable =
	lmm_variable_new(ptask_maxmin_system, action, 1.0, action->rate,
			 workstation_nb + nb_link);

  if (action->latency > 0) 
    lmm_update_variable_weight(ptask_maxmin_system,action->variable,0.0);

  for (i = 0; i < workstation_nb; i++)
    lmm_expand(ptask_maxmin_system,
	       ((cpu_L07_t) workstation_list[i])->constraint,
	       action->variable, computation_amount[i]);
  
  for (i = 0; i < workstation_nb; i++) {
    for (j = 0; j < workstation_nb; j++) {
      cpu_L07_t card_src = workstation_list[i];
      cpu_L07_t card_dst = workstation_list[j];
      int route_size = ROUTE(card_src->id, card_dst->id).size;
      link_L07_t *route = ROUTE(card_src->id, card_dst->id).links;
      
      if (communication_amount[i * workstation_nb + j] == 0.0) 
	continue;
      for (k = 0; k < route_size; k++) {
	  lmm_expand_add(ptask_maxmin_system, route[k]->constraint,
			 action->variable,
			 communication_amount[i * workstation_nb + j]);
      }
    }
  }

  if (nb_link + nb_host == 0) {
    action->generic_action.cost = 1.0;
    action->generic_action.remains = 0.0;
  }

  return (surf_action_t) action;
}

static surf_action_t execute(void *cpu, double size)
{
  void **workstation_list = xbt_new0(void *, 1);
  double *computation_amount = xbt_new0(double, 1);
  double *communication_amount = xbt_new0(double, 1);

  workstation_list[0] = cpu;
  communication_amount[0] = 0.0;
  computation_amount[0] = size;

  return execute_parallel_task(1, workstation_list, computation_amount,
			       communication_amount, 1, -1);
}

static surf_action_t communicate(void *src, void *dst, double size,
				 double rate)
{
  void **workstation_list = xbt_new0(void *, 2);
  double *computation_amount = xbt_new0(double, 2);
  double *communication_amount = xbt_new0(double, 4);
  surf_action_t res = NULL;

  workstation_list[0] = src;
  workstation_list[1] = dst;
  communication_amount[1] = size;

  res = execute_parallel_task(2, workstation_list,
			      computation_amount, communication_amount,
			      1, rate);

  return res;
}

static surf_action_t action_sleep(void *cpu, double duration)
{
  surf_action_workstation_L07_t action = NULL;

  XBT_IN2("(%s,%g)", ((cpu_L07_t) cpu)->name, duration);

  action = (surf_action_workstation_L07_t) execute(cpu, 1.0);
  action->generic_action.max_duration = duration;
  action->suspended = 2;
  lmm_update_variable_weight(ptask_maxmin_system, action->variable, 0.0);

  XBT_OUT;
  return (surf_action_t) action;
}

/* returns an array of link_L07_t */
static const void **get_route(void *src, void *dst)
{
  cpu_L07_t card_src = src;
  cpu_L07_t card_dst = dst;
  route_L07_t route = &(ROUTE(card_src->id, card_dst->id));

  return (const void **) route->links;
}

static int get_route_size(void *src, void *dst)
{
  cpu_L07_t card_src = src;
  cpu_L07_t card_dst = dst;
  route_L07_t route = &(ROUTE(card_src->id, card_dst->id));
  return route->size;
}

static const char *get_link_name(const void *link)
{
  return ((link_L07_t) link)->name;
}

static double get_link_bandwidth(const void *link)
{
  return ((link_L07_t) link)->bw_current;
}

static double get_link_latency(const void *link)
{
  return ((link_L07_t) link)->lat_current;
}


/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/

static void cpu_free(void *cpu)
{
  free(((cpu_L07_t) cpu)->name);
  free(cpu);
}

static cpu_L07_t cpu_new(const char *name, double power_scale,
			 double power_initial,
			 tmgr_trace_t power_trace,
			 e_surf_cpu_state_t state_initial,
			 tmgr_trace_t state_trace,
                         xbt_dict_t cpu_properties)
{
  cpu_L07_t cpu = xbt_new0(s_cpu_L07_t, 1);
  xbt_assert1(! xbt_dict_get_or_null(workstation_set, name),
	      "Host '%s' declared several times in the platform file.",name);

  cpu->model = (surf_model_t) surf_workstation_model;
  cpu->type = SURF_WORKSTATION_RESOURCE_CPU;
  cpu->name = xbt_strdup(name);
  cpu->id = nb_workstation++;

  cpu->power_scale = power_scale;
  xbt_assert0(cpu->power_scale > 0, "Power has to be >0");

  cpu->power_current = power_initial;
  if (power_trace)
    cpu->power_event =
	tmgr_history_add_trace(history, power_trace, 0.0, 0, cpu);

  cpu->state_current = state_initial;
  if (state_trace)
    cpu->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, cpu);

  cpu->constraint =
      lmm_constraint_new(ptask_maxmin_system, cpu,
			 cpu->power_current * cpu->power_scale);

  /*add the property set*/
  cpu->properties =  current_property_set;

  xbt_dict_set(workstation_set, name, cpu, cpu_free);

  return cpu;
}

static void create_routing_table(void)
{
   routing_table = xbt_new0(s_route_L07_t, nb_workstation * nb_workstation);
}

static void parse_cpu_init(void)
{
  double power_scale = 0.0;
  double power_initial = 0.0;
  tmgr_trace_t power_trace = NULL;
  e_surf_cpu_state_t state_initial = SURF_CPU_OFF;
  tmgr_trace_t state_trace = NULL;

  power_scale = get_cpu_power(A_surfxml_host_power);
  surf_parse_get_double(&power_initial, A_surfxml_host_availability);
  surf_parse_get_trace(&power_trace, A_surfxml_host_availability_file);

  xbt_assert0((A_surfxml_host_state == A_surfxml_host_state_ON) ||
	      (A_surfxml_host_state == A_surfxml_host_state_OFF),
	      "Invalid state");
  if (A_surfxml_host_state == A_surfxml_host_state_ON)
    state_initial = SURF_CPU_ON;
  if (A_surfxml_host_state == A_surfxml_host_state_OFF)
    state_initial = SURF_CPU_OFF;
  surf_parse_get_trace(&state_trace, A_surfxml_host_state_file);

  current_property_set = xbt_dict_new();
  cpu_new(A_surfxml_host_id, power_scale, power_initial, power_trace,
	  state_initial, state_trace,current_property_set);
}

static void link_free(void *nw_link)
{
  free(((link_L07_t) nw_link)->name);
  free(nw_link);
}

static link_L07_t link_new(char *name,
			   double bw_initial,
			   tmgr_trace_t bw_trace,
			   double lat_initial,
			   tmgr_trace_t lat_trace,
			   e_surf_link_state_t
			   state_initial,
			   tmgr_trace_t state_trace,
			   e_surf_link_sharing_policy_t
			   policy, xbt_dict_t properties)
{   
  link_L07_t nw_link = xbt_new0(s_link_L07_t, 1);
  xbt_assert1(! xbt_dict_get_or_null(link_set, name),
	      "Link '%s' declared several times in the platform file.",name);

  nw_link->model = (surf_model_t) surf_workstation_model;
  nw_link->type = SURF_WORKSTATION_RESOURCE_LINK;
  nw_link->name = name;
  nw_link->bw_current = bw_initial;
  if (bw_trace)
    nw_link->bw_event =
	tmgr_history_add_trace(history, bw_trace, 0.0, 0, nw_link);
  nw_link->state_current = state_initial;
  nw_link->lat_current = lat_initial;
  if (lat_trace)
    nw_link->lat_event =
	tmgr_history_add_trace(history, lat_trace, 0.0, 0, nw_link);
  if (state_trace)
    nw_link->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, nw_link);

  nw_link->constraint =
      lmm_constraint_new(ptask_maxmin_system, nw_link,
			 nw_link->bw_current);

  if (policy == SURF_LINK_FATPIPE)
    lmm_constraint_shared(nw_link->constraint);

  nw_link->properties = properties;
  
  xbt_dict_set(link_set, name, nw_link, link_free);

  return nw_link;
}

static void parse_link_init(void)
{
  char *name_link;
  double bw_initial;
  tmgr_trace_t bw_trace;
  double lat_initial;
  tmgr_trace_t lat_trace;
  e_surf_link_state_t state_initial_link = SURF_LINK_ON;
  e_surf_link_sharing_policy_t policy_initial_link = SURF_LINK_SHARED;
  tmgr_trace_t state_trace;

  name_link = xbt_strdup(A_surfxml_link_id);
  surf_parse_get_double(&bw_initial, A_surfxml_link_bandwidth);
  surf_parse_get_trace(&bw_trace, A_surfxml_link_bandwidth_file);
  surf_parse_get_double(&lat_initial, A_surfxml_link_latency);
  surf_parse_get_trace(&lat_trace, A_surfxml_link_latency_file);

  xbt_assert0((A_surfxml_link_state ==
	       A_surfxml_link_state_ON)
	      || (A_surfxml_link_state ==
		  A_surfxml_link_state_OFF), "Invalid state");
  if (A_surfxml_link_state == A_surfxml_link_state_ON)
    state_initial_link = SURF_LINK_ON;
  else if (A_surfxml_link_state ==
	   A_surfxml_link_state_OFF)
    state_initial_link = SURF_LINK_OFF;

  if (A_surfxml_link_sharing_policy ==
      A_surfxml_link_sharing_policy_SHARED)
    policy_initial_link = SURF_LINK_SHARED;
  else if (A_surfxml_link_sharing_policy ==
	   A_surfxml_link_sharing_policy_FATPIPE)
    policy_initial_link = SURF_LINK_FATPIPE;

  surf_parse_get_trace(&state_trace, A_surfxml_link_state_file);

  current_property_set = xbt_dict_new();
  link_new(name_link, bw_initial, bw_trace, lat_initial, lat_trace,
		   state_initial_link, state_trace, policy_initial_link, current_property_set);
 }

static void route_new(int src_id, int dst_id,
		      link_L07_t * link_list , int nb_link)
{
  route_L07_t route = &(ROUTE(src_id, dst_id));

  route->size = nb_link;
  route->links = link_list = xbt_realloc(link_list, sizeof(link_L07_t) * nb_link); 
}


static int src_id = -1;
static int dst_id = -1;

static void parse_route_set_endpoints(void)
{
  cpu_L07_t cpu_tmp = NULL;

  cpu_tmp = (cpu_L07_t) name_service(A_surfxml_route_src);
  xbt_assert1(cpu_tmp, "Invalid cpu %s", A_surfxml_route_src);
  if (cpu_tmp != NULL)
    src_id = cpu_tmp->id;

  cpu_tmp = (cpu_L07_t) name_service(A_surfxml_route_dst);
  xbt_assert1(cpu_tmp, "Invalid cpu %s", A_surfxml_route_dst);
  if (cpu_tmp != NULL)
    dst_id = cpu_tmp->id;

  route_action = A_surfxml_route_action;

  route_link_list = xbt_dynar_new(sizeof(char*), &free_string);
}

static void parse_route_set_route(void)
{
  char* name;
  if (src_id != -1 && dst_id != -1) {
     name = bprintf("%x#%x",src_id, dst_id);
     manage_route(route_table, name, route_action, 0);
     free(name);
  }
}

static void add_loopback(void)
{
  int i;

  /* Adding loopback if needed */
  for (i = 0; i < nb_workstation; i++)
    if (!ROUTE(i, i).size) {
      if (!loopback)
	loopback = link_new(xbt_strdup("__MSG_loopback__"),
				    498000000, NULL, 0.000015, NULL,
				    SURF_LINK_ON, NULL,
				    SURF_LINK_FATPIPE,NULL);

      ROUTE(i, i).size = 1;
      ROUTE(i, i).links = xbt_new0(link_L07_t, 1);
      ROUTE(i, i).links[0] = loopback;
    }
}

static void add_route(void)
{
    xbt_ex_t e;
    int nb_link = 0;
    unsigned int cpt = 0;
    int link_list_capacity = 0;
    link_L07_t *link_list = NULL;
    xbt_dict_cursor_t cursor = NULL;
    char *key,*data, *end;
    const char *sep = "#";
    xbt_dynar_t links, keys;
	char* link = NULL;

    if (routing_table == NULL) create_routing_table();

    xbt_dict_foreach(route_table, cursor, key, data) {
       nb_link = 0;
       links = (xbt_dynar_t)data;
       keys = xbt_str_split_str(key, sep);
       
       src_id = strtol(xbt_dynar_get_as(keys, 0, char*), &end, 16);
       dst_id = strtol(xbt_dynar_get_as(keys, 1, char*), &end, 16);

       link_list_capacity = xbt_dynar_length(links);
       link_list = xbt_new(link_L07_t, link_list_capacity);

       
       xbt_dynar_foreach (links, cpt, link) {
         TRY {
           link_list[nb_link++] = xbt_dict_get(link_set, link);
         }
         CATCH(e) {
           RETHROW1("Link %s not found (dict raised this exception: %s)", link);
         }    
       }
       route_new(src_id, dst_id, link_list, nb_link);
   }

   xbt_dict_free(&route_table);
}

static void add_traces(void) {   
   xbt_dict_cursor_t cursor=NULL;
   char *trace_name,*elm;
   
   if (!trace_connect_list_host_avail) return;
 
   /* Connect traces relative to cpu */
   xbt_dict_foreach(trace_connect_list_host_avail, cursor, trace_name, elm) {
      tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
      cpu_L07_t host = xbt_dict_get_or_null(workstation_set, elm);
      
      xbt_assert1(host, "Host %s undefined", elm);
      xbt_assert1(trace, "Trace %s undefined", trace_name);
      
      host->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, host); 
   }

   xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
      tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
      cpu_L07_t host = xbt_dict_get_or_null(workstation_set, elm);
      
      xbt_assert1(host, "Host %s undefined", elm);
      xbt_assert1(trace, "Trace %s undefined", trace_name);
      
      host->power_event = tmgr_history_add_trace(history, trace, 0.0, 0, host); 
   }

   /* Connect traces relative to network */
   xbt_dict_foreach(trace_connect_list_link_avail, cursor, trace_name, elm) {
      tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
      link_L07_t link = xbt_dict_get_or_null(link_set, elm);
      
      xbt_assert1(link, "Link %s undefined", elm);
      xbt_assert1(trace, "Trace %s undefined", trace_name);
      
      link->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
   }

   xbt_dict_foreach(trace_connect_list_bandwidth, cursor, trace_name, elm) {
      tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
      link_L07_t link = xbt_dict_get_or_null(link_set, elm);
      
      xbt_assert1(link, "Link %s undefined", elm);
      xbt_assert1(trace, "Trace %s undefined", trace_name);
      
      link->bw_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
   }
   
   xbt_dict_foreach(trace_connect_list_latency, cursor, trace_name, elm) {
      tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
      link_L07_t link = xbt_dict_get_or_null(link_set, elm);
      
      xbt_assert1(link, "Link %s undefined", elm);
      xbt_assert1(trace, "Trace %s undefined", trace_name);
      
      link->lat_event = tmgr_history_add_trace(history, trace, 0.0, 0, link);
   }
/*
   
   xbt_dynar_foreach (traces_connect_list, cpt, value) {
     trace_connect = xbt_str_split_str(value, "#");
     trace_id        = xbt_dynar_get_as(trace_connect, 0, char*);
     connect_element = atoi(xbt_dynar_get_as(trace_connect, 1, char*)); 
     connect_kind    = atoi(xbt_dynar_get_as(trace_connect, 2, char*));
     connector_id    = xbt_dynar_get_as(trace_connect, 3, char*);

     xbt_assert1((trace = xbt_dict_get_or_null(traces_set_list, trace_id)), "Trace %s undefined", trace_id);

     if (connect_element == A_surfxml_trace_c_connect_element_HOST) {
        xbt_assert1((host = xbt_dict_get_or_null(workstation_set, connector_id)), "Host %s undefined", connector_id);
        switch (connect_kind) {
           case A_surfxml_trace_c_connect_kind_AVAILABILITY: host->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, host); break;
           case A_surfxml_trace_c_connect_kind_POWER: host->power_event = tmgr_history_add_trace(history, trace, 0.0, 0, host); break;
        }
     }
     else {
        xbt_assert1((link = xbt_dict_get_or_null(link_set, connector_id)), "Link %s undefined", connector_id);
        switch (connect_kind) {
           case A_surfxml_trace_c_connect_kind_AVAILABILITY: link->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, link); break;
           case A_surfxml_trace_c_connect_kind_BANDWIDTH: link->bw_event = tmgr_history_add_trace(history, trace, 0.0, 0, link); break;
           case A_surfxml_trace_c_connect_kind_LATENCY: link->lat_event = tmgr_history_add_trace(history, trace, 0.0, 0, link); break;
        }
     }
   }
*/
   xbt_dict_free(&trace_connect_list_host_avail);
   xbt_dict_free(&trace_connect_list_power);
   xbt_dict_free(&trace_connect_list_link_avail);
   xbt_dict_free(&trace_connect_list_bandwidth);
   xbt_dict_free(&trace_connect_list_latency);
   
   xbt_dict_free(&traces_set_list); 
}

static void define_callbacks(const char *file)
{
  /* Adding callback functions */
  surf_parse_reset_parser();
  surfxml_add_callback(STag_surfxml_host_cb_list, &parse_cpu_init);
  surfxml_add_callback(STag_surfxml_prop_cb_list, &parse_properties);
  surfxml_add_callback(STag_surfxml_link_cb_list, &parse_link_init);
  surfxml_add_callback(STag_surfxml_route_cb_list, &parse_route_set_endpoints);
  surfxml_add_callback(ETag_surfxml_link_c_ctn_cb_list, &parse_route_elem);
  surfxml_add_callback(ETag_surfxml_route_cb_list, &parse_route_set_route);
  surfxml_add_callback(STag_surfxml_platform_cb_list, &init_data);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_route);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_loopback);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_traces);
  surfxml_add_callback(STag_surfxml_set_cb_list, &parse_sets);
  surfxml_add_callback(STag_surfxml_route_c_multi_cb_list, &parse_route_multi_set_endpoints);
  surfxml_add_callback(ETag_surfxml_route_c_multi_cb_list, &parse_route_multi_set_route);
  surfxml_add_callback(STag_surfxml_foreach_cb_list, &parse_foreach);
  surfxml_add_callback(STag_surfxml_cluster_cb_list, &parse_cluster);
  surfxml_add_callback(STag_surfxml_trace_cb_list, &parse_trace_init);
  surfxml_add_callback(ETag_surfxml_trace_cb_list, &parse_trace_finalize);
  surfxml_add_callback(STag_surfxml_trace_c_connect_cb_list, &parse_trace_c_connect);
  surfxml_add_callback(STag_surfxml_random_cb_list, &init_randomness);
  surfxml_add_callback(ETag_surfxml_random_cb_list, &add_randomness);
}


/**************************************/
/********* Module  creation ***********/
/**************************************/

static void model_init_internal(void)
{
  s_surf_action_t action;

  surf_workstation_model = xbt_new0(s_surf_workstation_model_t, 1);

  surf_workstation_model->common_private =
      xbt_new0(s_surf_model_private_t, 1);
  surf_workstation_model->common_public =
      xbt_new0(s_surf_model_public_t, 1);
  surf_workstation_model->extension_public =
      xbt_new0(s_surf_workstation_model_extension_public_t, 1);

  surf_workstation_model->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_model->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_model->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_model->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_workstation_model->common_public->name_service = name_service;
  surf_workstation_model->common_public->get_resource_name =
      get_resource_name;
  surf_workstation_model->common_public->action_get_state =
      surf_action_get_state;
  surf_workstation_model->common_public->action_get_start_time =
      surf_action_get_start_time;
  surf_workstation_model->common_public->action_get_finish_time =
      surf_action_get_finish_time;
  surf_workstation_model->common_public->action_use = action_use;
  surf_workstation_model->common_public->action_free = action_free;
  surf_workstation_model->common_public->action_cancel = action_cancel;
  surf_workstation_model->common_public->action_recycle =
      action_recycle;
  surf_workstation_model->common_public->action_change_state =
      surf_action_change_state;
  surf_workstation_model->common_public->action_set_data =
      surf_action_set_data;
  surf_workstation_model->common_public->suspend = action_suspend;
  surf_workstation_model->common_public->resume = action_resume;
  surf_workstation_model->common_public->is_suspended =
      action_is_suspended;
  surf_workstation_model->common_public->set_max_duration =
      action_set_max_duration;
  surf_workstation_model->common_public->set_priority =
      action_set_priority;
  surf_workstation_model->common_public->name = "Workstation ptask_L07";

  surf_workstation_model->common_private->resource_used = resource_used;
  surf_workstation_model->common_private->share_resources =
      share_resources;
  surf_workstation_model->common_private->update_actions_state =
      update_actions_state;
  surf_workstation_model->common_private->update_resource_state =
      update_resource_state;
  surf_workstation_model->common_private->finalize = finalize;

  surf_workstation_model->extension_public->execute = execute;
  surf_workstation_model->extension_public->sleep = action_sleep;
  surf_workstation_model->extension_public->get_state =
      resource_get_state;
  surf_workstation_model->extension_public->get_speed = get_speed;
  surf_workstation_model->extension_public->get_available_speed =
      get_available_speed;
  surf_workstation_model->extension_public->communicate = communicate;
  surf_workstation_model->extension_public->execute_parallel_task =
      execute_parallel_task;
  surf_workstation_model->extension_public->get_route = get_route;
  surf_workstation_model->extension_public->get_route_size =
      get_route_size;
  surf_workstation_model->extension_public->get_link_name =
      get_link_name;
  surf_workstation_model->extension_public->get_link_bandwidth =
      get_link_bandwidth;
  surf_workstation_model->extension_public->get_link_latency =
      get_link_latency;

  surf_workstation_model->common_public->get_properties = get_properties;

  workstation_set = xbt_dict_new();
  link_set = xbt_dict_new();

  if (!ptask_maxmin_system)
    ptask_maxmin_system = lmm_system_new();
}

/**************************************/
/*************** Generic **************/
/**************************************/
void surf_workstation_model_init_ptask_L07(const char *filename)
{
  xbt_assert0(!surf_cpu_model, "CPU model type already defined");
  xbt_assert0(!surf_network_model,
	      "network model type already defined");
  model_init_internal();
  define_callbacks(filename);

  update_model_description(surf_workstation_model_description,
			      surf_workstation_model_description_size,
			      "ptask_L07",
			      (surf_model_t) surf_workstation_model);
  xbt_dynar_push(model_list, &surf_workstation_model);
}
