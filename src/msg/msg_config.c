/*     $Id$      */

/* Copyright (c) 2002-2007 Arnaud Legrand.                                  */
/* Copyright (c) 2007 Bruno Donassolo.                                      */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "simix/simix.h"

/** \brief set a configuration variable
 * 
 * Currently existing configuation variable:
 *   - workstation_model (string): Model of workstation to use.  
 *     Possible values (defaults to "KCCFLN05"):
 *     - "CLM03": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03]) + support for parallel tasks
 *     - "KCCFLN05": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03]) + failure handling + interference between communications and computations if precised in the platform file.
 *     - "KCCFLN05": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03]) + failure handling + interference between communications and computations if precised in the platform file. Use maxmin for the network.
 *     - "KCCFLN05_proportional": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03]) + failure handling + interference between communications and computations if precised in the platform file. Uses the proportional approahc as described in the Corine Touati's PhD Thesis.
 *     - "KCCFLN05_Vegas": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03]) + failure handling + interference between communications and computations if precised in the platform file. Uses the fairness adapted to the TCP Vegas flow control.
 *     - "KCCFLN05_Reno": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03]) + failure handling + interference between communications and computations if precised in the platform file. Uses the fairness adapted to the TCP Reno flow control.
 * 
 * Example:
 * MSG_config("workstation_model","KCCFLN05");
 */
void MSG_config(const char *name, ...)
{

  if (!msg_global) {
    fprintf(stderr,
	    "ERROR: Please call MSG_init() before using MSG_config()\n");
    abort();
  }


  va_list pa;
  /*  xbt_cfg_dump("msg_cfg_set","",_msg_cfg_set); */
  va_start(pa, name);

  SIMIX_config(name, pa);

  va_end(pa);
  return;
}
