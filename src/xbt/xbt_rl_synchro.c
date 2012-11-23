/* xbt_synchro -- Synchronization virtualized depending on whether we are   */
/*                in simulation or real life (act on simulated processes)   */

/* This is the real life implementation, using xbt_os_thread to be portable */
/* to windows and linux.                                                    */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/ex.h"
#include "portable.h"

#include "xbt/xbt_os_thread.h"  /* The implementation we use */

/* the implementation would be cleaner (and faster) with ELF symbol aliasing */
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_sync, xbt,
                                "Synchronization mechanism");

typedef struct s_xbt_thread_ {
  xbt_os_thread_t os_thread;
  void_f_pvoid_t code;
  void *userparam;
} s_xbt_thread_t;

static void *xbt_thread_create_wrapper(void *p)
{
  xbt_thread_t t = (xbt_thread_t) p;
  XBT_DEBUG("I'm thread %p", p);
  t->code(t->userparam);
  return NULL;
}


xbt_thread_t xbt_thread_create(const char *name, void_f_pvoid_t code,
                               void *param, int joinable)
{

  xbt_thread_t res = xbt_new0(s_xbt_thread_t, 1);
  res->userparam = param;
  res->code = code;
  XBT_DEBUG("Create thread %p", res);
  res->os_thread =
      xbt_os_thread_create(name, xbt_thread_create_wrapper, res, NULL);
  return res;
}

const char *xbt_thread_name(xbt_thread_t t)
{
  return xbt_os_thread_name(t->os_thread);
}

const char *xbt_thread_self_name(void)
{
  return xbt_os_thread_self_name();
}

void xbt_thread_join(xbt_thread_t thread)
{
  XBT_DEBUG("Join thread %p", thread);
  xbt_os_thread_join(thread->os_thread, NULL);
  xbt_free(thread);
}

void xbt_thread_exit()
{
  XBT_DEBUG("Thread exits");
  xbt_os_thread_exit(NULL);
}

xbt_thread_t xbt_thread_self(void)
{
  return (xbt_thread_t) xbt_os_thread_self();
}

void xbt_thread_yield(void)
{
  XBT_DEBUG("Thread yields");
  xbt_os_thread_yield();
}

void xbt_thread_cancel(xbt_thread_t t)
{
  XBT_DEBUG("Cancel thread %p", t);
  xbt_os_thread_cancel(t->os_thread);
}

/****** mutex related functions ******/
struct xbt_mutex_ {
  /* KEEP IT IN SYNC WITH OS IMPLEMENTATION (both win and lin) */
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t m;
#elif defined(_XBT_WIN32)
  CRITICAL_SECTION lock;
#endif
};

xbt_mutex_t xbt_mutex_init(void)
{
  xbt_mutex_t res = (xbt_mutex_t) xbt_os_mutex_init();
  XBT_DEBUG("Create mutex %p", res);
  return res;
}

void xbt_mutex_acquire(xbt_mutex_t mutex)
{
  XBT_DEBUG("Acquire mutex %p", mutex);
  xbt_os_mutex_acquire((xbt_os_mutex_t) mutex);
}

void xbt_mutex_timedacquire(xbt_mutex_t mutex, double delay)
{
  XBT_DEBUG("Acquire mutex %p with delay %lf", mutex, delay);
  xbt_os_mutex_timedacquire((xbt_os_mutex_t) mutex, delay);
}

void xbt_mutex_release(xbt_mutex_t mutex)
{
  XBT_DEBUG("Unlock mutex %p", mutex);
  xbt_os_mutex_release((xbt_os_mutex_t) mutex);
}

void xbt_mutex_destroy(xbt_mutex_t mutex)
{
  XBT_DEBUG("Destroy mutex %p", mutex);
  xbt_os_mutex_destroy((xbt_os_mutex_t) mutex);
}

#ifdef _XBT_WIN32
enum {                          /* KEEP IT IN SYNC WITH OS IMPLEM */
  SIGNAL = 0,
  BROADCAST = 1,
  MAX_EVENTS = 2
};
#endif

/***** condition related functions *****/
typedef struct xbt_cond_ {
  /* KEEP IT IN SYNC WITH OS IMPLEMENTATION (both win and lin) */
#ifdef HAVE_PTHREAD_H
  pthread_cond_t c;
#elif defined(_XBT_WIN32)
  HANDLE events[MAX_EVENTS];

  unsigned int waiters_count;   /* the number of waiters                        */
  CRITICAL_SECTION waiters_count_lock;  /* protect access to waiters_count  */
#endif
} s_xbt_cond_t;

xbt_cond_t xbt_cond_init(void)
{
  xbt_cond_t res = (xbt_cond_t) xbt_os_cond_init();
  XBT_DEBUG("Create cond %p", res);
  return res;
}

void xbt_cond_wait(xbt_cond_t cond, xbt_mutex_t mutex)
{
  XBT_DEBUG("Wait cond %p, mutex %p", cond, mutex);
  xbt_os_cond_wait((xbt_os_cond_t) cond, (xbt_os_mutex_t) mutex);
}

void xbt_cond_timedwait(xbt_cond_t cond, xbt_mutex_t mutex, double delay)
{
  XBT_DEBUG("Wait cond %p, mutex %p for %f sec", cond, mutex, delay);
  xbt_os_cond_timedwait((xbt_os_cond_t) cond, (xbt_os_mutex_t) mutex,
                        delay);
  XBT_DEBUG("Done waiting cond %p, mutex %p for %f sec", cond, mutex, delay);
}

void xbt_cond_signal(xbt_cond_t cond)
{
  XBT_DEBUG("Signal cond %p", cond);
  xbt_os_cond_signal((xbt_os_cond_t) cond);
}

void xbt_cond_broadcast(xbt_cond_t cond)
{
  XBT_DEBUG("Broadcast cond %p", cond);
  xbt_os_cond_broadcast((xbt_os_cond_t) cond);
}

void xbt_cond_destroy(xbt_cond_t cond)
{
  XBT_DEBUG("Destroy cond %p", cond);
  xbt_os_cond_destroy((xbt_os_cond_t) cond);
}
