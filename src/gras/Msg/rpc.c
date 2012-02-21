/* rpc - RPC implementation on top of GRAS messages                         */

/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Msg/msg_private.h"

xbt_set_t _gras_rpctype_set = NULL;
xbt_dynar_t _gras_rpc_cancelled = NULL;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_msg_rpc, gras_msg, "RPC mecanism");


/** @brief declare a new versionned RPC type of the given name and payloads
 *
 * @param name: name as it should be used for logging messages (must be uniq)
 * @param payload_request: datatype of request
 * @param payload_answer: datatype of answer
 *
 * Registers a new RPC message to the GRAS mechanism. RPC are constituted of a pair
 * of messages.
 */
void
gras_msgtype_declare_rpc(const char *name,
                         xbt_datadesc_type_t payload_request,
                         xbt_datadesc_type_t payload_answer)
{

  gras_msgtype_declare_ext(name, 0,
                           e_gras_msg_kind_rpccall,
                           payload_request, payload_answer);

}

/** @brief declare a new versionned RPC type of the given name and payloads
 *
 * @param name: name as it should be used for logging messages (must be uniq)
 * @param version: something like versionning symbol
 * @param payload_request: datatype of request
 * @param payload_answer: datatype of answer
 *
 * Registers a new RPC message to the GRAS mechanism. RPC are constituted of a pair
 * of messages.
 *
 * Use this version instead of gras_rpctype_declare when you change the
 * semantic or syntax of a message and want your programs to be able to deal
 * with both versions. Internally, each will be handled as an independent
 * message type, so you can register differents for each of them.
 */
void
gras_msgtype_declare_rpc_v(const char *name,
                           short int version,
                           xbt_datadesc_type_t payload_request,
                           xbt_datadesc_type_t payload_answer)
{

  gras_msgtype_declare_ext(name, version,
                           e_gras_msg_kind_rpccall,
                           payload_request, payload_answer);

}

static unsigned long int last_msg_ID = 0;

static int msgfilter_rpcID(gras_msg_t msg, void *ctx)
{
  unsigned long int ID = *(unsigned long int *) ctx;
  int res = msg->ID == ID &&
      (msg->kind == e_gras_msg_kind_rpcanswer
       || msg->kind == e_gras_msg_kind_rpcerror);
  unsigned int cursor;
  gras_msg_cb_ctx_t rpc_ctx;


  XBT_DEBUG
      ("Filter a message of ID %lu, type '%s' and kind '%s'. Waiting for ID=%lu. %s",
       msg->ID, msg->type->name, e_gras_msg_kind_names[msg->kind], ID,
       res ? "take it" : "reject");

  if (res && !_gras_rpc_cancelled)
    return res;

  /* Check whether it is an old answer to a message we already canceled */
  xbt_dynar_foreach(_gras_rpc_cancelled, cursor, rpc_ctx) {
    if (msg->ID == rpc_ctx->ID && msg->kind == e_gras_msg_kind_rpcanswer) {
      XBT_VERB
          ("Got an answer to the already canceled (timeouted?) RPC %lu. Ignore it (leaking the payload!).",
           msg->ID);
      xbt_dynar_cursor_rm(_gras_rpc_cancelled, &cursor);
      return 1;
    }
  }

  return res;
}

/* Mallocator cruft */
xbt_mallocator_t gras_msg_ctx_mallocator = NULL;
void *gras_msg_ctx_mallocator_new_f(void)
{
  return xbt_new0(s_gras_msg_cb_ctx_t, 1);
}

void gras_msg_ctx_mallocator_reset_f(void *ctx)
{
  memset(ctx, 0, sizeof(s_gras_msg_cb_ctx_t));
}

/** @brief Launch a RPC call, but do not block for the answer */
gras_msg_cb_ctx_t
gras_msg_rpc_async_call_(xbt_socket_t server,
                         double timeOut,
                         gras_msgtype_t msgtype, void *request)
{
  gras_msg_cb_ctx_t ctx = xbt_mallocator_get(gras_msg_ctx_mallocator);

  if (msgtype->ctn_type) {
    xbt_assert(request,
                "RPC type '%s' convey a payload you must provide",
                msgtype->name);
  } else {
    xbt_assert(!request,
                "No payload was declared for RPC type '%s'",
                msgtype->name);
  }

  ctx->ID = last_msg_ID++;
  ctx->expeditor = server;
  ctx->msgtype = msgtype;
  ctx->timeout = timeOut;

  XBT_VERB("Send to %s:%d a RPC of type '%s' (ID=%lu)",
        xbt_socket_peer_name(server),
        xbt_socket_peer_port(server), msgtype->name, ctx->ID);

  gras_msg_send_ext(server, e_gras_msg_kind_rpccall, ctx->ID, msgtype,
                    request);

  return ctx;
}

/** @brief Wait the answer of a RPC call previously launched asynchronously */
void gras_msg_rpc_async_wait(gras_msg_cb_ctx_t ctx, void *answer)
{
  xbt_ex_t e;
  s_gras_msg_t received;

  if (ctx->msgtype->answer_type) {
    xbt_assert(answer,
                "Answers to RPC '%s' convey a payload you must accept",
                ctx->msgtype->name);
  } else {
    xbt_assert(!answer,
                "No payload was declared for answers to RPC '%s'",
                ctx->msgtype->name);
  }

  TRY {
    /* The filter returns 1 when we eat an old RPC answer to something canceled */
    do {
      gras_msg_wait_ext_(ctx->timeout,
                         ctx->msgtype, NULL, msgfilter_rpcID, &ctx->ID,
                         &received);
    } while (received.ID != ctx->ID);

  }
  CATCH(e) {
    if (!_gras_rpc_cancelled)
      _gras_rpc_cancelled = xbt_dynar_new(sizeof(ctx), NULL);
    xbt_dynar_push(_gras_rpc_cancelled, &ctx);
    XBT_INFO
        ("canceled RPC %lu pushed onto the stack (%s from %s:%d) Reason: %s",
         ctx->ID, ctx->msgtype->name,
         xbt_socket_peer_name(ctx->expeditor),
         xbt_socket_peer_port(ctx->expeditor), e.msg);
    RETHROW;
  }

  xbt_mallocator_release(gras_msg_ctx_mallocator, ctx);
  if (received.kind == e_gras_msg_kind_rpcerror) {
    xbt_ex_t e;
    memcpy(&e, received.payl, received.payl_size);
    free(received.payl);
    XBT_VERB("Raise a remote exception cat:%d coming from %s (%s)",
             (int)e.category, e.host, e.msg);
    __xbt_running_ctx_fetch()->exception.msg = e.msg;
    __xbt_running_ctx_fetch()->exception.category = e.category;
    __xbt_running_ctx_fetch()->exception.value = e.value;
    __xbt_running_ctx_fetch()->exception.remote = 1;
    __xbt_running_ctx_fetch()->exception.host = e.host;
    __xbt_running_ctx_fetch()->exception.procname = e.procname;
    __xbt_running_ctx_fetch()->exception.pid = e.pid;
    __xbt_running_ctx_fetch()->exception.file = e.file;
    __xbt_running_ctx_fetch()->exception.line = e.line;
    __xbt_running_ctx_fetch()->exception.func = e.func;
    __xbt_running_ctx_fetch()->exception.used = e.used;
    __xbt_running_ctx_fetch()->exception.bt_strings = e.bt_strings;
    memset(&__xbt_running_ctx_fetch()->exception.bt, 0,
           sizeof(__xbt_running_ctx_fetch()->exception.bt));
    DO_THROW(__xbt_running_ctx_fetch());
  }
  memcpy(answer, received.payl, received.payl_size);
  free(received.payl);
}

/** @brief Conduct a RPC call */
void gras_msg_rpccall_(xbt_socket_t server,
                       double timeout,
                       gras_msgtype_t msgtype, void *request, void *answer)
{

  gras_msg_cb_ctx_t ctx;

  ctx = gras_msg_rpc_async_call_(server, timeout, msgtype, request);
  gras_msg_rpc_async_wait(ctx, answer);
}


/** @brief Return the result of a RPC call
 *
 * It done before the actual return of the callback so that the callback can do
 * some cleanups before leaving.
 */

void gras_msg_rpcreturn(double timeOut, gras_msg_cb_ctx_t ctx,
                        void *answer)
{
  xbt_assert(ctx->answer_due,
              "RPC return not allowed here. Either not a RPC message or already returned a result");
  ctx->answer_due = 0;
  XBT_DEBUG("Return to RPC '%s' from %s:%d (tOut=%f, payl=%p)",
         ctx->msgtype->name,
         xbt_socket_peer_name(ctx->expeditor),
         xbt_socket_peer_port(ctx->expeditor), timeOut, answer);
  gras_msg_send_ext(ctx->expeditor, e_gras_msg_kind_rpcanswer, ctx->ID,
                    ctx->msgtype, answer);
}
