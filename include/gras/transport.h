/* $Id$ */

/* transport - low level communication (send/receive bunches of bytes)      */
/* module's public interface exported to end user.                          */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRANSPORT_H
#define GRAS_TRANSPORT_H

#include "xbt/error.h"

typedef struct s_gras_socket *gras_socket_t;

xbt_error_t gras_socket_client(const char *host,
				unsigned short port,
				/* OUT */ gras_socket_t *dst);
xbt_error_t gras_socket_server(unsigned short port,
				/* OUT */ gras_socket_t *dst);
void         gras_socket_close(gras_socket_t sd);

/* get information about socket */
int   gras_socket_my_port  (gras_socket_t sock);
int   gras_socket_peer_port(gras_socket_t sock);
char *gras_socket_peer_name(gras_socket_t sock);

/* extended interface to get all details */
xbt_error_t gras_socket_client_ext(const char *host,
				    unsigned short port,
				    unsigned long int bufSize,
				    int raw, 
				    /* OUT */ gras_socket_t *dst);
xbt_error_t gras_socket_server_ext(unsigned short port,
				    unsigned long int bufSize,
				    int raw,
				    /* OUT */ gras_socket_t *dst);

/* using raw sockets */
xbt_error_t gras_socket_raw_send(gras_socket_t peer, 
				  unsigned int timeout,
				  unsigned long int expSize, 
				  unsigned long int msgSize);
xbt_error_t gras_socket_raw_recv(gras_socket_t peer, 
				  unsigned int timeout,
				  unsigned long int expSize, 
				  unsigned long int msgSize);

/* debuging functions */
xbt_error_t gras_socket_client_from_file(const char*path,
					  /* OUT */ gras_socket_t *dst);
xbt_error_t gras_socket_server_from_file(const char*path,
					  /* OUT */ gras_socket_t *dst);
					  
   
#endif /* GRAS_TRANSPORT_H */
