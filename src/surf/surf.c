/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <ctype.h>

#include "surf_private.h"
#include "xbt/module.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_kernel, surf,
				"Logging specific to SURF (kernel)");

int use_sdp_solver = 0;
int use_lagrange_solver = 0;

/* Additional declarations for Windows potability. */

#ifndef MAX_DRIVE
#define MAX_DRIVE 26
#endif

#ifdef _WIN32
#include <windows.h>
static const char *disk_drives_letter_table[MAX_DRIVE] = {
  "A:\\",
  "B:\\",
  "C:\\",
  "D:\\",
  "E:\\",
  "F:\\",
  "G:\\",
  "H:\\",
  "I:\\",
  "J:\\",
  "K:\\",
  "L:\\",
  "M:\\",
  "N:\\",
  "O:\\",
  "P:\\",
  "Q:\\",
  "R:\\",
  "S:\\",
  "T:\\",
  "U:\\",
  "V:\\",
  "W:\\",
  "X:\\",
  "Y:\\",
  "Z:\\"
};
#endif				/* #ifdef _WIN32 */

/*
 * Returns the initial path. On Windows the initial path is
 * the current directory for the current process in the other
 * case the function returns "./" that represents the current
 * directory on Unix/Linux platforms.
 */

const char *__surf_get_initial_path(void)
{

#ifdef _WIN32
  unsigned i;
  char current_directory[MAX_PATH + 1] = { 0 };
  unsigned int len = GetCurrentDirectory(MAX_PATH + 1, current_directory);
  char root[4] = { 0 };

  if (!len)
    return NULL;

  strncpy(root, current_directory, 3);

  for (i = 0; i < MAX_DRIVE; i++) {
    if (toupper(root[0]) == disk_drives_letter_table[i][0])
      return disk_drives_letter_table[i];
  }

  return NULL;
#else
  return "./";
#endif
}

/* The __surf_is_absolute_file_path() returns 1 if
 * file_path is a absolute file path, in the other
 * case the function returns 0.
 */
int __surf_is_absolute_file_path(const char *file_path)
{
#ifdef _WIN32
  WIN32_FIND_DATA wfd = { 0 };
  HANDLE hFile = FindFirstFile(file_path, &wfd);

  if (INVALID_HANDLE_VALUE == hFile)
    return 0;

  FindClose(hFile);
  return 1;
#else
  return (file_path[0] == '/');
#endif
}

typedef struct surf_model_object {
  surf_model_t model;
} s_surf_model_object_t, *surf_model_object_t;

static double NOW = 0;

xbt_dynar_t model_list = NULL;
tmgr_history_t history = NULL;
lmm_system_t maxmin_system = NULL;
xbt_dynar_t surf_path = NULL;
const char *surf_action_state_names[6] = {
  "SURF_ACTION_READY",
  "SURF_ACTION_RUNNING",
  "SURF_ACTION_FAILED",
  "SURF_ACTION_DONE",
  "SURF_ACTION_TO_FREE",
  "SURF_ACTION_NOT_IN_THE_SYSTEM"
};

s_surf_model_description_t surf_network_model_description[surf_network_model_description_size] = {
  {"CM02", NULL, surf_network_model_init_CM02},
#ifdef HAVE_GTNETS
  {"GTNets", NULL, surf_network_model_init_GTNETS},
#endif
#ifdef HAVE_SDP
  {"SDP", NULL, surf_network_model_init_SDP},
#endif
  {"Reno", NULL, surf_network_model_init_Reno},
  {"Vegas", NULL, surf_network_model_init_Vegas}
};

s_surf_model_description_t surf_cpu_model_description[surf_cpu_model_description_size] = {
  {"Cas01", NULL, surf_cpu_model_init_Cas01},
};

s_surf_model_description_t surf_workstation_model_description[surf_workstation_model_description_size] = {
  {"CLM03", NULL, surf_workstation_model_init_CLM03, create_workstations},
  {"compound", NULL, surf_workstation_model_init_compound, NULL},
  {"ptask_L07", NULL, surf_workstation_model_init_ptask_L07, NULL}
};

void update_model_description(s_surf_model_description_t * table,
				 int table_size,
				 const char *name,
				 surf_model_t model)
{
  int i = find_model_description(table, table_size, name);
  table[i].model = model;
}

int find_model_description(s_surf_model_description_t * table,
			      int table_size, const char *name)
{
  int i;
  char *name_list = NULL;

  for (i = 0; i < table_size; i++)
    if (!strcmp(name, table[i].name)) {
      return i;
    }
  name_list = strdup(table[0].name);
  for (i = 1; i < table_size; i++) {
    name_list =
	xbt_realloc(name_list,
		    strlen(name_list) + strlen(table[i].name) + 2);
    strcat(name_list, ", ");
    strcat(name_list, table[i].name);
  }
  xbt_assert2(0, "Model '%s' is invalid! Valid models are: %s.", name,
	      name_list);
}

double generic_maxmin_share_resources(xbt_swag_t running_actions,
				      size_t offset,
				      lmm_system_t sys,
				      void (*solve) (lmm_system_t))
{
  surf_action_t action = NULL;
  double min = -1;
  double value = -1;
#define VARIABLE(action) (*((lmm_variable_t*)(((char *) (action)) + (offset))))

  xbt_assert0(solve, "Give me a real solver function!");
  solve(sys);

  xbt_swag_foreach(action, running_actions) {
    value = lmm_variable_getvalue(VARIABLE(action));
    if ((value > 0) || (action->max_duration >= 0))
      break;
  }

  if (!action)
    return -1.0;

  if (value > 0) {
    if(action->remains>0) 
      min = action->remains / value;
    else 
      min = 0.0;
    if ((action->max_duration >= 0) && (action->max_duration < min))
      min = action->max_duration;
  } else
    min = action->max_duration;

  DEBUG5("Found action (%p: duration = %f, remains = %f, value = %f) ! %f",
	 action, action->max_duration, action->remains, value, min);

  for (action = xbt_swag_getNext(action, running_actions->offset);
       action;
       action = xbt_swag_getNext(action, running_actions->offset)) {
    value = lmm_variable_getvalue(VARIABLE(action));
    if (value > 0) {
      if(action->remains>0) 
	value = action->remains / value;
      else 
	value = 0.0;
      if (value < min) {
	min = value;
	DEBUG2("Updating min (value) with %p: %f", action, min);
      }
    }
    if ((action->max_duration >= 0) && (action->max_duration < min)) {
      min = action->max_duration;
      DEBUG2("Updating min (duration) with %p: %f", action, min);
    }
  }
  DEBUG1("min value : %f", min);

#undef VARIABLE
  return min;
}

e_surf_action_state_t surf_action_get_state(surf_action_t action)
{
  surf_action_state_t action_state =
      &(action->model_type->common_public->states);

  if (action->state_set == action_state->ready_action_set)
    return SURF_ACTION_READY;
  if (action->state_set == action_state->running_action_set)
    return SURF_ACTION_RUNNING;
  if (action->state_set == action_state->failed_action_set)
    return SURF_ACTION_FAILED;
  if (action->state_set == action_state->done_action_set)
    return SURF_ACTION_DONE;
  return SURF_ACTION_NOT_IN_THE_SYSTEM;
}

double surf_action_get_start_time(surf_action_t action)
{
  return action->start;
}

double surf_action_get_finish_time(surf_action_t action)
{
  return action->finish;
}

void surf_action_free(surf_action_t * action)
{
  (*action)->model_type->common_public->action_cancel(*action);
  free(*action);
  *action = NULL;
}

void surf_action_change_state(surf_action_t action,
			      e_surf_action_state_t state)
{
  surf_action_state_t action_state =
      &(action->model_type->common_public->states);
  XBT_IN2("(%p,%s)", action, surf_action_state_names[state]);
  xbt_swag_remove(action, action->state_set);

  if (state == SURF_ACTION_READY)
    action->state_set = action_state->ready_action_set;
  else if (state == SURF_ACTION_RUNNING)
    action->state_set = action_state->running_action_set;
  else if (state == SURF_ACTION_FAILED)
    action->state_set = action_state->failed_action_set;
  else if (state == SURF_ACTION_DONE)
    action->state_set = action_state->done_action_set;
  else
    action->state_set = NULL;

  if (action->state_set)
    xbt_swag_insert(action, action->state_set);
  XBT_OUT;
}

void surf_action_set_data(surf_action_t action, void *data)
{
  action->data = data;
}

void surf_init(int *argc, char **argv)
{
  int i, j;
  char *opt;

  const char *initial_path;

  /* Connect our log channels: that must be done manually under windows */
  XBT_LOG_CONNECT(surf_cpu, surf);
  XBT_LOG_CONNECT(surf_kernel, surf);
  XBT_LOG_CONNECT(surf_lagrange_dichotomy, surf_lagrange);
  XBT_LOG_CONNECT(surf_lagrange, surf);
  XBT_LOG_CONNECT(surf_maxmin, surf);
  XBT_LOG_CONNECT(surf_network_gtnets, surf);
  XBT_LOG_CONNECT(surf_network, surf);
  XBT_LOG_CONNECT(surf_parse, surf);
  XBT_LOG_CONNECT(surf_sdp_out, surf);
  XBT_LOG_CONNECT(surf_sdp, surf);
  XBT_LOG_CONNECT(surf_timer, surf);
  XBT_LOG_CONNECT(surf_workstation, surf);
   
  xbt_init(argc, argv);
  if (!surf_path) {

    /* retrieves the current directory of the current process */
    initial_path = __surf_get_initial_path();

    xbt_assert0((initial_path),
		"__surf_get_initial_path() failed! Can't resolves current Windows directory");

    surf_path = xbt_dynar_new(sizeof(char *), NULL);
    xbt_dynar_push(surf_path, &initial_path);

    for (i = 1; i < *argc; i++) {
      if (!strncmp(argv[i], "--surf-path=", strlen("--surf-path="))) {
	opt = strchr(argv[i], '=');
	opt++;
	xbt_dynar_push(surf_path, &opt);
	/*remove this from argv */
	for (j = i + 1; j < *argc; j++) {
	  argv[j - 1] = argv[j];
	}
	argv[j - 1] = NULL;
	(*argc)--;
	i--;			/* compensate effect of next loop incrementation */
      }
    }
  }
  if (!model_list)
    model_list = xbt_dynar_new(sizeof(surf_model_private_t), NULL);
  if (!history)
    history = tmgr_history_new();
}

static char *path_name = NULL;
FILE *surf_fopen(const char *name, const char *mode)
{
  unsigned int iter;
  char *path = NULL;
  FILE *file = NULL;
  unsigned int path_name_len = 0;	/* don't count '\0' */

  xbt_assert0(name, "Need a non-NULL file name");

  xbt_assert0(surf_path,
	      "surf_init has to be called before using surf_fopen");

  if (__surf_is_absolute_file_path(name)) {	/* don't mess with absolute file names */
    return fopen(name, mode);

  } else {			/* search relative files in the path */

    if (!path_name) {
      path_name_len = strlen(name);
      path_name = xbt_new0(char, path_name_len + 1);
    }

    xbt_dynar_foreach(surf_path, iter, path) {
      if (path_name_len < strlen(path) + strlen(name) + 1) {
	path_name_len = strlen(path) + strlen(name) + 1;	/* plus '/' */
	path_name = xbt_realloc(path_name, path_name_len + 1);
      }
      #ifdef WIN32
      sprintf(path_name, "%s\\%s", path, name);
      #else
      sprintf(path_name, "%s/%s", path, name);
      #endif
      file = fopen(path_name, mode);
      if (file)
	return file;
    }
  }
  return file;
}

void surf_exit(void)
{
  unsigned int iter;
  surf_model_t model = NULL;

  xbt_dynar_foreach(model_list, iter, model) {
    model->common_private->finalize();
  }

  if (maxmin_system) {
    lmm_system_free(maxmin_system);
    maxmin_system = NULL;
  }
  if (history) {
    tmgr_history_free(history);
    history = NULL;
  }
  if (model_list)
    xbt_dynar_free(&model_list);

  if (surf_path)
    xbt_dynar_free(&surf_path);

  tmgr_finalize();
  surf_parse_lex_destroy();
  if (path_name) {
    free(path_name);
    path_name = NULL;
  }
  surf_parse_free_callbacks();
  xbt_dict_free(&route_table);
  NOW=0; /* Just in case the user plans to restart the simulation afterward */
  xbt_exit();
}

double surf_solve(void)
{
  static int first_run = 1;

  double min = -1.0;
  double next_event_date = -1.0;
  double model_next_action_end = -1.0;
  double value = -1.0;
  surf_model_object_t model_obj = NULL;
  surf_model_t model = NULL;
  tmgr_trace_event_t event = NULL;
  unsigned int iter;

  if (first_run) {
    DEBUG0
	("First Run! Let's \"purge\" events and put models in the right state");
    while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
      if (next_event_date > NOW)
	break;
      while ((event =
	      tmgr_history_get_next_event_leq(history, next_event_date,
					      &value,
					      (void **) &model_obj))) {
	model_obj->model->common_private->
	    update_resource_state(model_obj, event, value);
      }
    }
    xbt_dynar_foreach(model_list, iter, model) {
      model->common_private->update_actions_state(NOW, 0.0);
    }
    first_run = 0;
    return 0.0;
  }

  min = -1.0;

  DEBUG0("Looking for next action end");
  xbt_dynar_foreach(model_list, iter, model) {
    DEBUG1("Running for Resource [%s]", model->common_public->name);
    model_next_action_end =
	model->common_private->share_resources(NOW);
    DEBUG2("Resource [%s] : next action end = %f",
	   model->common_public->name, model_next_action_end);
    if (((min < 0.0) || (model_next_action_end < min))
	&& (model_next_action_end >= 0.0))
      min = model_next_action_end;
  }
  DEBUG1("Next action end : %f", min);

  if (min < 0.0)
    return -1.0;

  DEBUG0("Looking for next event");
  while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
    DEBUG1("Next event : %f", next_event_date);
    if (next_event_date > NOW + min)
      break;
    DEBUG0("Updating models");
    while ((event =
	    tmgr_history_get_next_event_leq(history, next_event_date,
					    &value,
					    (void **) &model_obj))) {
      if (model_obj->model->common_private->
	  resource_used(model_obj)) {
	min = next_event_date - NOW;
	DEBUG1
	    ("This event will modify model state. Next event set to %f",
	     min);
      }
      /* update state of model_obj according to new value. Does not touch lmm.
         It will be modified if needed when updating actions */
      model_obj->model->common_private->
	  update_resource_state(model_obj, event, value);
    }
  }

  DEBUG1("Duration set to %f", min);

  NOW = NOW + min;

  xbt_dynar_foreach(model_list, iter, model) {
    model->common_private->update_actions_state(NOW, min);
  }

  return min;
}

double surf_get_clock(void)
{
  return NOW;
}
