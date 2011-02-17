/* transport - low level communication                                      */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/***
 *** Options
 ***/
int gras_opt_trp_nomoredata_on_close = 0;

#include "xbt/ex.h"
#include "xbt/peer.h"
#include "portable.h"
#include "gras/Transport/transport_private.h"
#include "gras/Msg/msg_interface.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_trp, gras,
                                "Conveying bytes over the network");
XBT_LOG_NEW_SUBCATEGORY(gras_trp_meas, gras_trp,
                        "Conveying bytes over the network without formating for perf measurements");
static short int _gras_trp_started = 0;

static xbt_dict_t _gras_trp_plugins;    /* All registered plugins */
static void gras_trp_plugin_free(void *p);      /* free one of the plugins */

static void gras_trp_plugin_new(const char *name, gras_trp_setup_t setup)
{
  xbt_ex_t e;

  gras_trp_plugin_t plug = xbt_new0(s_gras_trp_plugin_t, 1);

  XBT_DEBUG("Create plugin %s", name);

  plug->name = xbt_strdup(name);

  TRY {
    setup(plug);
  } CATCH(e) {
    if (e.category == mismatch_error) {
      /* SG plugin raise mismatch when in RL mode (and vice versa) */
      free(plug->name);
      free(plug);
      plug = NULL;
      xbt_ex_free(e);
    } else {
      RETHROW;
    }
  }

  if (plug)
    xbt_dict_set(_gras_trp_plugins, name, plug, gras_trp_plugin_free);
}

void gras_trp_init(void)
{
  if (!_gras_trp_started) {
    /* make room for all plugins */
    _gras_trp_plugins = xbt_dict_new();

#ifdef HAVE_WINSOCK2_H
    /* initialize the windows mechanism */
    {
      WORD wVersionRequested;
      WSADATA wsaData;

      wVersionRequested = MAKEWORD(2, 0);
      xbt_assert0(WSAStartup(wVersionRequested, &wsaData) == 0,
                  "Cannot find a usable WinSock DLL");

      /* Confirm that the WinSock DLL supports 2.0. */
      /* Note that if the DLL supports versions greater    */
      /* than 2.0 in addition to 2.0, it will still return */
      /* 2.0 in wVersion since that is the version we      */
      /* requested.                                        */

      xbt_assert0(LOBYTE(wsaData.wVersion) == 2 &&
                  HIBYTE(wsaData.wVersion) == 0,
                  "Cannot find a usable WinSock DLL");
      XBT_INFO("Found and initialized winsock2");
    }                           /* The WinSock DLL is acceptable. Proceed. */
#elif HAVE_WINSOCK_H
    {
      WSADATA wsaData;
      xbt_assert0(WSAStartup(0x0101, &wsaData) == 0,
                  "Cannot find a usable WinSock DLL");
      XBT_INFO("Found and initialized winsock");
    }
#endif

    /* Add plugins */
    gras_trp_plugin_new("file", gras_trp_file_setup);
    gras_trp_plugin_new("sg", gras_trp_sg_setup);
    gras_trp_plugin_new("tcp", gras_trp_tcp_setup);
  }

  _gras_trp_started++;
}

void gras_trp_exit(void)
{
  XBT_DEBUG("gras_trp value %d", _gras_trp_started);
  if (_gras_trp_started == 0) {
    return;
  }

  if (--_gras_trp_started == 0) {
#ifdef HAVE_WINSOCK_H
    if (WSACleanup() == SOCKET_ERROR) {
      if (WSAGetLastError() == WSAEINPROGRESS) {
        WSACancelBlockingCall();
        WSACleanup();
      }
    }
#endif

    /* Delete the plugins */
    xbt_dict_free(&_gras_trp_plugins);
  }
}


void gras_trp_plugin_free(void *p)
{
  gras_trp_plugin_t plug = p;

  if (plug) {
    if (plug->exit) {
      plug->exit(plug);
    } else if (plug->data) {
      XBT_DEBUG("Plugin %s lacks exit(). Free data anyway.", plug->name);
      free(plug->data);
    }

    free(plug->name);
    free(plug);
  }
}


/**
 * gras_trp_socket_new:
 *
 * Malloc a new socket, and initialize it with defaults
 */
void gras_trp_socket_new(int incoming, gras_socket_t * dst)
{

  gras_socket_t sock = xbt_new0(s_gras_socket_t, 1);

  XBT_VERB("Create a new socket (%p)", (void *) sock);

  sock->plugin = NULL;

  sock->incoming = incoming ? 1 : 0;
  sock->outgoing = incoming ? 0 : 1;
  sock->accepting = incoming ? 1 : 0;
  sock->meas = 0;
  sock->recvd = 0;
  sock->valid = 1;
  sock->moredata = 0;

  sock->refcount = 1;
  sock->sd = -1;

  sock->data = NULL;
  sock->bufdata = NULL;

  *dst = sock;

  XBT_OUT;
}

/**
 * @brief Opens a server socket and makes it ready to be listened to.
 * @param port: port on which you want to listen
 * @param buf_size: size of the buffer (in byte) on the socket (for TCP sockets only). If 0, a sain default is used (32k, but may change)
 * @param measurement: whether this socket is meant to convey measurement (if you don't know, use 0 to exchange regular messages)
 *
 * In real life, you'll get a TCP socket.
 */
gras_socket_t
gras_socket_server_ext(unsigned short port,
                       unsigned long int buf_size, int measurement)
{

  xbt_ex_t e;
  gras_trp_plugin_t trp;
  gras_socket_t sock;

  XBT_DEBUG("Create a server socket from plugin %s on port %d",
         gras_if_RL()? "tcp" : "sg", port);
  trp = gras_trp_plugin_get_by_name(gras_if_SG()? "sg" : "tcp");

  /* defaults settings */
  gras_trp_socket_new(1, &sock);
  sock->plugin = trp;
  sock->buf_size = buf_size > 0 ? buf_size : 32 * 1024;
  sock->meas = measurement;

  /* Call plugin socket creation function */
  XBT_DEBUG("Prepare socket with plugin (fct=%p)", trp->socket_server);
  TRY {
    trp->socket_server(trp, port, sock);
    XBT_DEBUG("in=%c out=%c accept=%c",
           sock->incoming ? 'y' : 'n',
           sock->outgoing ? 'y' : 'n', sock->accepting ? 'y' : 'n');
  } CATCH(e) {

    free(sock);
    RETHROW;
  }

  if (!measurement)
    ((gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id))->myport
        = port;
  xbt_dynar_push(((gras_trp_procdata_t)
                  gras_libdata_by_id(gras_trp_libdata_id))->sockets,
                 &sock);

  gras_msg_listener_awake();
  return sock;
}

/**
 * @brief Opens a server socket on any port in the given range
 *
 * @param minport: first port we will try
 * @param maxport: last port we will try
 * @param buf_size: size of the buffer (in byte) on the socket (for TCP sockets only). If 0, a sain default is used (32k, but may change)
 * @param measurement: whether this socket is meant to convey measurement (if you don't know, use 0 to exchange regular messages)
 *
 * If none of the provided ports works, raises the exception got when trying the last possibility
 */
gras_socket_t
gras_socket_server_range(unsigned short minport, unsigned short maxport,
                         unsigned long int buf_size, int measurement)
{

  int port;
  gras_socket_t res = NULL;
  xbt_ex_t e;

  for (port = minport; port < maxport; port++) {
    TRY {
      res = gras_socket_server_ext(port, buf_size, measurement);
    }
    CATCH(e) {
      if (port == maxport)
        RETHROW;
      xbt_ex_free(e);
    }
    if (res)
      return res;
  }
  THROW_IMPOSSIBLE;
}

/**
 * @brief Opens a client socket to a remote host.
 * @param host: who you want to connect to
 * @param port: where you want to connect to on this host
 * @param buf_size: size of the buffer (in bytes) on the socket (for TCP sockets only). If 0, a sain default is used (32k, but may change)
 * @param measurement: whether this socket is meant to convey measurement (if you don't know, use 0 to exchange regular messages)
 *
 * In real life, you'll get a TCP socket.
 */
gras_socket_t
gras_socket_client_ext(const char *host,
                       unsigned short port,
                       unsigned long int buf_size, int measurement)
{

  xbt_ex_t e;
  gras_trp_plugin_t trp;
  gras_socket_t sock;

  trp = gras_trp_plugin_get_by_name(gras_if_SG()? "sg" : "tcp");

  XBT_DEBUG("Create a client socket from plugin %s",
         gras_if_RL()? "tcp" : "sg");
  /* defaults settings */
  gras_trp_socket_new(0, &sock);
  sock->plugin = trp;
  sock->buf_size = buf_size > 0 ? buf_size : 32 * 1024;
  sock->meas = measurement;

  /* plugin-specific */
  TRY {
    (*trp->socket_client) (trp,host,port,sock);
    XBT_DEBUG("in=%c out=%c accept=%c",
           sock->incoming ? 'y' : 'n',
           sock->outgoing ? 'y' : 'n', sock->accepting ? 'y' : 'n');
  } CATCH(e) {
    free(sock);
    RETHROW;
  }
  xbt_dynar_push(((gras_trp_procdata_t)
                  gras_libdata_by_id(gras_trp_libdata_id))->sockets,
                 &sock);
  gras_msg_listener_awake();
  return sock;
}

/**
 * @brief Opens a server socket and make it ready to be listened to.
 *
 * In real life, you'll get a TCP socket.
 */
gras_socket_t gras_socket_server(unsigned short port)
{
  return gras_socket_server_ext(port, 32 * 1024, 0);
}

/** @brief Opens a client socket to a remote host */
gras_socket_t gras_socket_client(const char *host, unsigned short port)
{
  return gras_socket_client_ext(host, port, 0, 0);
}

/** @brief Opens a client socket to a remote host specified as '\a host:\a port' */
gras_socket_t gras_socket_client_from_string(const char *host)
{
  xbt_peer_t p = xbt_peer_from_string(host);
  gras_socket_t res = gras_socket_client_ext(p->name, p->port, 0, 0);
  xbt_peer_free(p);
  return res;
}

void gras_socket_close_voidp(void *sock)
{
  gras_socket_close((gras_socket_t) sock);
}

/** \brief Close socket */
void gras_socket_close(gras_socket_t sock)
{
  if (--sock->refcount)
    return;

  xbt_dynar_t sockets =
      ((gras_trp_procdata_t)
       gras_libdata_by_id(gras_trp_libdata_id))->sockets;
  gras_socket_t sock_iter = NULL;
  unsigned int cursor;

  XBT_IN;
  XBT_VERB("Close %p", sock);
  if (sock == _gras_lastly_selected_socket) {
    xbt_assert0(!gras_opt_trp_nomoredata_on_close || !sock->moredata,
                "Closing a socket having more data in buffer while the nomoredata_on_close option is activated");

    if (sock->moredata)
      XBT_CRITICAL
          ("Closing a socket having more data in buffer. Option nomoredata_on_close disabled, so continuing.");
    _gras_lastly_selected_socket = NULL;
  }

  /* FIXME: Issue an event when the socket is closed */
  XBT_DEBUG("sockets pointer before %p", sockets);
  if (sock) {
    /* FIXME: Cannot get the dynar mutex, because it can be already locked */
//              _xbt_dynar_foreach(sockets,cursor,sock_iter) {
    for (cursor = 0; cursor < xbt_dynar_length(sockets); cursor++) {
      _xbt_dynar_cursor_get(sockets, cursor, &sock_iter);
      if (sock == sock_iter) {
        XBT_DEBUG("remove sock cursor %d dize %lu\n", cursor,
               xbt_dynar_length(sockets));
        xbt_dynar_cursor_rm(sockets, &cursor);
        if (sock->plugin->socket_close)
          (*sock->plugin->socket_close) (sock);

        /* free the memory */
        free(sock);
        XBT_OUT;
        return;
      }
    }
    XBT_WARN
        ("Ignoring request to free an unknown socket (%p). Execution stack:",
         sock);
    xbt_backtrace_display_current();
  }
  XBT_OUT;
}

/**
 * gras_trp_send:
 *
 * Send a bunch of bytes from on socket
 * (stable if we know the storage will keep as is until the next trp_flush)
 */
void gras_trp_send(gras_socket_t sd, char *data, long int size, int stable)
{
  xbt_assert0(sd->outgoing, "Socket not suited for data send");
  (*sd->plugin->send) (sd, data, size, stable);
}

/**
 * gras_trp_chunk_recv:
 *
 * Receive a bunch of bytes from a socket
 */
void gras_trp_recv(gras_socket_t sd, char *data, long int size)
{
  xbt_assert0(sd->incoming, "Socket not suited for data receive");
  (sd->plugin->recv) (sd, data, size);
}

/**
 * gras_trp_flush:
 *
 * Make sure all pending communications are done
 */
void gras_trp_flush(gras_socket_t sd)
{
  if (sd->plugin->flush)
    (sd->plugin->flush) (sd);
}

gras_trp_plugin_t gras_trp_plugin_get_by_name(const char *name)
{
  return xbt_dict_get(_gras_trp_plugins, name);
}

int gras_socket_my_port(gras_socket_t sock)
{
  if (!sock->plugin->my_port)
    THROW1(unknown_error,0,"Function my_port unimplemented in plugin %s",sock->plugin->name);
  return (*sock->plugin->my_port)(sock);

}

int gras_socket_peer_port(gras_socket_t sock)
{
  if (!sock->plugin->peer_port)
    THROW1(unknown_error,0,"Function peer_port unimplemented in plugin %s",sock->plugin->name);
  return (*sock->plugin->peer_port)(sock);
}

const char *gras_socket_peer_name(gras_socket_t sock)
{
  xbt_assert(sock->plugin);
  return (*sock->plugin->peer_name)(sock);
}

const char *gras_socket_peer_proc(gras_socket_t sock)
{
  return (*sock->plugin->peer_proc)(sock);
}

void gras_socket_peer_proc_set(gras_socket_t sock, char *peer_proc)
{
  return (*sock->plugin->peer_proc_set)(sock,peer_proc);
}

/** \brief Check if the provided socket is a measurement one (or a regular one) */
int gras_socket_is_meas(gras_socket_t sock)
{
  return sock->meas;
}

/** \brief Send a chunk of (random) data over a measurement socket
 *
 * @param peer measurement socket to use for the experiment
 * @param timeout timeout (in seconds)
 * @param msg_size size of each chunk sent over the socket (in bytes).
 * @param msg_amount how many of these packets you want to send.
 *
 * Calls to gras_socket_meas_send() and gras_socket_meas_recv() on
 * each side of the socket should be paired.
 *
 * The exchanged data is zeroed to make sure it's initialized, but
 * there is no way to control what is sent (ie, you cannot use these
 * functions to exchange data out of band).
 *
 * @warning: in SimGrid version 3.1 and previous, the numerical arguments
 *           were the total amount of data to send and the msg_size. This
 *           was changed for the fool wanting to send more than MAXINT
 *           bytes in a fat pipe.
 */
void gras_socket_meas_send(gras_socket_t peer,
                           unsigned int timeout,
                           unsigned long int msg_size,
                           unsigned long int msg_amount)
{
  char *chunk = NULL;
  unsigned long int sent_sofar;

  XBT_IN;
  THROW0(unknown_error,0,"measurement sockets were broken in this release of SimGrid and should be ported back in the future."
      "If you depend on it, sorry, you have to use an older version, or wait for the future version using it...");
  if (gras_if_RL())
    chunk = xbt_malloc0(msg_size);

  xbt_assert0(peer->meas,
              "Asked to send measurement data on a regular socket");
  xbt_assert0(peer->outgoing,
              "Socket not suited for data send (was created with gras_socket_server(), not gras_socket_client())");

  for (sent_sofar = 0; sent_sofar < msg_amount; sent_sofar++) {
    XBT_CDEBUG(gras_trp_meas,
            "Sent %lu msgs of %lu (size of each: %ld) to %s:%d",
            sent_sofar, msg_amount, msg_size, gras_socket_peer_name(peer),
            gras_socket_peer_port(peer));
    (*peer->plugin->raw_send) (peer, chunk, msg_size);
  }
  XBT_CDEBUG(gras_trp_meas,
          "Sent %lu msgs of %lu (size of each: %ld) to %s:%d", sent_sofar,
          msg_amount, msg_size, gras_socket_peer_name(peer),
          gras_socket_peer_port(peer));

  if (gras_if_RL())
    free(chunk);

  XBT_OUT;
}

/** \brief Receive a chunk of data over a measurement socket
 *
 * Calls to gras_socket_meas_send() and gras_socket_meas_recv() on
 * each side of the socket should be paired.
 *
 * @warning: in SimGrid version 3.1 and previous, the numerical arguments
 *           were the total amount of data to send and the msg_size. This
 *           was changed for the fool wanting to send more than MAXINT
 *           bytes in a fat pipe.
 */
void gras_socket_meas_recv(gras_socket_t peer,
                           unsigned int timeout,
                           unsigned long int msg_size,
                           unsigned long int msg_amount)
{

  char *chunk = NULL;
  unsigned long int got_sofar;

  XBT_IN;
  THROW0(unknown_error,0,"measurement sockets were broken in this release of SimGrid and should be ported back in the future."
      "If you depend on it, sorry, you have to use an older version, or wait for the future version using it...");

  if (gras_if_RL())
    chunk = xbt_malloc(msg_size);

  xbt_assert0(peer->meas,
              "Asked to receive measurement data on a regular socket");
  xbt_assert0(peer->incoming, "Socket not suited for data receive");

  for (got_sofar = 0; got_sofar < msg_amount; got_sofar++) {
    XBT_CDEBUG(gras_trp_meas,
            "Recvd %ld msgs of %lu (size of each: %ld) from %s:%d",
            got_sofar, msg_amount, msg_size, gras_socket_peer_name(peer),
            gras_socket_peer_port(peer));
    (peer->plugin->raw_recv) (peer, chunk, msg_size);
  }
  XBT_CDEBUG(gras_trp_meas,
          "Recvd %ld msgs of %lu (size of each: %ld) from %s:%d",
          got_sofar, msg_amount, msg_size, gras_socket_peer_name(peer),
          gras_socket_peer_port(peer));

  if (gras_if_RL())
    free(chunk);
  XBT_OUT;
}

/**
 * \brief Something similar to the good old accept system call.
 *
 * Make sure that there is someone speaking to the provided server socket.
 * In RL, it does an accept(2) and return the result as last argument.
 * In SG, as accepts are useless, it returns the provided argument as result.
 * You should thus test whether (peer != accepted) before closing both of them.
 *
 * You should only call this on measurement sockets. It is automatically
 * done for regular sockets, but you usually want more control about
 * what's going on with measurement sockets.
 */
gras_socket_t gras_socket_meas_accept(gras_socket_t peer)
{
  gras_socket_t res;
  THROW0(unknown_error,0,"measurement sockets were broken in this release of SimGrid and should be ported back in the future."
      "If you depend on it, sorry, you have to use an older version, or wait for the future version using it...");

  xbt_assert0(peer->meas,
              "No need to accept on non-measurement sockets (it's automatic)");

  if (!peer->accepting) {
    /* nothing to accept here (must be in SG) */
    /* BUG: FIXME: this is BAD! it makes tricky to free the accepted socket */
    return peer;
  }

  res = (peer->plugin->socket_accept) (peer);
  res->meas = peer->meas;
  XBT_CDEBUG(gras_trp_meas, "meas_accepted onto %d", res->sd);

  return res;
}


/*
 * Creating procdata for this module
 */
static void *gras_trp_procdata_new(void)
{
  gras_trp_procdata_t res = xbt_new(s_gras_trp_procdata_t, 1);

  res->name = xbt_strdup("gras_trp");
  res->name_len = 0;
  res->sockets = xbt_dynar_new_sync(sizeof(gras_socket_t *), NULL);
  res->myport = 0;

  return (void *) res;
}

/*
 * Freeing procdata for this module
 */
static void gras_trp_procdata_free(void *data)
{
  gras_trp_procdata_t res = (gras_trp_procdata_t) data;

  xbt_dynar_free(&(res->sockets));
  free(res->name);
  free(res);
}

void gras_trp_socketset_dump(const char *name)
{
  gras_trp_procdata_t procdata =
      (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);

  unsigned int it;
  gras_socket_t s;

  XBT_INFO("** Dump the socket set %s", name);
  xbt_dynar_foreach(procdata->sockets, it, s) {
    XBT_INFO("  %p -> %s:%d %s",
          s, gras_socket_peer_name(s), gras_socket_peer_port(s),
          s->valid ? "(valid)" : "(peer dead)");
  }
  XBT_INFO("** End of socket set %s", name);
}

/*
 * Module registration
 */
int gras_trp_libdata_id;
void gras_trp_register()
{
  gras_trp_libdata_id =
      gras_procdata_add("gras_trp", gras_trp_procdata_new,
                        gras_trp_procdata_free);
}

int gras_os_myport(void)
{
  return ((gras_trp_procdata_t)
          gras_libdata_by_id(gras_trp_libdata_id))->myport;
}
