/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "surf/surfxml_parse_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_deployment, simix,
                                "Logging specific to SIMIX (deployment)");
static int parse_argc = -1;
static char **parse_argv = NULL;
static xbt_main_func_t parse_code = NULL;
static char *parse_host = NULL;
static double start_time = 0.0;
static double kill_time = -1.0;

static void parse_process_init(void)
{
  parse_host = xbt_strdup(A_surfxml_process_host);
  xbt_assert(SIMIX_host_get_by_name(parse_host),
              "Host '%s' unknown", parse_host);
  parse_code = SIMIX_get_registered_function(A_surfxml_process_function);
  xbt_assert(parse_code, "Function '%s' unknown",
              A_surfxml_process_function);
  parse_argc = 0;
  parse_argv = NULL;
  parse_argc++;
  parse_argv = xbt_realloc(parse_argv, (parse_argc) * sizeof(char *));
  parse_argv[(parse_argc) - 1] = xbt_strdup(A_surfxml_process_function);
  surf_parse_get_double(&start_time, A_surfxml_process_start_time);
  surf_parse_get_double(&kill_time, A_surfxml_process_kill_time);
}

static void parse_argument(void)
{
  parse_argc++;
  parse_argv = xbt_realloc(parse_argv, (parse_argc) * sizeof(char *));
  parse_argv[(parse_argc) - 1] = xbt_strdup(A_surfxml_argument_value);
}

static void parse_process_finalize(void)
{
  smx_process_arg_t arg = NULL;
  smx_process_t process = NULL;
  if (start_time > SIMIX_get_clock()) {
    arg = xbt_new0(s_smx_process_arg_t, 1);
    arg->name = parse_argv[0];
    arg->code = parse_code;
    arg->data = NULL;
    arg->hostname = parse_host;
    arg->argc = parse_argc;
    arg->argv = parse_argv;
    arg->kill_time = kill_time;
    arg->properties = current_property_set;

    XBT_DEBUG("Process %s(%s) will be started at time %f", arg->name,
           arg->hostname, start_time);
    SIMIX_timer_set(start_time, &SIMIX_process_create_from_wrapper, arg);
  } else {                      // start_time <= SIMIX_get_clock()
    XBT_DEBUG("Starting Process %s(%s) right now", parse_argv[0], parse_host);

    if (simix_global->create_process_function)
      (*simix_global->create_process_function) (&process,
                                                parse_argv[0],
                                                parse_code, NULL,
                                                parse_host, parse_argc,
                                                parse_argv,
                                                current_property_set);
    else
      SIMIX_req_process_create(&process, parse_argv[0], parse_code, NULL, parse_host, parse_argc, parse_argv,
                               current_property_set);
    /* verify if process has been created (won't be the case if the host is currently dead, but that's fine) */
    if (!process) {
      xbt_free(parse_host);
      return;
    }
    if (kill_time > SIMIX_get_clock()) {
      if (simix_global->kill_process_function) {
        SIMIX_timer_set(start_time, simix_global->kill_process_function, process);
      }
    }
    xbt_free(parse_host);
  }
  current_property_set = NULL;
}

/**
 * \brief An application deployer.
 *
 * Creates the process described in \a file.
 * \param file a filename of a xml description of the application. This file
 * follows this DTD :
 *
 *     \include surfxml.dtd
 *
 * Here is a small example of such a platform
 *
 *     \include small_deployment.xml
 *
 */
void SIMIX_launch_application(const char *file)
{
  int parse_status;
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_launch_application.");

  // Reset callbacks
  surf_parse_reset_callbacks();

  surfxml_add_callback(STag_surfxml_process_cb_list, parse_process_init);
  surfxml_add_callback(ETag_surfxml_argument_cb_list, parse_argument);
  surfxml_add_callback(STag_surfxml_prop_cb_list, parse_properties);
  surfxml_add_callback(ETag_surfxml_process_cb_list,
                       parse_process_finalize);

  surf_parse_open(file);
  parse_status = surf_parse();
  surf_parse_close();
  xbt_assert(!parse_status, "Parse error in %s", file);
}

/**
 * \brief Registers a #smx_process_code_t code in a global table.
 *
 * Registers a code function in a global table.
 * This table is then used by #SIMIX_launch_application.
 * \param name the reference name of the function.
 * \param code the function
 */
XBT_INLINE void SIMIX_function_register(const char *name,
                                        xbt_main_func_t code)
{
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_function_register.");

  xbt_dict_set(simix_global->registered_functions, name, code, NULL);
}

static xbt_main_func_t default_function = NULL;
/**
 * \brief Registers a #smx_process_code_t code as default value.
 *
 * Registers a code function as being the default value. This function will get used by SIMIX_launch_application() when there is no registered function of the requested name in.
 * \param code the function
 */
void SIMIX_function_register_default(xbt_main_func_t code)
{
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_function_register.");

  default_function = code;
}

/**
 * \brief Gets a #smx_process_t code from the global table.
 *
 * Gets a code function from the global table. Returns NULL if there are no function registered with the name.
 * This table is then used by #SIMIX_launch_application.
 * \param name the reference name of the function.
 * \return The #smx_process_t or NULL.
 */
xbt_main_func_t SIMIX_get_registered_function(const char *name)
{
  xbt_main_func_t res = NULL;
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_get_registered_function.");

  res = xbt_dict_get_or_null(simix_global->registered_functions, name);
  return res ? res : default_function;
}


/**
 * \brief Bypass the parser, get arguments, and set function to each process
 */

void SIMIX_process_set_function(const char *process_host,
                                const char *process_function,
                                xbt_dynar_t arguments,
                                double process_start_time,
                                double process_kill_time)
{
  unsigned int i;
  char *arg;

  /* init process */
  parse_host = xbt_strdup(process_host);
  xbt_assert(SIMIX_host_get_by_name(parse_host),
              "Host '%s' unknown", parse_host);
  parse_code = SIMIX_get_registered_function(process_function);
  xbt_assert(parse_code, "Function '%s' unknown", process_function);

  parse_argc = 0;
  parse_argv = NULL;
  parse_argc++;
  parse_argv = xbt_realloc(parse_argv, (parse_argc) * sizeof(char *));
  parse_argv[(parse_argc) - 1] = xbt_strdup(process_function);
  start_time = process_start_time;
  kill_time = process_kill_time;

  /* add arguments */
  xbt_dynar_foreach(arguments, i, arg) {
    parse_argc++;
    parse_argv = xbt_realloc(parse_argv, (parse_argc) * sizeof(char *));
    parse_argv[(parse_argc) - 1] = xbt_strdup(arg);
  }

  /* finalize */
  parse_process_finalize();
}
