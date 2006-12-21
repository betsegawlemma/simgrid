/* $Id$ */

/* log_usage - A test of normal usage of the log facilities                 */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
 
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "gras.h"



XBT_LOG_NEW_DEFAULT_SUBCATEGORY(test, top,"Logging specific to this test");
XBT_LOG_NEW_CATEGORY(top,"Useless test channel");

#ifdef __BORLANDC__
#pragma argsused
#endif

int main(int argc, char **argv) {
  xbt_init(&argc,argv);

  DEBUG1("val=%d", 1);
  WARN1("val=%d", 2);
  CDEBUG2(top, "val=%d%s", 3, "!");
  CRITICAL6("false alarm%s%s%s%s%s%s", "","","","","","!");
  
  xbt_exit();
  return 0;
}
