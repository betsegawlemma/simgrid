/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

static xbt_dict_t keys;

static char *TRACE_smpi_container(int rank, char *container, int n)
{
  snprintf(container, n, "rank-%d", rank);
  return container;
}

static char *TRACE_smpi_put_key(int src, int dst, char *key, int n)
{
  //get the dynar for src#dst
  char aux[INSTR_DEFAULT_STR_SIZE];
  snprintf(aux, INSTR_DEFAULT_STR_SIZE, "%d#%d", src, dst);
  xbt_dynar_t d = xbt_dict_get_or_null(keys, aux);
  if (d == NULL) {
    d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
    xbt_dict_set(keys, aux, d, xbt_free);
  }
  //generate the key
  static unsigned long long counter = 0;
  snprintf(key, n, "%d%d%lld", src, dst, counter++);

  //push it
  char *a = (char*)xbt_strdup(key);
  xbt_dynar_push_as(d, char *, a);

  return key;
}

static char *TRACE_smpi_get_key(int src, int dst, char *key, int n)
{
  char aux[INSTR_DEFAULT_STR_SIZE];
  snprintf(aux, INSTR_DEFAULT_STR_SIZE, "%d#%d", src, dst);
  xbt_dynar_t d = xbt_dict_get_or_null(keys, aux);

  int length = xbt_dynar_length(d);
  char *s = xbt_dynar_get_as (d, length-1, char *);
  snprintf (key, n, "%s", s);
  xbt_dynar_remove_at (d, length-1, NULL);
  return key;
}

void TRACE_smpi_alloc()
{
  keys = xbt_dict_new();
}

void TRACE_smpi_start(void)
{
  if (IS_TRACING_SMPI) {
    TRACE_start();
  }
}

void TRACE_smpi_release(void)
{
  TRACE_surf_release();
  if (IS_TRACING_SMPI) {
    TRACE_end();
  }
}

void TRACE_smpi_init(int rank)
{
  if (!IS_TRACING_SMPI)
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  TRACE_smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  if (TRACE_smpi_is_grouped()){
    pajeCreateContainer(SIMIX_get_clock(), str, "MPI_PROCESS",
                      SIMIX_host_self_get_name(), str);
  }else{
    pajeCreateContainer(SIMIX_get_clock(), str, "MPI_PROCESS",
                      "platform", str);
  }
}

void TRACE_smpi_finalize(int rank)
{
  if (!IS_TRACING_SMPI)
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  pajeDestroyContainer(SIMIX_get_clock(), "MPI_PROCESS",
                       TRACE_smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE));
}

void TRACE_smpi_collective_in(int rank, int root, const char *operation)
{
  if (!IS_TRACING_SMPI)
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  pajePushState(SIMIX_get_clock(), "MPI_STATE",
                TRACE_smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE), operation);
}

void TRACE_smpi_collective_out(int rank, int root, const char *operation)
{
  if (!IS_TRACING_SMPI)
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  pajePopState(SIMIX_get_clock(), "MPI_STATE",
               TRACE_smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE));
}

void TRACE_smpi_ptp_in(int rank, int src, int dst, const char *operation)
{
  if (!IS_TRACING_SMPI)
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  pajePushState(SIMIX_get_clock(), "MPI_STATE",
                TRACE_smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE), operation);
}

void TRACE_smpi_ptp_out(int rank, int src, int dst, const char *operation)
{
  if (!IS_TRACING_SMPI)
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  pajePopState(SIMIX_get_clock(), "MPI_STATE",
               TRACE_smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE));
}

void TRACE_smpi_send(int rank, int src, int dst)
{
  if (!IS_TRACING_SMPI)
    return;

  char key[INSTR_DEFAULT_STR_SIZE], str[INSTR_DEFAULT_STR_SIZE];
  TRACE_smpi_put_key(src, dst, key, INSTR_DEFAULT_STR_SIZE);
  pajeStartLink(SIMIX_get_clock(), "MPI_LINK", "0", "PTP",
                TRACE_smpi_container(src, str, INSTR_DEFAULT_STR_SIZE), key);
}

void TRACE_smpi_recv(int rank, int src, int dst)
{
  if (!IS_TRACING_SMPI)
    return;

  char key[INSTR_DEFAULT_STR_SIZE], str[INSTR_DEFAULT_STR_SIZE];
  TRACE_smpi_get_key(src, dst, key, INSTR_DEFAULT_STR_SIZE);
  pajeEndLink(SIMIX_get_clock(), "MPI_LINK", "0", "PTP",
              TRACE_smpi_container(dst, str, INSTR_DEFAULT_STR_SIZE), key);
}
#endif
