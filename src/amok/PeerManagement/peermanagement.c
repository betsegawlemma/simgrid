/* amok peer management - servers main loop and remote peer stopping        */

/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/peer.h"
#include "amok/peermanagement.h"

#include "amok/amok_modinter.h" /* prototype of my module declaration */
#include "gras/module.h"        /* module mecanism */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(amok_pm, amok, "peer management");


/* data management */
int amok_pm_moddata_id = -1;
typedef struct {
  int done;
  xbt_dict_t groups;
} s_amok_pm_moddata_t, *amok_pm_moddata_t;

/* Message callbacks */
static int amok_pm_cb_kill(gras_msg_cb_ctx_t ctx, void *payload_data)
{

  amok_pm_moddata_t g = gras_moddata_by_id(amok_pm_moddata_id);
  g->done = 1;
  return 0;
}

static int amok_pm_cb_killrpc(gras_msg_cb_ctx_t ctx, void *payload_data)
{

  amok_pm_moddata_t g = gras_moddata_by_id(amok_pm_moddata_id);
  g->done = 1;
  gras_msg_rpcreturn(30, ctx, NULL);
  return 0;
}

static int amok_pm_cb_get(gras_msg_cb_ctx_t ctx, void *payload)
{
  amok_pm_moddata_t g = gras_moddata_by_id(amok_pm_moddata_id);
  char *name = *(void **) payload;
  xbt_dynar_t res = xbt_dict_get(g->groups, name);

  gras_msg_rpcreturn(30, ctx, &res);
  return 0;
}

static int amok_pm_cb_join(gras_msg_cb_ctx_t ctx, void *payload)
{
  amok_pm_moddata_t g = gras_moddata_by_id(amok_pm_moddata_id);
  char *group_name = *(char **) payload;
  xbt_dynar_t group = xbt_dict_get(g->groups, group_name);
  int rank;

  gras_socket_t exp = gras_msg_cb_ctx_from(ctx);
  xbt_peer_t dude = xbt_peer_new(gras_socket_peer_name(exp),
                                 gras_socket_peer_port(exp));

  rank = xbt_dynar_length(group);
  xbt_dynar_push(group, &dude);
  XBT_VERB("Contacted by %s:%d. Give it rank #%d", dude->name, dude->port,
        rank);

  gras_msg_rpcreturn(10, ctx, &rank);
  free(group_name);
  return 0;
}

static int amok_pm_cb_leave(gras_msg_cb_ctx_t ctx, void *payload)
{
  amok_pm_moddata_t g = gras_moddata_by_id(amok_pm_moddata_id);
  char *name = *(void **) payload;
  xbt_dynar_t group = xbt_dict_get(g->groups, name);

  gras_socket_t exp = gras_msg_cb_ctx_from(ctx);
  xbt_peer_t dude = xbt_peer_new(gras_socket_peer_name(exp),
                                 gras_socket_peer_port(exp));

  unsigned int cpt;
  xbt_peer_t peer_it;

  xbt_dynar_foreach(group, cpt, peer_it) {
    if (!strcmp(peer_it->name, dude->name) && peer_it->port == dude->port) {
      xbt_dynar_cursor_rm(group, &cpt);
      goto end;
    }
  }
  XBT_WARN("Asked to remove %s:%d from group '%s', but not found. Ignoring",
        dude->name, dude->port, name);

end:
  gras_msg_rpcreturn(30, ctx, NULL);
  return 0;
}

static int amok_pm_cb_shutdown(gras_msg_cb_ctx_t ctx, void *payload)
{
  char *name = *(void **) payload;
  amok_pm_group_shutdown(name);

  gras_msg_rpcreturn(30, ctx, NULL);
  return 0;
}

/** \brief Enter the main loop of the program. It won't return until we get a kill message. */
void amok_pm_mainloop(double timeOut)
{
  amok_pm_moddata_t g = gras_moddata_by_id(amok_pm_moddata_id);

  while (!g->done) {
    gras_msg_handle(timeOut);
  }
}

/** \brief kill a buddy identified by its peername and port. Note that it is not removed from any group it may belong to. */
void amok_pm_kill_hp(char *name, int port)
{
  gras_socket_t sock = gras_socket_client(name, port);
  amok_pm_kill(sock);
  gras_socket_close(sock);
}

/** \brief kill a buddy to which we have a socket already. Note that it is not removed from any group it may belong to. */
void amok_pm_kill(gras_socket_t buddy)
{
  gras_msg_send(buddy, "amok_pm_kill", NULL);
}

/** \brief kill syncronously a buddy (do not return before its death). Note that it is not removed from any group it may belong to. */
void amok_pm_kill_sync(gras_socket_t buddy)
{
  gras_msg_rpccall(buddy, 30, "amok_pm_killrpc", NULL, NULL);
}


/** \brief create a new peermanagement group located on local peer 
 *
 * The dynar elements are of type xbt_peer_t
 */
xbt_dynar_t amok_pm_group_new(const char *group_name)
{
  amok_pm_moddata_t g;
  xbt_dynar_t res = xbt_dynar_new(sizeof(xbt_peer_t),
                                  xbt_peer_free_voidp);

  xbt_assert(amok_pm_moddata_id != -1, "Run amok_pm_init first!");
  g = gras_moddata_by_id(amok_pm_moddata_id);

  XBT_DEBUG("retrieved groups=%p", g->groups);

  xbt_dict_set(g->groups, group_name, res, NULL);       /*FIXME: leaking xbt_dynar_free_voidp); */
  XBT_VERB("Group %s created", group_name);

  return res;
}

/** \brief retrieve all members of the given remote group */
xbt_dynar_t amok_pm_group_get(gras_socket_t master, const char *group_name)
{
  xbt_dynar_t res;

  gras_msg_rpccall(master, 30, "amok_pm_get", &group_name, &res);
  return res;
}

/** \brief add current peer to the given remote group 
 *
 * Returns the rank of the process in the group.
 */
int amok_pm_group_join(gras_socket_t master, const char *group_name)
{
  int rank;
  XBT_VERB("Join group '%s' on %s:%d",
        group_name, gras_socket_peer_name(master),
        gras_socket_peer_port(master));
  gras_msg_rpccall(master, 30, "amok_pm_join", &group_name, &rank);
  XBT_VERB("Joined group '%s' on %s:%d. Got rank %d",
        group_name, gras_socket_peer_name(master),
        gras_socket_peer_port(master), rank);
  return rank;
}

/** \brief remove current peer from the given remote group if found
 *
 * If not found, call is ignored 
 */
void amok_pm_group_leave(gras_socket_t master, const char *group_name)
{
  gras_msg_rpccall(master, 30, "amok_pm_leave", &group_name, NULL);
  XBT_VERB("Leaved group '%s' on %s:%d",
        group_name, gras_socket_peer_name(master),
        gras_socket_peer_port(master));
}

/** \brief stops all members of the given local group */
void amok_pm_group_shutdown(const char *group_name)
{
  amok_pm_moddata_t g = gras_moddata_by_id(amok_pm_moddata_id);
  xbt_dynar_t group = xbt_dict_get(g->groups, group_name);

  unsigned int cpt;
  xbt_peer_t peer_it;

  xbt_dynar_foreach(group, cpt, peer_it) {
    amok_pm_kill_hp(peer_it->name, peer_it->port);
  }

  xbt_dynar_free(&group);
  xbt_dict_remove(g->groups, group_name);
}

/** \brief stops all members of the given remote group */
void amok_pm_group_shutdown_remote(gras_socket_t master,
                                   const char *group_name)
{
  gras_msg_rpccall(master, 30, "amok_pm_shutdown", &group_name, NULL);
}


/* *
 * *
 * * Module management functions
 * *
 * */



static void _amok_pm_init(void)
{
  /* no world-wide globals */
  /* Datatype and message declarations */
  gras_datadesc_type_t pm_group_type =
      gras_datadesc_dynar(gras_datadesc_by_name("xbt_peer_t"),
                          xbt_peer_free_voidp);

  gras_msgtype_declare("amok_pm_kill", NULL);
  gras_msgtype_declare_rpc("amok_pm_killrpc", NULL, NULL);

  gras_msgtype_declare_rpc("amok_pm_get",
                           gras_datadesc_by_name("string"), pm_group_type);
  gras_msgtype_declare_rpc("amok_pm_join", gras_datadesc_by_name("string"),
                           gras_datadesc_by_name("int"));
  gras_msgtype_declare_rpc("amok_pm_leave",
                           gras_datadesc_by_name("string"), NULL);

  gras_msgtype_declare_rpc("amok_pm_shutdown",
                           gras_datadesc_by_name("string"), NULL);
}

static void _amok_pm_join(void *p)
{
  /* moddata management */
  amok_pm_moddata_t mod = (amok_pm_moddata_t) p;

  mod->groups = NULL;

  mod->done = 0;
  mod->groups = xbt_dict_new_homogeneous(NULL);

  /* callbacks */
  gras_cb_register("amok_pm_kill", &amok_pm_cb_kill);
  gras_cb_register("amok_pm_killrpc", &amok_pm_cb_killrpc);

  gras_cb_register("amok_pm_get", &amok_pm_cb_get);
  gras_cb_register("amok_pm_join", &amok_pm_cb_join);
  gras_cb_register("amok_pm_leave", &amok_pm_cb_leave);
  gras_cb_register("amok_pm_shutdown", &amok_pm_cb_shutdown);
}

static void _amok_pm_exit(void)
{
  /* no world-wide globals */
}

static void _amok_pm_leave(void *p)
{
  /* moddata */
  amok_pm_moddata_t mod = (amok_pm_moddata_t) p;

  xbt_dict_free(&mod->groups);

  /* callbacks */
  gras_cb_unregister("amok_pm_kill", &amok_pm_cb_kill);
  gras_cb_unregister("amok_pm_killrpc", &amok_pm_cb_killrpc);

  gras_cb_unregister("amok_pm_get", &amok_pm_cb_get);
  gras_cb_unregister("amok_pm_join", &amok_pm_cb_join);
  gras_cb_unregister("amok_pm_leave", &amok_pm_cb_leave);
  gras_cb_unregister("amok_pm_shutdown", &amok_pm_cb_shutdown);
}

void amok_pm_modulecreate()
{
  gras_module_add("amok_pm", sizeof(s_amok_pm_moddata_t),
                  &amok_pm_moddata_id, _amok_pm_init, _amok_pm_exit,
                  _amok_pm_join, _amok_pm_leave);
}



/* *
 * *
 * * Old module functions (kept for compatibility)
 * *
 * */
/** \brief Initialize the peer management module. Every process must run it before use */
void amok_pm_init()
{
  gras_module_join("amok_pm");
}

/** \brief Finalize the peer management module. Every process should run it after use */
void amok_pm_exit()
{
  gras_module_leave("amok_pm");
}
