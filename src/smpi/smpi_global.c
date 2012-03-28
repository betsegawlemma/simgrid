/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "private.h"
#include "smpi_mpi_dt_private.h"
#include "mc/mc.h"
#include "surf/surf.h"

XBT_LOG_NEW_CATEGORY(smpi, "All SMPI categories");

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_kernel, smpi,
                                "Logging specific to SMPI (kernel)");

typedef struct s_smpi_process_data {
  int index;
  int* argc;
  char*** argv;
  smx_rdv_t mailbox;
  xbt_os_timer_t timer;
  double simulated;
  MPI_Comm comm_self;
} s_smpi_process_data_t;

static smpi_process_data_t *process_data = NULL;
static int process_count = 0;

MPI_Comm MPI_COMM_WORLD = MPI_COMM_NULL;

#define MAILBOX_NAME_MAXLEN (5 + sizeof(int) * 2 + 1)

static char* get_mailbox_name(char* str, int index) {
  snprintf(str, MAILBOX_NAME_MAXLEN, "SMPI-%0*x", (int)(sizeof(int) * 2), index);
  return str;
}

void smpi_process_init(int *argc, char ***argv)
{
  int index;
  smpi_process_data_t data;
  smx_process_t proc;

  if(argc && argv) {
    proc = SIMIX_process_self();
    index = atoi((*argv)[1]);
    data = smpi_process_remote_data(index);
    simcall_process_set_data(proc, data);
    if (*argc > 2) {
      free((*argv)[1]);
      memmove(&(*argv)[1], &(*argv)[2], sizeof(char *) * (*argc - 2));
      (*argv)[(*argc) - 1] = NULL;
    }
    (*argc)--;
    data->argc = argc;
    data->argv = argv;
    XBT_DEBUG("<%d> New process in the game: %p", index, proc);
  }
}

void smpi_process_destroy(void)
{
  int index = smpi_process_index();

  XBT_DEBUG("<%d> Process left the game", index);
}

/**
 * @brief Prepares the current process for termination.
 */
void smpi_process_finalize(void)
{
  // wait for all pending asynchronous comms to finish
  while (SIMIX_process_has_pending_comms(SIMIX_process_self())) {
    simcall_process_sleep(1);
  }
}

int smpi_process_argc(void) {
  smpi_process_data_t data = smpi_process_data();

  return data->argc ? *(data->argc) - 1 : 0;
}

int smpi_process_getarg(integer* index, char* dst, ftnlen len) {
  smpi_process_data_t data = smpi_process_data();
  char* arg;
  ftnlen i;

  if(!data->argc || !data->argv
     || *index < 1 || *index >= *(data->argc)) {
    return -1;
  }
  arg = (*data->argv)[*index];
  for(i = 0; i < len && arg[i] != '\0'; i++) {
    dst[i] = arg[i];
  }
  for(; i < len; i++) {
    dst[i] = ' ';
  }
  return 0;
}

int smpi_global_size(void) {
   char* value = getenv("SMPI_GLOBAL_SIZE");

   if(!value) {
      fprintf(stderr, "Please set env var SMPI_GLOBAL_SIZE to expected number of processes.\n");
      abort();
   }
   return atoi(value);
}

smpi_process_data_t smpi_process_data(void)
{
  return SIMIX_process_self_get_data(SIMIX_process_self());
}

smpi_process_data_t smpi_process_remote_data(int index)
{
  return process_data[index];
}

int smpi_process_count(void)
{
  return process_count;
}

int smpi_process_index(void)
{
  smpi_process_data_t data = smpi_process_data();

  return data->index;
}

smx_rdv_t smpi_process_mailbox(void) {
  smpi_process_data_t data = smpi_process_data();

  return data->mailbox;
}

smx_rdv_t smpi_process_remote_mailbox(int index) {
  smpi_process_data_t data = smpi_process_remote_data(index);

  return data->mailbox;
}

xbt_os_timer_t smpi_process_timer(void)
{
  smpi_process_data_t data = smpi_process_data();

  return data->timer;
}

void smpi_process_simulated_start(void)
{
  smpi_process_data_t data = smpi_process_data();

  data->simulated = SIMIX_get_clock();
}

double smpi_process_simulated_elapsed(void)
{
  smpi_process_data_t data = smpi_process_data();

  return SIMIX_get_clock() - data->simulated;
}

MPI_Comm smpi_process_comm_self(void)
{
  smpi_process_data_t data = smpi_process_data();

  return data->comm_self;
}

void print_request(const char *message, MPI_Request request)
{
  XBT_DEBUG("%s  request %p  [buf = %p, size = %zu, src = %d, dst = %d, tag = %d, flags = %x]",
         message, request, request->buf, request->size,
         request->src, request->dst, request->tag, request->flags);
}

void smpi_global_init(void)
{
  int i;
  MPI_Group group;
  char name[MAILBOX_NAME_MAXLEN];

  SIMIX_comm_set_copy_data_callback(&smpi_comm_copy_data_callback);
  process_count = SIMIX_process_count();
  process_data = xbt_new(smpi_process_data_t, process_count);
  for (i = 0; i < process_count; i++) {
    process_data[i] = xbt_new(s_smpi_process_data_t, 1);
    process_data[i]->index = i;
    process_data[i]->argc = NULL;
    process_data[i]->argv = NULL;
    process_data[i]->mailbox = simcall_rdv_create(get_mailbox_name(name, i));
    process_data[i]->timer = xbt_os_timer_new();
    group = smpi_group_new(1);
    process_data[i]->comm_self = smpi_comm_new(group);
    smpi_group_set_mapping(group, i, 0);
  }
  group = smpi_group_new(process_count);
  MPI_COMM_WORLD = smpi_comm_new(group);
  for (i = 0; i < process_count; i++) {
    smpi_group_set_mapping(group, i, i);
  }
}

void smpi_global_destroy(void)
{
  int count = smpi_process_count();
  int i;

  smpi_bench_destroy();
  smpi_comm_destroy(MPI_COMM_WORLD);
  MPI_COMM_WORLD = MPI_COMM_NULL;
  for (i = 0; i < count; i++) {
    smpi_comm_destroy(process_data[i]->comm_self);
    xbt_os_timer_free(process_data[i]->timer);
    simcall_rdv_destroy(process_data[i]->mailbox);
    xbt_free(process_data[i]);
  }
  xbt_free(process_data);
  process_data = NULL;
}

/* Fortran specific stuff */
/* With smpicc, the following weak symbols are used */
/* With smpiff, the following weak symbols are replaced by those in libf2c */
int __attribute__((weak)) xargc;
char** __attribute__((weak)) xargv;

int __attribute__((weak)) main(int argc, char** argv) {
   xargc = argc;
   xargv = argv;
   return MAIN__();
}

int MAIN__(void)
{
  srand(SMPI_RAND_SEED);

  if(getenv("SMPI_PRETEND_CC") != NULL) {
	/* Hack to ensure that smpicc can pretend to be a simple compiler. Particularly handy to pass it to the configuration tools */
    return 0;
  }

  /* Connect log categories.  See xbt/log.c */
  XBT_LOG_CONNECT(smpi_base, smpi);
  XBT_LOG_CONNECT(smpi_bench, smpi);
  XBT_LOG_CONNECT(smpi_coll, smpi);
  XBT_LOG_CONNECT(smpi_comm, smpi);
  XBT_LOG_CONNECT(smpi_group, smpi);
  XBT_LOG_CONNECT(smpi_kernel, smpi);
  XBT_LOG_CONNECT(smpi_mpi, smpi);
  XBT_LOG_CONNECT(smpi_mpi_dt, smpi);
  XBT_LOG_CONNECT(smpi_pmpi, smpi);

#ifdef HAVE_TRACING
  TRACE_global_init(&xargc, xargv);
#endif

  SIMIX_global_init(&xargc, xargv);

#ifdef HAVE_TRACING
  TRACE_start();
#endif

  // parse the platform file: get the host list
  SIMIX_create_environment(xargv[1]);

  SIMIX_function_register_default(smpi_simulated_main);
  SIMIX_launch_application(xargv[2]);

  smpi_global_init();

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_IS_ENABLED)
    MC_modelcheck();
  else
    SIMIX_run();

  if (xbt_cfg_get_int(_surf_cfg_set, "smpi/display_timing"))
    XBT_INFO("Simulation time: %g seconds.", SIMIX_get_clock());

  smpi_global_destroy();

#ifdef HAVE_TRACING
  TRACE_end();
#endif

  SIMIX_clean();
  return 0;
}
