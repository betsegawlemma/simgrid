/* context_sysv - context switching with ucontextes from System V           */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

 /* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_context_sysv_private.h"
#include "xbt/threadpool.h"
#include "simix/private.h"

#ifdef HAVE_VALGRIND_VALGRIND_H
#  include <valgrind/valgrind.h>
#endif                          /* HAVE_VALGRIND_VALGRIND_H */

#ifdef _XBT_WIN32
#include "win32_ucontext.h"
#include "win32_ucontext.c"
#else
#include "ucontext.h"
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

static xbt_tpool_t tpool; 

static smx_context_t
smx_ctx_sysv_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func, void* data);


static void smx_ctx_sysv_wrapper(smx_ctx_sysv_t context);

void SIMIX_ctx_sysv_factory_init(smx_context_factory_t *factory)
{
  smx_ctx_base_factory_init(factory);

  (*factory)->finalize = smx_ctx_sysv_factory_finalize;
  (*factory)->create_context = smx_ctx_sysv_create_context;
  /* Do not overload that method (*factory)->finalize */
  (*factory)->free = smx_ctx_sysv_free;
  (*factory)->stop = smx_ctx_sysv_stop;
  (*factory)->suspend = smx_ctx_sysv_suspend;
  (*factory)->name = "smx_sysv_context_factory";

  if(_surf_parallel_contexts){
    tpool = xbt_tpool_new(2, 10);
    (*factory)->runall = smx_ctx_sysv_runall_parallel;
  }else{
    (*factory)->runall = smx_ctx_sysv_runall;
  }    
}

int smx_ctx_sysv_factory_finalize(smx_context_factory_t *factory)
{ 
  if(tpool)
    xbt_tpool_destroy(tpool);
  return smx_ctx_base_factory_finalize(factory);
}

smx_context_t
smx_ctx_sysv_create_context_sized(size_t size, xbt_main_func_t code,
                                  int argc, char **argv,
                                  void_pfn_smxprocess_t cleanup_func,
                                  void *data)
{

  smx_ctx_sysv_t context =
      (smx_ctx_sysv_t) smx_ctx_base_factory_create_context_sized(size,
                                                                 code,
                                                                 argc,
                                                                 argv,
                                                                 cleanup_func,
                                                                 data);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if (code) {

    xbt_assert2(getcontext(&(context->uc)) == 0,
                "Error in context saving: %d (%s)", errno,
                strerror(errno));

    context->uc.uc_link = NULL;

    context->uc.uc_stack.ss_sp =
        pth_skaddr_makecontext(context->stack, CONTEXT_STACK_SIZE);

    context->uc.uc_stack.ss_size =
        pth_sksize_makecontext(context->stack, CONTEXT_STACK_SIZE);

#ifdef HAVE_VALGRIND_VALGRIND_H
    context->valgrind_stack_id =
        VALGRIND_STACK_REGISTER(context->uc.uc_stack.ss_sp,
                                ((char *) context->uc.uc_stack.ss_sp) +
                                context->uc.uc_stack.ss_size);
#endif                          /* HAVE_VALGRIND_VALGRIND_H */

    makecontext(&((smx_ctx_sysv_t) context)->uc, (void (*)())smx_ctx_sysv_wrapper,
                sizeof(void*)/sizeof(int), context);
  }

  return (smx_context_t) context;

}

static smx_context_t
smx_ctx_sysv_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func,
    void *data)
{

  return smx_ctx_sysv_create_context_sized(sizeof(s_smx_ctx_sysv_t),
                                           code, argc, argv, cleanup_func,
                                           data);

}

void smx_ctx_sysv_free(smx_context_t context)
{

  if (context) {

#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_STACK_DEREGISTER(((smx_ctx_sysv_t)
                               context)->valgrind_stack_id);
#endif                          /* HAVE_VALGRIND_VALGRIND_H */

  }
  smx_ctx_base_free(context);
}

void smx_ctx_sysv_stop(smx_context_t context)
{
  smx_ctx_base_stop(context);

  smx_ctx_sysv_suspend(context);
}

void smx_ctx_sysv_wrapper(smx_ctx_sysv_t context)
{ 
  (context->super.code) (context->super.argc, context->super.argv);

  smx_ctx_sysv_stop((smx_context_t) context);
}

void smx_ctx_sysv_suspend(smx_context_t context)
{
  ucontext_t maestro_ctx =
      ((smx_ctx_sysv_t) simix_global->maestro_process->context)->uc;

  int rv = swapcontext(&((smx_ctx_sysv_t) context)->uc, &maestro_ctx);

  xbt_assert0((rv == 0), "Context swapping failure");
}

void smx_ctx_sysv_resume(smx_context_t new_context)
{
  smx_ctx_sysv_t maestro =
      (smx_ctx_sysv_t) simix_global->maestro_process->context;

  int rv =
      swapcontext(&(maestro->uc), &((smx_ctx_sysv_t) new_context)->uc);

  xbt_assert0((rv == 0), "Context swapping failure");
}

void smx_ctx_sysv_runall(xbt_swag_t processes)
{
  smx_context_t old_context;
  smx_process_t process;
  
  while ((process = xbt_swag_extract(processes))) {
    old_context = smx_current_context;
    smx_current_context = process->context;
    smx_ctx_sysv_resume(smx_current_context);
    smx_current_context = old_context;
  }
}

void smx_ctx_sysv_runall_parallel(xbt_swag_t processes)
{
  smx_process_t process;
  while((process = xbt_swag_extract(processes))){
    xbt_tpool_queue_job(tpool, (void_f_pvoid_t)smx_ctx_sysv_resume, process->context);
  }
}