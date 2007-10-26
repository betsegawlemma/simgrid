/* $Id$ */

/* gras/process.h - Manipulating data related to an host.                   */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_PROCESS_H
#define GRAS_PROCESS_H

#include "xbt/misc.h"  /* SG_BEGIN_DECL */
#include "xbt/dict.h"

SG_BEGIN_DECL()

void gras_agent_spawn(const char *name, void *data, xbt_main_func_t code, int argc, char *argv[], xbt_dict_t properties);
  
  
/****************************************************************************/
/* Manipulating User Data                                                   */
/****************************************************************************/

/** \addtogroup GRAS_globals
 *  \brief Handling global variables so that it works on simulator.
 * 
 * In GRAS, using globals is forbidden since the "processes" will
 * sometimes run as a thread inside the same process (namely, in
 * simulation mode). So, you have to put all globals in a structure, and
 * let GRAS handle it.
 * 
 * Use the \ref gras_userdata_new macro to create a new user data (or malloc it
 * and use \ref gras_userdata_set yourself), and \ref gras_userdata_get to
 * retrieve a reference to it. 
 */
/* @{ */

/**
 * \brief Get the data associated with the current process.
 * \ingroup GRAS_globals
 */
XBT_PUBLIC(void*) gras_userdata_get(void);

/**
 * \brief Set the data associated with the current process.
 * \ingroup GRAS_globals
 */
XBT_PUBLIC(void) gras_userdata_set(void *ud);

/** \brief Malloc and set the data associated with the current process. */
#define gras_userdata_new(type) (gras_userdata_set(xbt_new0(type,1)),gras_userdata_get())
/* @} */

SG_END_DECL()

#endif /* GRAS_PROCESS_H */

