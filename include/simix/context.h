/* a fast and simple context switching library                              */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_H
#define _XBT_CONTEXT_H

#include "xbt/swag.h"
#include "simix/datatypes.h"

SG_BEGIN_DECL()
/******************************** Context *************************************/
typedef struct s_smx_context *smx_context_t;
typedef struct s_smx_context_factory *smx_context_factory_t;

/* Process creation/destruction callbacks */
typedef void (*void_pfn_smxprocess_t) (smx_process_t);


/* The following function pointer types describe the interface that any context
   factory should implement */


typedef smx_context_t(*smx_pfn_context_factory_create_context_t)
  (xbt_main_func_t, int, char **, void_pfn_smxprocess_t, void* data);
typedef int (*smx_pfn_context_factory_finalize_t) (smx_context_factory_t*);
typedef void (*smx_pfn_context_free_t) (smx_context_t);
typedef void (*smx_pfn_context_start_t) (smx_context_t);
typedef void (*smx_pfn_context_stop_t) (smx_context_t);
typedef void (*smx_pfn_context_suspend_t) (smx_context_t context);
typedef void (*smx_pfn_context_runall_t) (xbt_dynar_t processes);
typedef smx_context_t (*smx_pfn_context_self_t) (void);
typedef void* (*smx_pfn_context_get_data_t) (smx_context_t context);

/* interface of the context factories */
typedef struct s_smx_context_factory {
  const char *name;
  smx_pfn_context_factory_create_context_t create_context;
  smx_pfn_context_factory_finalize_t finalize;
  smx_pfn_context_free_t free;
  smx_pfn_context_stop_t stop;
  smx_pfn_context_suspend_t suspend;
  smx_pfn_context_runall_t runall;
  smx_pfn_context_self_t self;
  smx_pfn_context_get_data_t get_data;
} s_smx_context_factory_t;



/* Hack: let msg load directly the right factory */
typedef void (*smx_ctx_factory_initializer_t)(smx_context_factory_t*);
extern smx_ctx_factory_initializer_t smx_factory_initializer_to_use;
extern char* smx_context_factory_name;
extern int smx_parallel_contexts;
smx_context_t smx_current_context;

/* *********************** */
/* Context type definition */
/* *********************** */
/* the following function pointers types describe the interface that all context
   concepts must implement */
/* each context type derive from this structure, so they must contain this structure
 * at their begining -- OOP in C :/ */
typedef struct s_smx_context {
  s_xbt_swag_hookup_t hookup;
  xbt_main_func_t code;
  int argc;
  char **argv;
  void_pfn_smxprocess_t cleanup_func;
  int iwannadie:1;
  void *data;   /* Here SIMIX stores the smx_process_t containing the context */
} s_smx_ctx_base_t;

/* methods of this class */
void smx_ctx_base_factory_init(smx_context_factory_t *factory);
int smx_ctx_base_factory_finalize(smx_context_factory_t *factory);

smx_context_t
smx_ctx_base_factory_create_context_sized(size_t size,
                                          xbt_main_func_t code, int argc,
                                          char **argv,
                                          void_pfn_smxprocess_t cleanup,
                                          void* data);
void smx_ctx_base_free(smx_context_t context);
void smx_ctx_base_stop(smx_context_t context);
smx_context_t smx_ctx_base_self(void);
void *smx_ctx_base_get_data(smx_context_t context);

SG_END_DECL()

#endif                          /* !_XBT_CONTEXT_H */
