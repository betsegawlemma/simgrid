/* Copyright (c) 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

//teshsuite/simdag/platforms/evaluate_parse_time ../examples/platforms/nancy.xml

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "surf/surf_private.h"
#include "xbt/xbt_os_time.h"

extern routing_global_t global_routing;

int main(int argc, char **argv)
{
  xbt_os_timer_t timer = xbt_os_timer_new();

	/* initialization of SD */
	SD_init(&argc, argv);

	/* creation of the environment, timed */
	xbt_os_timer_start(timer);
	SD_create_environment(argv[1]);
  xbt_os_timer_stop(timer);

  /* Display the result and exit after cleanup */
	printf( "%lf\n", xbt_os_timer_elapsed(timer) );

	sleep(20);

	SD_exit();

	free(timer);
	return 0;
}
