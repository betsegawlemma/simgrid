/* $Id$ */

/* gras_modinter.h - Interface to GRAS modules                              */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_MODINTER_H
#define GRAS_MODINTER_H

/* modules initialization functions */
void gras_msg_init(void);
void gras_msg_exit(void);
void gras_trp_init(void);
void gras_trp_exit(void);
void gras_datadesc_init(void);
void gras_datadesc_exit(void);

#endif /* GRAS_MODINTER_H */
