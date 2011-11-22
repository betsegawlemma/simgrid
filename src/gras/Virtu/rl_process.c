/* process_rl - GRAS process handling on real life                          */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_modinter.h"      /* module initialization interface */
#include "gras/Virtu/virtu_rl.h"
#include "portable.h"
#include "xbt/xbt_os_thread.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_virtu_process);

/* globals */
static gras_procdata_t *_gras_procdata = NULL;
XBT_EXPORT_NO_IMPORT(char const *) _gras_procname = NULL;
static xbt_dict_t _process_properties = NULL;
static xbt_dict_t _host_properties = NULL;

# ifdef __APPLE__
        /* under darwin, the environment gets added to the process at startup time. So, it's not defined at library link time, forcing us to extra tricks */
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
# else
#ifdef _XBT_WIN32
                 /* the environment, as specified by the opengroup, used to initialize the process properties */
extern char **wenviron;
#else
extern char **environ;
#endif
# endif

void gras_process_init()
{
  char **env_iter;
  _gras_procdata = xbt_new0(gras_procdata_t, 1);
  gras_procdata_init();

  /* initialize the host & process properties */
  _host_properties = xbt_dict_new();
  _process_properties = xbt_dict_new();
  env_iter = environ;
  while (*env_iter) {
    char *equal, *buf = xbt_strdup(*env_iter);
    equal = strchr(buf, '=');
    if (!equal) {
      XBT_WARN
          ("The environment contains an entry without '=' char: %s (ignore it)",
           *env_iter);
      continue;
    }
    *equal = '\0';
    xbt_dict_set(_process_properties, buf, xbt_strdup(equal + 1),
                 xbt_free_f);
    free(buf);
    env_iter++;
  }
}

void gras_process_exit()
{
  gras_procdata_exit();
  free(_gras_procdata);
  xbt_dict_free(&_process_properties);
  xbt_dict_free(&_host_properties);
}

const char *xbt_procname(void)
{
  if (_gras_procname)
    return _gras_procname;
  else
    return "";
}

int gras_os_getpid(void)
{
#ifdef _XBT_WIN32
  return (long int) GetCurrentProcessId();
#else
  return (long int) getpid();
#endif
}

/* **************************************************************************
 * Process data
 * **************************************************************************/

gras_procdata_t *gras_procdata_get(void)
{
  xbt_assert(_gras_procdata, "Run gras_process_init (ie, gras_init)!");

  return _gras_procdata;
}

typedef struct {
  xbt_main_func_t code;
  int argc;
  char **argv;
} spawner_wrapper_args;

static void *spawner_wrapper(void *data) {
  spawner_wrapper_args *a = data;
  a->code(a->argc, a->argv);
  free(a);
  return NULL;
}

void gras_agent_spawn(const char *name,
                      xbt_main_func_t code, int argc, char *argv[],
                      xbt_dict_t properties)
{
  spawner_wrapper_args *args =malloc(sizeof(spawner_wrapper_args));
  args->argc=argc;
  args->argv=argv;
  args->code=code;
  xbt_os_thread_create(name,spawner_wrapper,args, NULL);
}

/* **************************************************************************
 * Properties
 * **************************************************************************/

const char *gras_process_property_value(const char *name)
{
  return xbt_dict_get_or_null(_process_properties, name);
}

xbt_dict_t gras_process_properties(void)
{
  return _process_properties;
}

const char *gras_os_host_property_value(const char *name)
{
  return xbt_dict_get_or_null(_host_properties, name);
}

xbt_dict_t gras_os_host_properties(void)
{
  return _host_properties;
}
