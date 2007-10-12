/* $Id: log.c 4794 2007-10-10 12:38:37Z mquinson $ */

/* Copyright (c) 2003-2007 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef LOG_PRIVATE_H
#define LOG_PRIVATE_H

#include "xbt/log.h"
struct xbt_log_appender_s {
  void (*do_append) (xbt_log_appender_t this_appender,
		     char *event);
  void (*free_) (xbt_log_appender_t this_);
  void *data;
};

struct xbt_log_layout_s {
  void (*do_layout)(xbt_log_layout_t l,
		    xbt_log_event_t event, const char *fmt,
		    xbt_log_appender_t appender);
  void (*free_) (xbt_log_layout_t l);
  void *data;
} ;


#endif /* LOG_PRIVATE_H */
