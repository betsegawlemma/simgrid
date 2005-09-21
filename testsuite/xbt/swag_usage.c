/* 	$Id$	 */

/* A simple example to demonstrate the use of swags */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include "xbt/swag.h"

typedef struct {
  s_xbt_swag_hookup_t setA;
  s_xbt_swag_hookup_t setB;
  const char *name;
} shmurtz, s_shmurtz_t, *shmurtz_t;

int main(void)
{
  shmurtz_t obj1, obj2, obj;
  xbt_swag_t setA,setB;

  obj1 = calloc(1,sizeof(s_shmurtz_t));
  obj2 = calloc(1,sizeof(s_shmurtz_t));

  obj1->name="Obj 1";
  obj2->name="Obj 2";

  printf("%p %p %ld\n",obj1,&(obj1->setB),
	 (long)((char *)&(obj1->setB) - (char *)obj1));

  setA = xbt_swag_new(xbt_swag_offset(*obj1,setA));
  setB = xbt_swag_new(xbt_swag_offset(*obj1,setB));

  xbt_swag_insert(obj1, setA);
  xbt_swag_insert(obj1, setB);
  xbt_swag_insert(obj2, setA);
  xbt_swag_insert(obj2, setB);

  xbt_swag_remove(obj1, setB);
  /*  xbt_swag_remove(obj2, setB);*/

  xbt_swag_foreach(obj,setA) {
    printf("\t%s\n",obj->name);
  }

  xbt_swag_foreach(obj,setB) {
    printf("\t%s\n",obj->name);
  }

  printf("Belongs : %d\n", xbt_swag_belongs(obj2,setB));

  printf("%d %d\n", xbt_swag_size(setA),xbt_swag_size(setB));
  return 0;
}
