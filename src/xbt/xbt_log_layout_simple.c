/* $Id$ */

/* layout_simple - a dumb log layout                                        */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/synchro.h" /* xbt_thread_name */
#include "gras/virtu.h"
#include <stdio.h>
#include "portable.h"

extern const char *xbt_log_priority_names[7];

/* only used after the format using: we suppose that the buffer is big enough to display our data */
#define check_overflow \
  if (p-ev->buffer > XBT_LOG_BUFF_SIZE) { /* buffer overflow */ \
     p=ev->buffer + XBT_LOG_BUFF_SIZE - strlen(" >> OUTPUT TRUNCATED <<\n"); \
     p+=sprintf(p," >> OUTPUT TRUNCATED <<\n"); \
  } 

static void xbt_log_layout_simple_doit(xbt_log_layout_t l,
				       xbt_log_event_t ev, 
				       const char *fmt) {
  static double begin_of_time = -1;
  char *p;  

  xbt_assert0(ev->priority>=0,
	      "Negative logging priority naturally forbidden");
  xbt_assert1(ev->priority<sizeof(xbt_log_priority_names),
	      "Priority %d is greater than the biggest allowed value",
	      ev->priority);

  if (begin_of_time<0) 
    begin_of_time=gras_os_time();

  p = ev->buffer;
  p += snprintf(p,XBT_LOG_BUFF_SIZE-(p-ev->buffer),"[");

  /* Display the proc info if available */
  if(strlen(xbt_procname()))
    p += snprintf(p,XBT_LOG_BUFF_SIZE-(p-ev->buffer),"%s:%s:(%d) ", 
		 gras_os_myname(), xbt_procname(),(*xbt_getpid)());

  /* Display the date */
  p += snprintf(p,XBT_LOG_BUFF_SIZE-(p-ev->buffer),"%f] ", gras_os_time()-begin_of_time);

  /* Display file position if not INFO*/
  if (ev->priority != xbt_log_priority_info)
    p += snprintf(p,XBT_LOG_BUFF_SIZE-(p-ev->buffer), "%s:%d: ", ev->fileName, ev->lineNum);

  /* Display category name */
  p += snprintf(p,XBT_LOG_BUFF_SIZE-(p-ev->buffer), "[%s/%s] ", 
	       ev->cat->name, xbt_log_priority_names[ev->priority] );

  /* Display user-provided message */
  p += vsnprintf(p,XBT_LOG_BUFF_SIZE-(p-ev->buffer), fmt, ev->ap);
  check_overflow;
   
  /* End it */
  p += snprintf(p,XBT_LOG_BUFF_SIZE-(p-ev->buffer), "\n");
  check_overflow;
}

xbt_log_layout_t xbt_log_layout_simple_new(char *arg) {
  xbt_log_layout_t res = xbt_new0(s_xbt_log_layout_t,1);
  res->do_layout = xbt_log_layout_simple_doit;
  return res;
}
