/* $Id$ */

/* multidict_crash - A crash test for multi-level dictionnaries             */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <assert.h>

#include <gras.h>

#define NB_ELM 10/*00*/
#define MULTICACHE_DEPTH 5
#define KEY_SIZE 12 /*512*/
#define NB_TEST 20
int verbose=0;

static xbt_error_t test1();

static xbt_error_t test1() {
  xbt_error_t errcode;
  xbt_dict_t *head=NULL;
  int i,j,k,l;
  char **key = NULL;
  char **val = NULL;
  void *data;

  printf("\nGeneric multicache: CRASH test:\n");
  printf(" Fill the struct and frees it %d times, using %d elements, depth of multicache=%d\n",NB_TEST,NB_ELM,MULTICACHE_DEPTH);
  printf(" with %d chars long randomized keys. (a point is a test)\n",KEY_SIZE);

  for (i=0;i<NB_TEST;i++) {
    if (i%10) printf("."); else printf("%d",i/10); fflush(stdout);
    if (!(key=malloc(sizeof(char*)*MULTICACHE_DEPTH)))
      RAISE_MALLOC;
    if (!(val=malloc(sizeof(char*)*MULTICACHE_DEPTH)))
      RAISE_MALLOC;
    for (l=0 ; l<MULTICACHE_DEPTH ; l++)
      if (!(val[l]=malloc(sizeof(char)*KEY_SIZE)))
	RAISE_MALLOC;

    for (j=0;j<NB_ELM;j++) {
      if (verbose) printf ("Add ");
      for (l=0 ; l<MULTICACHE_DEPTH ; l++){
	for (k=0;k<KEY_SIZE-1;k++) {
	  val[l][k]=rand() % ('z' - 'a') + 'a';
	}
	val[l][k]='\0';
	if (verbose) printf("%s ; ",val[l]);
	key[l]=val[l];/*  NOWADAYS, no need to strdup the key. */
      }
      if (verbose) printf("in multitree %p.\n",head);
      TRYFAIL(xbt_multidict_set(&head,MULTICACHE_DEPTH,key,
				 strdup(val[0]),&free));

      TRYFAIL(xbt_multidict_get(head,
				 MULTICACHE_DEPTH,(const char **)val,
				 &data));
      if (!data || strcmp((char*)data,val[0])) {
	fprintf(stderr,"Retrieved value (%s) does not match the entrered one (%s)\n",
		(char*)data,val[0]);
	abort();
      }
    }
    xbt_dict_dump(head,&xbt_dict_print);
    xbt_dict_free(&head);

    for (l=0 ; l<MULTICACHE_DEPTH ; l++)
      if (val[l]) free(val[l]);
    free(val);
    free(key);

  }

  printf("\n");
  return no_error;
}

int main(int argc, char *argv[]) {
  xbt_error_t errcode;

  xbt_init(argc,argv,"root.thresh=debug"));
  TRYFAIL(test1());
   
  xbt_exit();
  return 0;
}
