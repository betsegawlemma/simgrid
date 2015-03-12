/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_SMX_H
#define MC_SMX_H

#include <stddef.h>

#include <xbt/log.h>
#include <simgrid/simix.h>

#include "mc_process.h"
#include "mc_protocol.h"

/** @file
 *  @brief (Cross-process, MCer/MCed) Access to SMX structures
 *
 *  We copy some C data structure from the MCed process in the MCer process.
 *  This is implemented by:
 *
 *   - `model_checker->process.smx_process_infos`
 *      (copy of `simix_global->process_list`);
 *
 *   - `model_checker->process.smx_old_process_infos`
 *      (copy of `simix_global->process_to_destroy`);
 *
 *   - `model_checker->hostnames`.
 *
 * The process lists are currently refreshed each time MCed code is executed.
 * We don't try to give a persistent MCer address for a given MCed process.
 * For this reason, a MCer mc_process_t is currently not reusable after
 * MCed code.
 */

SG_BEGIN_DECL()

struct s_mc_smx_process_info {
  /** MCed address of the process */
  void* address;
  /** (Flat) Copy of the process data structure */
  struct s_smx_process copy;
  /** Hostname (owned by `mc_modelchecker->hostnames`) */
  char* hostname;
  char* name;
};

xbt_dynar_t MC_smx_process_info_list_new(void);

void MC_process_smx_refresh(mc_process_t process);

/** Get the issuer of  a simcall (`req->issuer`)
 *
 *  In split-process mode, it does the black magic necessary to get an address
 *  of a (shallow) copy of the data structure the issuer SIMIX process in the local
 *  address space.
 *
 *  @param process the MCed process
 *  @param req     the simcall (copied in the local process)
 */
smx_process_t MC_smx_simcall_get_issuer(smx_simcall_t req);

const char* MC_smx_process_get_name(smx_process_t p);
const char* MC_smx_process_get_host_name(smx_process_t p);

#define MC_EACH_SIMIX_PROCESS(process, code) \
  if (MC_process_is_self(&mc_model_checker->process)) { \
    xbt_swag_foreach(process, simix_global->process_list) { \
      code; \
    } \
  } else { \
    MC_process_smx_refresh(&mc_model_checker->process); \
    unsigned int _smx_process_index; \
    mc_smx_process_info_t _smx_process_info; \
    xbt_dynar_foreach_ptr(mc_model_checker->process.smx_process_infos, _smx_process_index, _smx_process_info) { \
      smx_process_t process = &_smx_process_info->copy; \
      code; \
    } \
  }

/** Execute a given simcall */
void MC_simcall_handle(smx_simcall_t req, int value);



/* ***** Resolve (local/MCer structure from remote/MCed addresses) ***** */

/** Get a local copy of the process from the process remote address */
smx_process_t MC_smx_resolve_process(smx_process_t process_remote_address);

/** Get the process info structure from the process remote address */
mc_smx_process_info_t MC_smx_resolve_process_info(smx_process_t process_remote_address);

SG_END_DECL()

#endif
