/* $Id$ */

/* messaging - high level communication (send/receive messages)             */

/* module's private interface masked even to other parts of GRAS.           */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_MESSAGE_PRIVATE_H
#define GRAS_MESSAGE_PRIVATE_H

#include "gras_config.h"

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/dynar.h"
#include "xbt/set.h"
#include "gras/transport.h"
#include "gras/datadesc.h"
#include "gras/virtu.h"

#include "gras/messages.h"
#include "gras_modinter.h"

#include "gras/Msg/msg_interface.h"

/**
 * gras_msgtype_t:
 *
 * Message type descriptor. There one of these for each registered version.
 */
typedef struct s_gras_msgtype {
  /* headers for the data set */
  unsigned int   code;
  char          *name;
  unsigned int   name_len;
        
  /* payload */
  short int version;
  gras_datadesc_type_t ctn_type;
} s_gras_msgtype_t;

extern xbt_set_t _gras_msgtype_set; /* of gras_msgtype_t */
void gras_msgtype_free(void *msgtype);


xbt_error_t gras_msg_recv(gras_socket_t    sock,
			   gras_msgtype_t  *msgtype,
			   void           **payload,
			   int             *payload_size);

/**
 * gras_cblist_t:
 *
 * association between msg ID and cb list for a given process
 */
struct s_gras_cblist {
  long int id;
  xbt_dynar_t cbs; /* of gras_msg_cb_t */
};

void gras_cblist_free(void *cbl);

#endif  /* GRAS_MESSAGE_PRIVATE_H */
