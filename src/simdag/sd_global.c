/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/dynar.h"
#include "surf/surf.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/config.h"
#include "instr/instr_private.h"
#include "surf/surfxml_parse.h"
#ifdef HAVE_LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif

#ifdef HAVE_JEDULE
#include "instr/jedule/jedule_output.h"
#endif

XBT_LOG_NEW_CATEGORY(sd, "Logging specific to SimDag");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_kernel, sd,
                                "Logging specific to SimDag (kernel)");

SD_global_t sd_global = NULL;

XBT_LOG_EXTERNAL_CATEGORY(sd_kernel);
XBT_LOG_EXTERNAL_CATEGORY(sd_task);
XBT_LOG_EXTERNAL_CATEGORY(sd_workstation);

/**
 * \brief Initialises SD internal data
 *
 * This function must be called before any other SD function. Then you
 * should call SD_create_environment().
 *
 * \param argc argument number
 * \param argv argument list
 * \see SD_create_environment(), SD_exit()
 */
void SD_init(int *argc, char **argv)
{
#ifdef HAVE_TRACING
  TRACE_global_init(argc, argv);
#endif

  s_SD_task_t task;

  xbt_assert0(!SD_INITIALISED(), "SD_init() already called");

  /* Connect our log channels: that must be done manually under windows */
  XBT_LOG_CONNECT(sd_kernel, sd);
  XBT_LOG_CONNECT(sd_task, sd);
  XBT_LOG_CONNECT(sd_workstation, sd);


  sd_global = xbt_new(s_SD_global_t, 1);
  sd_global->workstations = xbt_dict_new();
  sd_global->workstation_count = 0;
  sd_global->workstation_list = NULL;
  sd_global->links = xbt_dict_new();
  sd_global->link_count = 0;
  sd_global->link_list = NULL;
  sd_global->recyclable_route = NULL;
  sd_global->watch_point_reached = 0;

  sd_global->not_scheduled_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->schedulable_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->scheduled_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->runnable_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->in_fifo_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->running_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->done_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->failed_task_set =
      xbt_swag_new(xbt_swag_offset(task, state_hookup));
  sd_global->task_number = 0;

  surf_init(argc, argv);

  xbt_cfg_setdefault_string(_surf_cfg_set, "workstation/model",
                            "ptask_L07");

#ifdef HAVE_TRACING
  TRACE_start ();
#endif

#ifdef HAVE_JEDULE
  init_jedule_output();
#endif
}

/**
 * \brief Reinits the application part of the simulation (experimental feature)
 *
 * This function allows you to run several simulations on the same platform
 * by resetting the part describing the application.
 *
 * @warning: this function is still experimental and not perfect. For example,
 * the simulation clock (and traces usage) is not reset. So, do not use it if
 * you use traces in your simulation, and do not use absolute timing after using it.
 * That being said, this function is still precious if you want to compare a bunch of
 * heuristics on the same platforms.
 */
void SD_application_reinit(void)
{

  s_SD_task_t task;

  if (SD_INITIALISED()) {
    DEBUG0("Recreating the swags...");
    xbt_swag_free(sd_global->not_scheduled_task_set);
    xbt_swag_free(sd_global->schedulable_task_set);
    xbt_swag_free(sd_global->scheduled_task_set);
    xbt_swag_free(sd_global->runnable_task_set);
    xbt_swag_free(sd_global->in_fifo_task_set);
    xbt_swag_free(sd_global->running_task_set);
    xbt_swag_free(sd_global->done_task_set);
    xbt_swag_free(sd_global->failed_task_set);

    sd_global->not_scheduled_task_set =
        xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->schedulable_task_set =
        xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->scheduled_task_set =
        xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->runnable_task_set =
        xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->in_fifo_task_set =
        xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->running_task_set =
        xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->done_task_set =
        xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->failed_task_set =
        xbt_swag_new(xbt_swag_offset(task, state_hookup));
    sd_global->task_number = 0;

#ifdef HAVE_JEDULE
    cleanup_jedule();
    init_jedule_output();
#endif

  } else {
    WARN0("SD_application_reinit called before initialization of SimDag");
    /* we cannot use exceptions here because xbt is not running! */
  }

}

/**
 * \brief Creates the environment
 *
 * The environment (i.e. the \ref SD_workstation_management "workstations" and the
 * \ref SD_link_management "links") is created with the data stored in the given XML
 * platform file.
 *
 * \param platform_file name of an XML file describing the environment to create
 * \see SD_workstation_management, SD_link_management
 *
 * The XML file follows this DTD:
 *
 *     \include simgrid.dtd
 *
 * Here is a small example of such a platform:
 *
 *     \include small_platform.xml
 */
void SD_create_environment(const char *platform_file)
{
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *surf_workstation = NULL;
  void *surf_link = NULL;

  platform_filename = bprintf("%s",platform_file);

  // Reset callbacks
  surf_parse_reset_callbacks();
  // Add config callbacks
  surf_parse_add_callback_config();
  SD_CHECK_INIT_DONE();
  parse_platform_file(platform_file);
  surf_config_models_create_elms();

  /* now let's create the SD wrappers for workstations and links */
  xbt_dict_foreach(surf_model_resource_set(surf_workstation_model), cursor,
                   name, surf_workstation) {
    __SD_workstation_create(surf_workstation, NULL);
  }

  xbt_dict_foreach(surf_model_resource_set(surf_network_model), cursor,
                   name, surf_link) {
    __SD_link_create(surf_link, NULL);
  }

  DEBUG2("Workstation number: %d, link number: %d",
         SD_workstation_get_number(), SD_link_get_number());
#ifdef HAVE_JEDULE
  jedule_setup_platform();
#endif
}

/**
 * \brief Launches the simulation.
 *
 * The function will execute the \ref SD_RUNNABLE runnable tasks.
 * If \a how_long is positive, then the simulation will be stopped either
 * when time reaches \a how_long or when a watch point is reached.
 * A nonpositive value for \a how_long means no time limit, in which case
 * the simulation will be stopped either when a watch point is reached or
 * when no more task can be executed.
 * Then you can call SD_simulate() again.
 *
 * \param how_long maximum duration of the simulation (a negative value means no time limit)
 * \return a NULL-terminated array of \ref SD_task_t whose state has changed.
 * \see SD_task_schedule(), SD_task_watch()
 */
xbt_dynar_t SD_simulate(double how_long)
{
  double total_time = 0.0;      /* we stop the simulation when total_time >= how_long */
  double elapsed_time = 0.0;
  SD_task_t task, task_safe, dst;
  SD_dependency_t dependency;
  surf_action_t action;
  xbt_dynar_t changed_tasks = xbt_dynar_new(sizeof(SD_task_t), NULL);
  unsigned int iter, depcnt;
  static int first_time = 1;

  SD_CHECK_INIT_DONE();

   if (first_time) {
    VERB0("Starting simulation...");

    surf_presolve();            /* Takes traces into account */
    first_time = 0;
  }

  sd_global->watch_point_reached = 0;

  /* explore the runnable tasks */
  xbt_swag_foreach_safe(task, task_safe, sd_global->runnable_task_set) {
    VERB1("Executing task '%s'", SD_task_get_name(task));
    if (__SD_task_try_to_run(task)
        && !xbt_dynar_member(changed_tasks, &task))
      xbt_dynar_push(changed_tasks, &task);
  }

  /* main loop */
  elapsed_time = 0.0;
  while (elapsed_time >= 0.0 &&
         (how_long < 0.0 || total_time < how_long) &&
         !sd_global->watch_point_reached) {
    surf_model_t model = NULL;
    /* dumb variables */


    DEBUG1("Total time: %f", total_time);

    elapsed_time = surf_solve(how_long > 0 ? surf_get_clock() + how_long : -1.0);
    DEBUG1("surf_solve() returns %f", elapsed_time);
    if (elapsed_time > 0.0)
      total_time += elapsed_time;

    /* let's see which tasks are done */
    xbt_dynar_foreach(model_list, iter, model) {
      while ((action = xbt_swag_extract(model->states.done_action_set))) {
        task = action->data;
        task->start_time =
            surf_workstation_model->
            action_get_start_time(task->surf_action);
        task->finish_time = surf_get_clock();
        VERB1("Task '%s' done", SD_task_get_name(task));
        DEBUG0("Calling __SD_task_just_done");
        __SD_task_just_done(task);
        DEBUG1("__SD_task_just_done called on task '%s'",
               SD_task_get_name(task));

        /* the state has changed */
        if (!xbt_dynar_member(changed_tasks, &task))
          xbt_dynar_push(changed_tasks, &task);

        /* remove the dependencies after this task */
        xbt_dynar_foreach(task->tasks_after, depcnt, dependency) {
          dst = dependency->dst;
          if (dst->unsatisfied_dependencies > 0)
            dst->unsatisfied_dependencies--;
          if (dst->is_not_ready > 0)
            dst->is_not_ready--;

          if (!(dst->unsatisfied_dependencies)) {
            if (__SD_task_is_scheduled(dst))
              __SD_task_set_state(dst, SD_RUNNABLE);
            else
              __SD_task_set_state(dst, SD_SCHEDULABLE);
          }

          if (SD_task_get_kind(dst) == SD_TASK_COMM_E2E) {
            SD_dependency_t comm_dep;
            SD_task_t comm_dst;
            xbt_dynar_get_cpy(dst->tasks_after, 0, &comm_dep);
            comm_dst = comm_dep->dst;
            if (__SD_task_is_not_scheduled(comm_dst) &&
                comm_dst->is_not_ready > 0) {
              comm_dst->is_not_ready--;

              if (!(comm_dst->is_not_ready)) {
                __SD_task_set_state(comm_dst, SD_SCHEDULABLE);
              }
            }
          }

          /* is dst runnable now? */
          if (__SD_task_is_runnable(dst)
              && !sd_global->watch_point_reached) {
            VERB1("Executing task '%s'", SD_task_get_name(dst));
            if (__SD_task_try_to_run(dst) &&
                !xbt_dynar_member(changed_tasks, &task))
              xbt_dynar_push(changed_tasks, &task);
          }
        }
      }

      /* let's see which tasks have just failed */
      while ((action = xbt_swag_extract(model->states.failed_action_set))) {
        task = action->data;
        task->start_time =
            surf_workstation_model->
            action_get_start_time(task->surf_action);
        task->finish_time = surf_get_clock();
        VERB1("Task '%s' failed", SD_task_get_name(task));
        __SD_task_set_state(task, SD_FAILED);
        surf_workstation_model->action_unref(action);
        task->surf_action = NULL;

        if (!xbt_dynar_member(changed_tasks, &task))
          xbt_dynar_push(changed_tasks, &task);
      }
    }
  }

  if (!sd_global->watch_point_reached && how_long<0){
    if (xbt_swag_size(sd_global->done_task_set) < sd_global->task_number){
    	WARN0("Simulation is finished but some tasks are still not done");
      	xbt_swag_foreach_safe (task, task_safe,sd_global->not_scheduled_task_set){
       		WARN1("%s is in SD_NOT_SCHEDULED state", SD_task_get_name(task));
		}
    	xbt_swag_foreach_safe (task, task_safe,sd_global->schedulable_task_set){
   		WARN1("%s is in SD_SCHEDULABLE state", SD_task_get_name(task));
	}
      	xbt_swag_foreach_safe (task, task_safe,sd_global->scheduled_task_set){
       		WARN1("%s is in SD_SCHEDULED state", SD_task_get_name(task));
       	}
    }
  }

  DEBUG3("elapsed_time = %f, total_time = %f, watch_point_reached = %d",
         elapsed_time, total_time, sd_global->watch_point_reached);
  DEBUG1("current time = %f", surf_get_clock());

  return changed_tasks;
}

/**
 * \brief Returns the current clock
 *
 * \return the current clock, in second
 */
double SD_get_clock(void)
{
  SD_CHECK_INIT_DONE();

  return surf_get_clock();
}

/**
 * \brief Destroys all SD internal data
 *
 * This function should be called when the simulation is over. Don't forget also to destroy
 * the tasks.
 *
 * \see SD_init(), SD_task_destroy()
 */
void SD_exit(void)
{
#ifdef HAVE_TRACING
  TRACE_surf_release();
#endif
  if (SD_INITIALISED()) {
    DEBUG0("Destroying workstation and link dictionaries...");
    xbt_dict_free(&sd_global->workstations);
    xbt_dict_free(&sd_global->links);

    DEBUG0("Destroying workstation and link arrays if necessary...");
    if (sd_global->workstation_list != NULL)
      xbt_free(sd_global->workstation_list);

    if (sd_global->link_list != NULL)
      xbt_free(sd_global->link_list);

    if (sd_global->recyclable_route != NULL)
      xbt_free(sd_global->recyclable_route);

    DEBUG0("Destroying the swags...");
    xbt_swag_free(sd_global->not_scheduled_task_set);
    xbt_swag_free(sd_global->schedulable_task_set);
    xbt_swag_free(sd_global->scheduled_task_set);
    xbt_swag_free(sd_global->runnable_task_set);
    xbt_swag_free(sd_global->in_fifo_task_set);
    xbt_swag_free(sd_global->running_task_set);
    xbt_swag_free(sd_global->done_task_set);
    xbt_swag_free(sd_global->failed_task_set);

    xbt_free(sd_global);
    sd_global = NULL;

#ifdef HAVE_TRACING
  TRACE_end();
#endif
#ifdef HAVE_JEDULE
  cleanup_jedule();
#endif

    DEBUG0("Exiting Surf...");
    surf_exit();
  } else {
    WARN0("SD_exit() called, but SimDag is not running");
    /* we cannot use exceptions here because xbt is not running! */
  }
}

/**
 * \bried load script file
 */

void SD_load_environment_script(const char *script_file)
{
#ifdef HAVE_LUA
  lua_State *L = lua_open();
  luaL_openlibs(L);

  if (luaL_loadfile(L, script_file) || lua_pcall(L, 0, 0, 0)) {
    printf("error: %s\n", lua_tostring(L, -1));
    return;
  }
#else
  xbt_die
      ("Lua is not available!! to call SD_load_environment_script, lua should be available...");
#endif
  return;
}
