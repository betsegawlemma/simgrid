/* $Id$ */

/* dict_crash - A crash test for dictionnaries                              */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define NB_ELM 20000
#define SIZEOFKEY 1024

static void print_str(void *str);
static void print_str(void *str) {
  printf("%s",(char*)str);
}

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging specific to this test");

static xbt_error_t traverse(xbt_dict_t head) {
  xbt_dict_cursor_t cursor=NULL;
  char *key;
  char *data;

  xbt_dict_foreach(head,cursor,key,data) {
    /*    printf("   Seen:  %s=%s\n",key,data); */
    xbt_assert2 (!strcmp(key,data),
      "Key(%s) != value(%s). Abording\n",key,data);
  }
  return no_error;
}

static xbt_error_t countelems(xbt_dict_t head,int*count) {
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  *count=0;

  xbt_dict_foreach(head,cursor,key,data) {
    (*count)++;
  }
  return no_error;
}

int main(int argc,char **argv) {
  xbt_error_t errcode;
  xbt_dict_t head=NULL;
  int i,j,k, nb;
  char *key;
  void *data;

  xbt_init_defaultlog(&argc,argv,"dict.thresh=verbose");
  srand((unsigned int)time(NULL));

  printf("Dictionnary: CRASH test:\n");
  printf(" Fill the struct, count its elems and frees the structure (x20)\n");
  printf(" using 1000 elements with %d chars long randomized keys.\n",SIZEOFKEY);
  printf(" (a point is a test)\n");

  for (i=0;i<20;i++) {
    head=xbt_dict_new();
    if (i%10) printf("."); else printf("%d",i/10); fflush(stdout);
    nb=0;
    for (j=0;j<1000;j++) {
      if (!(key=malloc(SIZEOFKEY))) {
	fprintf(stderr,"Out of memory\n");
	return 1;
      }

      for (k=0;k<SIZEOFKEY-1;k++)
	key[k]=rand() % ('z' - 'a') + 'a';
      key[k]='\0';
      /*      printf("[%d %s]\n",j,key); */
      xbt_dict_set(head,key,key,&free);
    }
    nb=0;
    /*    xbt_dict_dump(head,(void (*)(void*))&printf); */
    TRYFAIL(countelems(head,&nb));
    if (nb != 1000) {
       printf ("\nI found %d elements, and not 1000\n",nb);
       abort();
    }	  
    TRYFAIL(traverse(head));
    xbt_dict_free(&head);
    xbt_dict_free(&head);
  }


  head=xbt_dict_new();
  printf("\n Fill 20 000 elements, with keys being the number of element\n");
  printf("  (a point is 1 000 elements)\n");
  for (j=0;j<NB_ELM;j++) {
    if (!(j%1000)) {
      printf("."); 
      fflush(stdout);
    }
    if (!(key=malloc(10))) {
      fprintf(stderr,"Out of memory\n");
      abort();
    }
    
    sprintf(key,"%d",j);
    xbt_dict_set(head,key,key,&free);
  }

  printf("\n Count the elements (retrieving the key and data for each): \n");
  TRYFAIL(countelems(head,&i));

  printf(" There is %d elements\n",i);
  printf("\n Search my 20 000 elements 20 times. (a point is a test)\n");
  if (!(key=malloc(10))) {
    fprintf(stderr,"Out of memory\n");
    abort();
  } 
  for (i=0;i<20;i++) {
    if (i%10) printf("."); else printf("%d",i/10); fflush(stdout);
    for (j=0;j<NB_ELM;j++) {
      
      sprintf(key,"%d",j);
      TRYFAIL(xbt_dict_get(head,key,&data));
      if (strcmp(key,(char*)data)) {
	printf("key=%s != data=%s\n",key,(char*)data);
	abort();
      }
    }
  }
  free(key);

  printf("\n Remove my 20 000 elements. (a point is 10 000 elements)\n");
  if (!(key=malloc(10))) {
    fprintf(stderr,"Out of memory\n");
    abort();
  }
  for (j=0;j<NB_ELM;j++) {
    if (!(j%10000)) printf("."); fflush(stdout);
    
    sprintf(key,"%d",j);
    TRYFAIL(xbt_dict_remove(head,key));
  }
  printf("\n");
  free(key);

  
  printf("\n Free the structure (twice)\n");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
  
  xbt_exit();
  return 0;
}
