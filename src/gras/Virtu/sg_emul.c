/* sg_emul - Emulation support (simulation)                                 */

/* Copyright (c) 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>              /* sprintf */
#include "gras/emul.h"
#include "gras/Virtu/virtu_sg.h"
#include "gras_modinter.h"

#include "xbt/xbt_os_time.h"    /* timers */
#include "xbt/dict.h"
#include "xbt/ex.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_virtu_emul, gras_virtu,
                                "Emulation support");
/*** CPU burning */
void gras_cpu_burn(double flops)
{
  smx_action_t execution;

  if (flops > 0){
    execution = SIMIX_req_host_execute("task", SIMIX_host_self(), flops, 1);
    SIMIX_req_host_execution_wait(execution);
  }
}

/*** Timing macros ***/
static xbt_os_timer_t timer;
static int benchmarking = 0;
static xbt_dict_t benchmark_set = NULL;
static double reference = .00000000523066250047108838;  /* FIXME: we should benchmark host machine to set this; unit=s/flop */
static double duration = 0.0;

static char *locbuf = NULL;
static unsigned int locbufsize;

void gras_emul_init(void)
{
  if (!benchmark_set) {
    benchmark_set = xbt_dict_new();
    timer = xbt_os_timer_new();
  }
}

void gras_emul_exit(void)
{
  free(locbuf);
  xbt_dict_free(&benchmark_set);
  xbt_os_timer_free(timer);
}


static void store_in_dict(xbt_dict_t dict, const char *key, double value)
{
  double *ir;

  ir = xbt_dict_get_or_null(dict, key);
  if (!ir) {
    ir = xbt_new0(double, 1);
    xbt_dict_set(dict, key, ir, xbt_free_f);
  }
  *ir = value;
}

static double get_from_dict(xbt_dict_t dict, const char *key)
{
  double *ir = xbt_dict_get(dict, key);

  return *ir;
}

int gras_bench_always_begin(const char *location, int line)
{
  xbt_assert(!benchmarking, "Already benchmarking");
  benchmarking = 1;

  if (!timer)
    xbt_os_timer_start(timer);
  return 0;
}

int gras_bench_always_end(void)
{
  xbt_assert(benchmarking, "Not benchmarking yet");
  benchmarking = 0;
  xbt_os_timer_stop(timer);
  duration = xbt_os_timer_elapsed(timer);

  gras_cpu_burn(duration / reference);

  return 0;
}

int gras_bench_once_begin(const char *location, int line)
{
  double *ir = NULL;
  xbt_assert(!benchmarking, "Already benchmarking");
  benchmarking = 1;

  if (!locbuf || locbufsize < strlen(location) + 64) {
    locbufsize = strlen(location) + 64;
    locbuf = xbt_realloc(locbuf, locbufsize);
  }
  sprintf(locbuf, "%s:%d", location, line);

  ir = xbt_dict_get_or_null(benchmark_set, locbuf);
  if (!ir) {
    XBT_DEBUG("%s", locbuf);
    duration = 1;
    xbt_os_timer_start(timer);
    return 1;
  } else {
    duration = -1.0;
    return 0;
  }
}

int gras_bench_once_end(void)
{
  xbt_assert(benchmarking, "Not benchmarking yet");
  benchmarking = 0;
  if (duration > 0) {
    xbt_os_timer_stop(timer);
    duration = xbt_os_timer_elapsed(timer);
    store_in_dict(benchmark_set, locbuf, duration);
  } else {
    duration = get_from_dict(benchmark_set, locbuf);
  }
  XBT_DEBUG("Simulate the run of a task of %f sec for %s", duration, locbuf);
  gras_cpu_burn(duration / reference);
  return 0;
}


/*** Conditional execution support ***/

int gras_if_RL(void)
{
  return 0;
}

int gras_if_SG(void)
{
  return 1;
}
