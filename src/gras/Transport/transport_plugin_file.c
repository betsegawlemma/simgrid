/* $Id$ */

/* File transport - send/receive a bunch of bytes from a file               */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "transport_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(trp_file,transport,
	"Pseudo-transport to write to/read from a file");

/***
 *** Prototypes 
 ***/
void         gras_trp_file_close(gras_socket_t sd);
  
xbt_error_t gras_trp_file_chunk_send(gras_socket_t sd,
				      const char *data,
				      long int size);

xbt_error_t gras_trp_file_chunk_recv(gras_socket_t sd,
				      char *data,
				      long int size);


/***
 *** Specific plugin part
 ***/

typedef struct {
  fd_set incoming_socks;
} gras_trp_file_plug_data_t;

/***
 *** Specific socket part
 ***/



/***
 *** Code
 ***/
xbt_error_t
gras_trp_file_setup(gras_trp_plugin_t *plug) {

  gras_trp_file_plug_data_t *file = xbt_new(gras_trp_file_plug_data_t,1);

  FD_ZERO(&(file->incoming_socks));

  plug->socket_close = gras_trp_file_close;
  plug->chunk_send   = gras_trp_file_chunk_send;
  plug->chunk_recv   = gras_trp_file_chunk_recv;
  plug->data         = (void*)file;

  return no_error;
}

/**
 * gras_socket_client_from_file:
 *
 * Create a client socket from a file path.
 *
 * This only possible in RL, and is mainly for debugging.
 */
xbt_error_t
gras_socket_client_from_file(const char*path,
			     /* OUT */ gras_socket_t *dst) {
  xbt_error_t errcode;
  gras_trp_plugin_t *trp;

  xbt_assert0(gras_if_RL(),
	       "Cannot use file as socket in the simulator");

  gras_trp_socket_new(0,dst);

  TRY(gras_trp_plugin_get_by_name("file",&trp));
  (*dst)->plugin=trp;

  if (strcmp("-", path)) {
    (*dst)->sd = open(path, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP );
    
    if ( (*dst)->sd < 0) {
      RAISE2(system_error,
	     "Cannot create a client socket from file %s: %s",
	     path, strerror(errno));
    }
  } else {
    (*dst)->sd = 1; /* stdout */
  }

  DEBUG5("sock_client_from_file(%s): sd=%d in=%c out=%c accept=%c",
	 path,
	 (*dst)->sd,
	 (*dst)->incoming?'y':'n', 
	 (*dst)->outgoing?'y':'n',
	 (*dst)->accepting?'y':'n');
   
  return no_error;
}

/**
 * gras_socket_server_from_file:
 *
 * Create a server socket from a file path.
 *
 * This only possible in RL, and is mainly for debugging.
 */
xbt_error_t
gras_socket_server_from_file(const char*path,
			     /* OUT */ gras_socket_t *dst) {
  xbt_error_t errcode;
  gras_trp_plugin_t *trp;

  xbt_assert0(gras_if_RL(),
	       "Cannot use file as socket in the simulator");

  gras_trp_socket_new(1,dst);

  TRY(gras_trp_plugin_get_by_name("file",&trp));
  (*dst)->plugin=trp;


  if (strcmp("-", path)) {
    (*dst)->sd = open(path, O_RDONLY );

    if ( (*dst)->sd < 0) {
      RAISE2(system_error,
	     "Cannot create a server socket from file %s: %s",
	     path, strerror(errno));
    }
  } else {
    (*dst)->sd = 0; /* stdin */
  }

  DEBUG4("sd=%d in=%c out=%c accept=%c",
	 (*dst)->sd,
	 (*dst)->incoming?'y':'n', 
	 (*dst)->outgoing?'y':'n',
	 (*dst)->accepting?'y':'n');

  return no_error;
}

void gras_trp_file_close(gras_socket_t sock){
  gras_trp_file_plug_data_t *data;
  
  if (!sock) return; /* close only once */
  data=sock->plugin->data;

  if (sock->sd == 0) {
    DEBUG0("Do not close stdin");
  } else if (sock->sd == 1) {
    DEBUG0("Do not close stdout");
  } else {
    DEBUG1("close file connection %d", sock->sd);

    /* forget about the socket */
    FD_CLR(sock->sd, &(data->incoming_socks));

    /* close the socket */
    if(close(sock->sd) < 0) {
      WARN2("error while closing file %d: %s", 
	       sock->sd, strerror(errno));
    }
  }
}

/**
 * gras_trp_file_chunk_send:
 *
 * Send data on a file pseudo-socket
 */
xbt_error_t 
gras_trp_file_chunk_send(gras_socket_t sock,
			 const char *data,
			 long int size) {
  
  xbt_assert0(sock->outgoing, "Cannot write on client file socket");
  xbt_assert0(size >= 0, "Cannot send a negative amount of data");

  while (size) {
    int status = 0;
    
    DEBUG3("write(%d, %p, %ld);", sock->sd, data, (long int)size);
    status = write(sock->sd, data, (long int)size);
    
    if (status == -1) {
      RAISE4(system_error,"write(%d,%p,%d) failed: %s",
	     sock->sd, data, (int)size,
	     strerror(errno));
    }
    
    if (status) {
      size  -= status;
      data  += status;
    } else {
      RAISE0(system_error,"file descriptor closed");
    }
  }

  return no_error;
}
/**
 * gras_trp_file_chunk_recv:
 *
 * Receive data on a file pseudo-socket.
 */
xbt_error_t 
gras_trp_file_chunk_recv(gras_socket_t sock,
			char *data,
			long int size) {

  xbt_assert0(sock, "Cannot recv on an NULL socket");
  xbt_assert0(sock->incoming, "Cannot recv on client file socket");
  xbt_assert0(size >= 0, "Cannot receive a negative amount of data");
  
  while (size) {
    int status = 0;
    
    status = read(sock->sd, data, (long int)size);
    DEBUG3("read(%d, %p, %ld);", sock->sd, data, size);
    
    if (status == -1) {
      RAISE4(system_error,"read(%d,%p,%d) failed: %s",
	     sock->sd, data, (int)size,
	     strerror(errno));
    }
    
    if (status) {
      size  -= status;
      data  += status;
    } else {
      RAISE0(system_error,"file descriptor closed");
    }
  }
  
  return no_error;
}

