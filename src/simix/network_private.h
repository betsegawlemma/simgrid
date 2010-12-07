/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_NETWORK_PRIVATE_H
#define _SIMIX_NETWORK_PRIVATE_H

#include "simix/datatypes.h"
#include "smurf_private.h"

/** @brief Rendez-vous point datatype */
typedef struct s_smx_rvpoint {
  char *name;
  xbt_fifo_t comm_fifo;
  void *data;
} s_smx_rvpoint_t;

void SIMIX_network_init(void);
void SIMIX_network_exit(void);

#ifdef HAVE_LATENCY_BOUND_TRACKING
XBT_PUBLIC(int) SIMIX_comm_is_latency_bounded(smx_action_t comm);
#endif

smx_rdv_t SIMIX_rdv_create(const char *name);
void SIMIX_rdv_destroy(smx_rdv_t rdv);
smx_rdv_t SIMIX_rdv_get_by_name(const char *name);
int SIMIX_rdv_comm_count_by_host(smx_rdv_t rdv, smx_host_t host);
smx_action_t SIMIX_rdv_get_head(smx_rdv_t rdv);
smx_action_t SIMIX_comm_isend(smx_process_t src_proc, smx_rdv_t rdv,
                              double task_size, double rate,
                              void *src_buff, size_t src_buff_size, void *data);
smx_action_t SIMIX_comm_irecv(smx_process_t dst_proc, smx_rdv_t rdv,
                              void *dst_buff, size_t *dst_buff_size);
void SIMIX_comm_destroy(smx_action_t action);
void SIMIX_comm_destroy_internal_actions(smx_action_t action);
void SIMIX_pre_comm_wait(smx_req_t req);
void SIMIX_pre_comm_waitany(smx_req_t req);
void SIMIX_post_comm(smx_action_t action);
void SIMIX_pre_comm_test(smx_req_t req);
void SIMIX_pre_comm_testany(smx_req_t req);
void SIMIX_comm_cancel(smx_action_t action);
double SIMIX_comm_get_remains(smx_action_t action);
e_smx_state_t SIMIX_comm_get_state(smx_action_t action);
void SIMIX_comm_suspend(smx_action_t action);
void SIMIX_comm_resume(smx_action_t action);
void* SIMIX_comm_get_data(smx_action_t action);
void* SIMIX_comm_get_src_buff(smx_action_t action);
void* SIMIX_comm_get_dst_buff(smx_action_t action);
size_t SIMIX_comm_get_src_buff_size(smx_action_t action);
size_t SIMIX_comm_get_dst_buff_size(smx_action_t action);
smx_process_t SIMIX_comm_get_src_proc(smx_action_t action);
smx_process_t SIMIX_comm_get_dst_proc(smx_action_t action);

#endif

