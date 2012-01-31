/* gras_modinter.h - How to init/exit the GRAS modules                      */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_MODINTER_H
#define GRAS_MODINTER_H
#include <xbt/misc.h>           /* XBT_PUBLIC */

/* modules initialization functions */
void gras_emul_init(void);
void gras_emul_exit(void);

void gras_msg_register(void);
void gras_msg_init(void);
void gras_msg_exit(void);
void gras_trp_register(void);

void gras_procdata_init(void);
void gras_procdata_exit(void);

#endif                          /* GRAS_MODINTER_H */
