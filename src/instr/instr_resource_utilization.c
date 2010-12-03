/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

//to check if variables were previously set to 0, otherwise paje won't simulate them
static xbt_dict_t platform_variables;   /* host or link name -> array of categories */

//B
static xbt_dict_t method_b_dict;

//C
static xbt_dict_t method_c_dict;

//resource utilization tracing method
static void (*TRACE_method_alloc) (void) = NULL;
static void (*TRACE_method_release) (void) = NULL;
static void (*TRACE_method_start) (smx_action_t action) = NULL;
static void (*TRACE_method_event) (smx_action_t action, double now,
                                   double delta, const char *variable,
                                   const char *resource, double value) =
    NULL;
static void (*TRACE_method_end) (smx_action_t action) = NULL;

//used by all methods
static void __TRACE_surf_check_variable_set_to_zero(double now,
                                                    const char *variable,
                                                    const char *resource)
{
  /* check if we have to set it to 0 */
  if (!xbt_dict_get_or_null(platform_variables, resource)) {
    xbt_dynar_t array = xbt_dynar_new(sizeof(char *), xbt_free);
    char *var_cpy = xbt_strdup(variable);
    xbt_dynar_push(array, &var_cpy);
    if (TRACE_platform_is_enabled())
      pajeSetVariable(now, variable, resource, "0");
    xbt_dict_set(platform_variables, resource, array,
                 xbt_dynar_free_voidp);
  } else {
    xbt_dynar_t array = xbt_dict_get(platform_variables, resource);
    unsigned int i;
    char *cat;
    int flag = 0;
    xbt_dynar_foreach(array, i, cat) {
      if (strcmp(variable, cat) == 0) {
        flag = 1;
      }
    }
    if (flag == 0) {
      char *var_cpy = xbt_strdup(variable);
      xbt_dynar_push(array, &var_cpy);
      if (TRACE_platform_is_enabled())
        pajeSetVariable(now, variable, resource, "0");
    }
  }
  /* end of check */
}

#define A_METHOD
//A
static void __TRACE_A_alloc(void)
{
}

static void __TRACE_A_release(void)
{
}

static void __TRACE_A_start(smx_action_t action)
{
}

static void __TRACE_A_event(smx_action_t action, double now, double delta,
                            const char *variable, const char *resource,
                            double value)
{
  if (!TRACE_platform_is_enabled())
    return;

  char valuestr[100];
  snprintf(valuestr, 100, "%f", value);

  __TRACE_surf_check_variable_set_to_zero(now, variable, resource);
  pajeAddVariable(now, variable, resource, valuestr);
  pajeSubVariable(now + delta, variable, resource, valuestr);
}

static void __TRACE_A_end(smx_action_t action)
{
}

#define B_METHOD
//B

static void __TRACE_B_alloc(void)
{
  method_b_dict = xbt_dict_new();
}

static void __TRACE_B_release(void)
{
  if (!TRACE_platform_is_enabled())
    return;

  char *key, *time;
  xbt_dict_cursor_t cursor = NULL;
  xbt_dict_foreach(method_b_dict, cursor, key, time) {
    char resource[INSTR_DEFAULT_STR_SIZE];
    char variable[INSTR_DEFAULT_STR_SIZE];
    char what[INSTR_DEFAULT_STR_SIZE];
    sscanf (key, "%s %s %s", resource, variable, what);
    if (strcmp(what, "time")==0){
      char key_value[INSTR_DEFAULT_STR_SIZE];
      snprintf (key_value, INSTR_DEFAULT_STR_SIZE, "%s %s value", resource, variable);
      char *value = xbt_dict_get_or_null (method_b_dict, key_value);
      pajeSubVariable(atof(time), variable, resource, value);
    }
  }
  xbt_dict_free(&method_b_dict);
}

static void __TRACE_B_start(smx_action_t action)
{
}

static void __TRACE_B_event(smx_action_t action, double now, double delta,
                            const char *variable, const char *resource,
                            double value)
{
  if (!TRACE_platform_is_enabled())
    return;

  char key_time[INSTR_DEFAULT_STR_SIZE];
  char key_value[INSTR_DEFAULT_STR_SIZE];
  char nowstr[INSTR_DEFAULT_STR_SIZE];
  char valuestr[INSTR_DEFAULT_STR_SIZE];
  char nowdeltastr[INSTR_DEFAULT_STR_SIZE];

  snprintf (key_time, INSTR_DEFAULT_STR_SIZE, "%s %s time", resource, variable);
  snprintf (key_value, INSTR_DEFAULT_STR_SIZE, "%s %s value", resource, variable);
  snprintf (nowstr, INSTR_DEFAULT_STR_SIZE, "%f", now);
  snprintf (valuestr, INSTR_DEFAULT_STR_SIZE, "%f", value);
  snprintf (nowdeltastr, INSTR_DEFAULT_STR_SIZE, "%f", now+delta);

  char *lasttimestr = xbt_dict_get_or_null(method_b_dict, key_time);
  char *lastvaluestr = xbt_dict_get_or_null(method_b_dict, key_value);
  if (lasttimestr == NULL){
    __TRACE_surf_check_variable_set_to_zero(now, variable, resource);
    pajeAddVariable(now, variable, resource, valuestr);
    xbt_dict_set(method_b_dict, key_time, xbt_strdup(nowdeltastr), xbt_free);
    xbt_dict_set(method_b_dict, key_value, xbt_strdup(valuestr), xbt_free);
  }else{
    double lasttime = atof (lasttimestr);
    double lastvalue = atof (lastvaluestr);

    if (lastvalue == value){
      double dif = fabs(now - lasttime);
      if (dif < 0.000001){
        //perfect, just go on
      }else{
        //time changed, have to update
        pajeSubVariable(lasttime, variable, resource, lastvaluestr);
        pajeAddVariable(now, variable, resource, valuestr);
      }
    }else{
      //value changed, have to update
      pajeSubVariable(lasttime, variable, resource, lastvaluestr);
      pajeAddVariable(now, variable, resource, valuestr);
    }
    xbt_dict_set(method_b_dict, key_time, xbt_strdup(nowdeltastr), xbt_free);
    xbt_dict_set(method_b_dict, key_value, xbt_strdup(valuestr), xbt_free);
  }
  return;
}

static void __TRACE_B_end(smx_action_t action)
{
}

#define C_METHOD
//C
static void __TRACE_C_alloc(void)
{
  method_c_dict = xbt_dict_new();
}

static void __TRACE_C_release(void)
{
  xbt_dict_free(&method_c_dict);
}

static void __TRACE_C_start(smx_action_t action)
{
  char key[100];
  snprintf(key, 100, "%p", action);

  //check if exists
  if (xbt_dict_get_or_null(method_c_dict, key)) {
    xbt_dict_remove(method_c_dict, key);        //should never execute here, but it does
  }
  xbt_dict_set(method_c_dict, key, xbt_dict_new(), xbt_free);
}

static void __TRACE_C_event(smx_action_t action, double now, double delta,
                            const char *variable, const char *resource,
                            double value)
{
  char key[100];
  snprintf(key, 100, "%p", action);

  xbt_dict_t action_dict = xbt_dict_get(method_c_dict, key);
  //setting start time
  if (!xbt_dict_get_or_null(action_dict, "start")) {
    char start_time[100];
    snprintf(start_time, 100, "%f", now);
    xbt_dict_set(action_dict, "start", xbt_strdup(start_time), xbt_free);
  }
  //updating end time
  char end_time[100];
  snprintf(end_time, 100, "%f", now + delta);
  xbt_dict_set(action_dict, "end", xbt_strdup(end_time), xbt_free);

  //accumulate the value resource-variable
  char res_var[300];
  snprintf(res_var, 300, "%s %s", resource, variable);
  double current_value_f;
  char *current_value = xbt_dict_get_or_null(action_dict, res_var);
  if (current_value) {
    current_value_f = atof(current_value);
    current_value_f += value * delta;
  } else {
    current_value_f = value * delta;
  }
  char new_current_value[100];
  snprintf(new_current_value, 100, "%f", current_value_f);
  xbt_dict_set(action_dict, res_var, xbt_strdup(new_current_value),
               xbt_free);
}

static void __TRACE_C_end(smx_action_t action)
{
  char key[100];
  snprintf(key, 100, "%p", action);

  xbt_dict_t action_dict = xbt_dict_get(method_c_dict, key);
  double start_time = atof(xbt_dict_get(action_dict, "start"));
  double end_time = atof(xbt_dict_get(action_dict, "end"));

  xbt_dict_cursor_t cursor = NULL;
  char *action_dict_key, *action_dict_value;
  xbt_dict_foreach(action_dict, cursor, action_dict_key, action_dict_value) {
    char resource[100], variable[100];
    if (sscanf(action_dict_key, "%s %s", resource, variable) != 2)
      continue;
    __TRACE_surf_check_variable_set_to_zero(start_time, variable,
                                            resource);
    char value_str[100];
    if (end_time - start_time != 0) {
      snprintf(value_str, 100, "%f",
               atof(action_dict_value) / (end_time - start_time));
      pajeAddVariable(start_time, variable, resource, value_str);
      pajeSubVariable(end_time, variable, resource, value_str);
    }
  }
  xbt_dict_remove(method_c_dict, key);
}

#define RESOURCE_UTILIZATION_INTERFACE
/*
 * TRACE_surf_link_set_utilization: entry point from SimGrid
 */
void TRACE_surf_link_set_utilization(void *link, smx_action_t smx_action,
                                     surf_action_t surf_action,
                                     double value, double now,
                                     double delta)
{
  if (!TRACE_is_active())
    return;
  if (!value)
    return;
  //only trace link utilization if link is known by tracing mechanism
  if (!TRACE_surf_link_is_traced(link))
    return;
  if (!value)
    return;

  char resource[100];
  snprintf(resource, 100, "%p", link);

  //trace uncategorized link utilization
  if (TRACE_uncategorized()){
    TRACE_surf_resource_utilization_event(smx_action, now, delta,
                                        "bandwidth_used", resource, value);
  }

  //trace categorized utilization
  if (!IS_TRACED(surf_action))
    return;
  char type[100];
  snprintf(type, 100, "b%s", surf_action->category);
  TRACE_surf_resource_utilization_event(smx_action, now, delta, type,
                                        resource, value);
  return;
}

/*
 * TRACE_surf_host_set_utilization: entry point from SimGrid
 */
void TRACE_surf_host_set_utilization(const char *name,
                                     smx_action_t smx_action,
                                     surf_action_t surf_action,
                                     double value, double now,
                                     double delta)
{
  if (!TRACE_is_active())
    return;
  if (!value)
    return;

  //trace uncategorized host utilization
  if (TRACE_uncategorized()){
    TRACE_surf_resource_utilization_event(smx_action, now, delta,
                                        "power_used", name, value);
  }

  //trace categorized utilization
  if (!IS_TRACED(surf_action))
    return;
  char type[100];
  snprintf(type, 100, "p%s", surf_action->category);
  TRACE_surf_resource_utilization_event(smx_action, now, delta, type, name,
                                        value);
  return;
}

/*
 * __TRACE_surf_resource_utilization_*: entry points from tracing functions
 */
void TRACE_surf_resource_utilization_start(smx_action_t action)
{
  if (!TRACE_is_active())
    return;
  TRACE_method_start(action);
}

void TRACE_surf_resource_utilization_event(smx_action_t action, double now,
                                           double delta,
                                           const char *variable,
                                           const char *resource,
                                           double value)
{
  if (!TRACE_is_active())
    return;
  TRACE_method_event(action, now, delta, variable, resource, value);
}

void TRACE_surf_resource_utilization_end(smx_action_t action)
{
  if (!TRACE_is_active())
    return;
  TRACE_method_end(action);
}

void TRACE_surf_resource_utilization_release()
{
  if (!TRACE_is_active())
    return;
  TRACE_method_release();
}

static void __TRACE_define_method(char *method)
{
  if (!strcmp(method, "a")) {
    TRACE_method_alloc = __TRACE_A_alloc;
    TRACE_method_release = __TRACE_A_release;
    TRACE_method_start = __TRACE_A_start;
    TRACE_method_event = __TRACE_A_event;
    TRACE_method_end = __TRACE_A_end;
  } else if (!strcmp(method, "c")) {
    TRACE_method_alloc = __TRACE_C_alloc;
    TRACE_method_release = __TRACE_C_release;
    TRACE_method_start = __TRACE_C_start;
    TRACE_method_event = __TRACE_C_event;
    TRACE_method_end = __TRACE_C_end;
  } else {                      //default is B
    TRACE_method_alloc = __TRACE_B_alloc;
    TRACE_method_release = __TRACE_B_release;
    TRACE_method_start = __TRACE_B_start;
    TRACE_method_event = __TRACE_B_event;
    TRACE_method_end = __TRACE_B_end;
  }
}

void TRACE_surf_resource_utilization_alloc()
{
  platform_variables = xbt_dict_new();
  __TRACE_define_method(TRACE_get_platform_method());
  TRACE_method_alloc();
}
#endif /* HAVE_TRACING */
