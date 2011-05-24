/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_WIN32
#include <unistd.h>
#endif

#include "msg/msg.h"
#include "xbt/graph.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(graphicator,
                             "Graphicator Logging System");

int main(int argc, char **argv)
{
#ifdef HAVE_TRACING
  MSG_global_init(&argc, argv);

  if (argc < 3){
    XBT_INFO("Usage: %s <platform_file.xml> <graphviz_file.dot>", argv[0]);
    return 1;
  }
  char *platformFile = argv[1];
  char *graphvizFile = argv[2];

  MSG_create_environment(platformFile);

  //creating the graph structure
  xbt_graph_t graph = TRACE_platform_graph();
  if (graph == NULL){
    XBT_INFO ("%s expects --cfg=tracing:1 --cfg=tracing/platform:1", argv[0]);
  }else{
    TRACE_platform_graph_export_graphviz (graph, graphvizFile);
    XBT_INFO ("Output is in file %s", graphvizFile);
  }
  MSG_clean();
#else
  XBT_INFO ("works only if simgrid was compiled with tracing enabled.");
#endif
  return 0;
}
