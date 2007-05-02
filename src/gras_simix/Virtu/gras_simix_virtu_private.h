/* $Id$ */

/* virtu[alization] - speciafic parts for each OS and for SG                */

/* module's private interface.                                              */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef GRAS_VIRTU_PRIVATE_H
#define GRAS_VIRTU_PRIVATE_H

#include "xbt/dynar.h"
#include "gras_simix/Virtu/gras_simix_virtu_interface.h"
#include "simix/simix.h"

/** @brief Data for each process */
typedef struct {
  /* globals of the process */
  void *userdata;

  /* data specific to each process for each module. 
   * Registered with gras_procdata_add(), retrieved with gras_libdata_get() 
   * This is the old interface, and will disapear before 3.2
   */
  xbt_set_t libdata;
   
  /* data specific to each process for each module. 
   * Registered with gras_module_add(), retrieved with gras_moddata_get() 
   * This is the new interface
   */
  xbt_dynar_t moddata;
} gras_procdata_t;

gras_procdata_t *gras_procdata_get(void);
void *gras_libdata_by_name_from_procdata(const char *name, gras_procdata_t* pd);
   
   
#endif  /* GRAS_VIRTU_PRIVATE_H */
