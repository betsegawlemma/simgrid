/* GridRPC - Fake Grid RPC thingy doing matrix multiplications (as expected)*/

/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define GRAS_DEFINE_TYPE_EXTERN
#include "xbt/matrix.h"
#include "mmrpc.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(MatMult);

static gras_socket_t try_gras_socket_client(const char *host, int port)
{
  volatile gras_socket_t sock = NULL;
  xbt_ex_t e;
  TRY {
    sock = gras_socket_client(host, port);
  }
  CATCH(e) {
    if (e.category != system_error)
      RETHROWF("Unable to connect to the server: %s");
    xbt_ex_free(e);
  }
  return sock;
}

int client(int argc, char *argv[])
{
  gras_socket_t toserver = NULL;        /* peer */

  gras_socket_t from;
  xbt_matrix_t request[2], answer;

  int i, j;

  const char *host = "127.0.0.1";
  int port = 4000;

  /* 1. Init the GRAS's infrastructure */
  gras_init(&argc, argv);

  /* 2. Get the server's address. The command line override defaults when specified */
  if (argc == 3) {
    host = argv[1];
    port = atoi(argv[2]);
  }

  XBT_INFO("Launch client (server on %s:%d)", host, port);

  /* 3. Register the messages (before use) */
  mmrpc_register_messages();

  /* 4. Create a socket to speak to the server */
  while (!(toserver = try_gras_socket_client(host, port)))
    gras_os_sleep(0.05);
  XBT_INFO("Connected to %s:%d.", host, port);

  /* 5. Keep the user informed of what's going on */
  XBT_INFO(">>>>>>>> Connected to server which is on %s:%d <<<<<<<<",
        gras_socket_peer_name(toserver), gras_socket_peer_port(toserver));

  /* 6. Prepare and send the request to the server */

  request[0] = xbt_matrix_double_new_id(MATSIZE, MATSIZE);
  request[1] = xbt_matrix_double_new_rand(MATSIZE, MATSIZE);

  /*
     xbt_matrix_dump(request[0],"C:sent0",0,xbt_matrix_dump_display_double);
     xbt_matrix_dump(request[1],"C:sent1",0,xbt_matrix_dump_display_double);
   */

  gras_msg_send(toserver, "request", &request);

  xbt_matrix_free(request[0]);

  XBT_INFO(">>>>>>>> Request sent to %s:%d <<<<<<<<",
        gras_socket_peer_name(toserver), gras_socket_peer_port(toserver));

  /* 7. Wait for the answer from the server, and deal with issues */
  gras_msg_wait(6000, "answer", &from, &answer);

  /*
     xbt_matrix_dump(answer,"C:answer",0,xbt_matrix_dump_display_double);
   */
  for (i = 0; i < MATSIZE; i++)
    for (j = 0; i < MATSIZE; i++)
      xbt_assert(xbt_matrix_get_as(answer, i, j, double) ==
                  xbt_matrix_get_as(request[1], i, j, double),
                  "Answer does not match expectations. Found %f at cell %d,%d instead of %f",
                  xbt_matrix_get_as(answer, i, j, double), i, j,
                  xbt_matrix_get_as(request[1], i, j, double));

  /* 8. Keep the user informed of what's going on, again */
  XBT_INFO(">>>>>>>> Got answer from %s:%d (values are right) <<<<<<<<",
        gras_socket_peer_name(from), gras_socket_peer_port(from));

  /* 9. Free the allocated resources, and shut GRAS down */
  xbt_matrix_free(request[1]);
  xbt_matrix_free(answer);
  gras_socket_close(toserver);
  gras_exit();
  return 0;
}                               /* end_of_client */
