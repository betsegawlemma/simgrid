/* $Id$ */

/* ddt_exchange - send/recv data described                                  */

/* Copyright (c) 2003 Olivier Aumage.                                       */
/* Copyright (c) 2003, 2004 Martin Quinson.                                 */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/DataDesc/datadesc_private.h"
#include "gras/Transport/transport_interface.h" /* gras_trp_chunk_send/recv */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ddt_exchange,datadesc,
				 "Sending data over the network");
const char *gras_datadesc_cat_names[9] = { 
  "undefined", 
  "scalar", "struct", "union", "ref", "array", "ignored",
  "invalid"};

static gras_datadesc_type_t int_type = NULL;
static gras_datadesc_type_t pointer_type = NULL;    
static _XBT_INLINE xbt_error_t gras_dd_send_int(gras_socket_t sock,             int  i);
static _XBT_INLINE xbt_error_t gras_dd_recv_int(gras_socket_t sock, int r_arch, int *i);

static _XBT_INLINE xbt_error_t
gras_dd_alloc_ref(xbt_dict_t  refs,  long int     size,
		  char       **r_ref, long int     r_len,
		  char	     **l_ref, int detect_cycle);

static _XBT_INLINE int
gras_dd_is_r_null(char **r_ptr, long int length);

static xbt_error_t 
gras_datadesc_send_rec(gras_socket_t         sock,
		       gras_cbps_t           state,
		       xbt_dict_t           refs,
		       gras_datadesc_type_t  type, 
		       char                 *data, 
		       int                   detect_cycle);
static xbt_error_t
gras_datadesc_recv_rec(gras_socket_t         sock, 
		       gras_cbps_t           state,
		       xbt_dict_t           refs,
		       gras_datadesc_type_t  type,
		       int                   r_arch,
		       char                **r_data,
		       long int              r_lgr,
		       char                 *dst,
		       int                   subsize, 
		       int                   detect_cycle);


static _XBT_INLINE xbt_error_t
gras_dd_send_int(gras_socket_t sock,int i) {

  if (!int_type) {
    int_type = gras_datadesc_by_name("int");
     xbt_assert(int_type);  
  }
   
  DEBUG1("send_int(%d)",i);
  return gras_trp_chunk_send(sock, (char*)&i, int_type->size[GRAS_THISARCH]);
}

static _XBT_INLINE xbt_error_t
gras_dd_recv_int(gras_socket_t sock, int r_arch, int *i) {
  xbt_error_t errcode;

  if (!int_type) {
     int_type = gras_datadesc_by_name("int");
     xbt_assert(int_type);
  }

  if (int_type->size[GRAS_THISARCH] >= int_type->size[r_arch]) {
    TRY(gras_trp_chunk_recv(sock, (char*)i, int_type->size[r_arch]));
    if (r_arch != GRAS_THISARCH)
      TRY(gras_dd_convert_elm(int_type,1,r_arch, i,i));
  } else {
    void *ptr = xbt_malloc(int_type->size[r_arch]);

    TRY(gras_trp_chunk_recv(sock, (char*)ptr, int_type->size[r_arch]));
    if (r_arch != GRAS_THISARCH)
      TRY(gras_dd_convert_elm(int_type,1,r_arch, ptr,i));
    xbt_free(ptr);
  }
  DEBUG1("recv_int(%d)",*i);

  return no_error;
}

/*
 * Note: here we suppose that the remote NULL is a sequence 
 *       of 'length' bytes set to 0.
 * FIXME: Check in configure?
 */
static _XBT_INLINE int 
gras_dd_is_r_null(char **r_ptr, long int length) {
  int i;

  for (i=0; i<length; i++) {
    if ( ((unsigned char*)r_ptr) [i]) {
      return 0;
    }
  }

  return 1;
}

static _XBT_INLINE xbt_error_t
gras_dd_alloc_ref(xbt_dict_t  refs,
		  long int     size,
		  char       **r_ref,
		  long int     r_len, /* pointer_type->size[r_arch] */
		  char	     **l_ref,
		  int          detect_cycle) {
  char *l_data = NULL;

  xbt_assert1(size>0,"Cannot allocate %ld bytes!", size);
  l_data = xbt_malloc((size_t)size);

  *l_ref = l_data;
  DEBUG2("l_data=%p, &l_data=%p",(void*)l_data,(void*)&l_data);

  DEBUG3("alloc_ref: r_ref=%p; *r_ref=%p, r_len=%ld",
	 (void*)r_ref, (void*)(r_ref?*r_ref:NULL), r_len);
  if (detect_cycle && r_ref && !gras_dd_is_r_null( r_ref, r_len)) {
    void *ptr = xbt_malloc(sizeof(void *));

    CRITICAL0("detect_cycle");
    memcpy(ptr,l_ref, sizeof(void *));

    DEBUG2("Insert %p under %p",*(void**)ptr, *(void**)r_ref);

    if (detect_cycle)
       xbt_dict_set_ext(refs,(const char *) r_ref, r_len, ptr, xbt_free_fct);
  }
  return no_error;
}

/**
 * gras_datadesc_type_cmp:
 *
 * Compares two datadesc types with the same semantic than strcmp.
 *
 * This comparison does not take the set headers into account (name and ID), 
 * but only the payload (actual type description).
 */
int gras_datadesc_type_cmp(const gras_datadesc_type_t d1,
			   const gras_datadesc_type_t d2) {
  int ret,cpt;
  gras_dd_cat_field_t field1,field2;
  gras_datadesc_type_t field_desc_1,field_desc_2;

  if (d1 == d2) return 0; /* easy optimization */

  if (!d1 && d2) {
    DEBUG0("ddt_cmp: !d1 && d2 => 1");
    return 1;
  }
  if (!d1 && !d2) {
    DEBUG0("ddt_cmp: !d1 && !d2 => 0");
    return 0;
  }
  if ( d1 && !d2) {
    DEBUG0("ddt_cmp: d1 && !d2 => -1");
    return -1;
  }

  for (cpt=0; cpt<gras_arch_count; cpt++) {
    if (d1->size[cpt] != d2->size[cpt]) {
      DEBUG5("ddt_cmp: %s->size=%ld  !=  %s->size=%ld (on %s)",
	     d1->name,d1->size[cpt],d2->name,d2->size[cpt],
	     gras_arches[cpt].name);
      return d1->size[cpt] >  d2->size[cpt] ? 1 : -1;
    }

    if (d1->alignment[cpt] != d2->alignment[cpt]) {
      DEBUG5("ddt_cmp: %s->alignment=%ld  !=  %s->alignment=%ld (on %s)",
	     d1->name,d1->alignment[cpt],d2->name,d2->alignment[cpt],
	     gras_arches[cpt].name);
      return d1->alignment[cpt] > d2->alignment[cpt] ? 1 : -1;
    }

    if (d1->aligned_size[cpt] != d2->aligned_size[cpt]) {
      DEBUG5("ddt_cmp: %s->aligned_size=%ld  !=  %s->aligned_size=%ld (on %s)",
	     d1->name,d1->aligned_size[cpt],d2->name,d2->aligned_size[cpt],
	     gras_arches[cpt].name);
      return d1->aligned_size[cpt] > d2->aligned_size[cpt] ? 1 : -1;
    }
  }

  if (d1->category_code != d2->category_code) {
    DEBUG4("ddt_cmp: %s->cat=%s  !=  %s->cat=%s",
	   d1->name,gras_datadesc_cat_names[d1->category_code],
	   d2->name,gras_datadesc_cat_names[d2->category_code]);
    return d1->category_code > d2->category_code ? 1 : -1;
  }

  if (d1->send != d2->send) {
    DEBUG4("ddt_cmp: %s->send=%p  !=  %s->send=%p",
	   d1->name,(void*)d1->send, d2->name,(void*)d2->send);
    return 1; /* ISO C forbids ordered comparisons of pointers to functions */
  }

  if (d1->recv != d2->recv) {
    DEBUG4("ddt_cmp: %s->recv=%p  !=  %s->recv=%p",
	   d1->name,(void*)d1->recv, d2->name,(void*)d2->recv);
    return 1; /* ISO C forbids ordered comparisons of pointers to functions */
  }

  switch (d1->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    if (d1->category.scalar_data.encoding != d2->category.scalar_data.encoding)
      return d1->category.scalar_data.encoding > d2->category.scalar_data.encoding ? 1 : -1 ;
    break;
    
  case e_gras_datadesc_type_cat_struct:    
    if (xbt_dynar_length(d1->category.struct_data.fields) != 
	xbt_dynar_length(d2->category.struct_data.fields)) {
      DEBUG4("ddt_cmp: %s (having %lu fields) !=  %s (having %lu fields)",
	     d1->name, xbt_dynar_length(d1->category.struct_data.fields),
	     d2->name, xbt_dynar_length(d2->category.struct_data.fields));
      
      return xbt_dynar_length(d1->category.struct_data.fields) >
	xbt_dynar_length(d2->category.struct_data.fields) ?
	1 : -1;
    }
    xbt_dynar_foreach(d1->category.struct_data.fields, cpt, field1) {
      
      field2 = xbt_dynar_get_as(d2->category.struct_data.fields, cpt, gras_dd_cat_field_t);
      field_desc_1 = field1->type;
      field_desc_2 = field2->type;
      ret = gras_datadesc_type_cmp(field_desc_1,field_desc_2);
      if (ret) {
	DEBUG6("%s->field[%d]=%s != %s->field[%d]=%s",
	       d1->name,cpt,field1->name, 	       
	       d2->name,cpt,field2->name);
	return ret;
      }
      
    }
    break;
    
  case e_gras_datadesc_type_cat_union:
    if (d1->category.union_data.selector != d2->category.union_data.selector) 
      return 1;  /* ISO C forbids ordered comparisons of pointers to functions */
    
    if (xbt_dynar_length(d1->category.union_data.fields) != 
	xbt_dynar_length(d2->category.union_data.fields))
      return xbt_dynar_length(d1->category.union_data.fields) >
	     xbt_dynar_length(d2->category.union_data.fields) ?
	1 : -1;
    
    xbt_dynar_foreach(d1->category.union_data.fields, cpt, field1) {
      
      field2 = xbt_dynar_get_as(d2->category.union_data.fields, cpt, gras_dd_cat_field_t);
      field_desc_1 = field1->type;
      field_desc_2 = field2->type;
      ret = gras_datadesc_type_cmp(field_desc_1,field_desc_2);
      if (ret)
	return ret;
      
    }
    break;
    
    
  case e_gras_datadesc_type_cat_ref:
    if (d1->category.ref_data.selector != d2->category.ref_data.selector) 
      return 1; /* ISO C forbids ordered comparisons of pointers to functions */
    
    if (d1->category.ref_data.type != d2->category.ref_data.type) 
      return d1->category.ref_data.type > d2->category.ref_data.type ? 1 : -1;
    break;
    
  case e_gras_datadesc_type_cat_array:
    if (d1->category.array_data.type != d2->category.array_data.type) 
      return d1->category.array_data.type > d2->category.array_data.type ? 1 : -1;
    
    if (d1->category.array_data.fixed_size != d2->category.array_data.fixed_size) 
      return d1->category.array_data.fixed_size > d2->category.array_data.fixed_size ? 1 : -1;
    
    if (d1->category.array_data.dynamic_size != d2->category.array_data.dynamic_size) 
      return 1; /* ISO C forbids ordered comparisons of pointers to functions */
    
    break;
    
  case e_gras_datadesc_type_cat_ignored:
    /* That's ignored... */
  default:
    /* two stupidly created ddt are equally stupid ;) */
    break;
  }
  return 0;
  
}

/**
 * gras_datadesc_cpy:
 *
 * Copy the data pointed by src and described by type 
 * to a new location, and store a pointer to it in dst.
 *
 */
xbt_error_t gras_datadesc_cpy(gras_datadesc_type_t type, 
			       void *src, 
			       void **dst) {
  RAISE_UNIMPLEMENTED;
}

static xbt_error_t 
gras_datadesc_send_rec(gras_socket_t         sock,
		       gras_cbps_t           state,
		       xbt_dict_t           refs,
		       gras_datadesc_type_t  type, 
		       char                 *data,
		       int                   detect_cycle) {

  xbt_error_t         errcode;
  int                  cpt;
  gras_datadesc_type_t sub_type; /* type on which we recurse */
  
  VERB2("Send a %s (%s)", 
	type->name, gras_datadesc_cat_names[type->category_code]);

  if (type->send) {
    type->send(state,data);
  }

  switch (type->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    TRY(gras_trp_chunk_send(sock, data, type->size[GRAS_THISARCH]));
    break;

  case e_gras_datadesc_type_cat_struct: {
    gras_dd_cat_struct_t struct_data;
    gras_dd_cat_field_t  field;
    char                *field_data;
    
    struct_data = type->category.struct_data;
    xbt_assert1(struct_data.closed,
      "Please call gras_datadesc_declare_struct_close on %s before sending it",
		type->name);
    VERB1(">> Send all fields of the structure %s",type->name);
    xbt_dynar_foreach(struct_data.fields, cpt, field) {
      field_data = data;
      field_data += field->offset[GRAS_THISARCH];
      
      sub_type = field->type;
      
      if (field->pre)
	field->pre(state,field_data);
      
      VERB1("Send field %s",field->name);
      TRY(gras_datadesc_send_rec(sock,state,refs,sub_type, field_data, detect_cycle || sub_type->cycle));
      
      if (field->post)
	field->post(state,field_data);
    }
    VERB1("<< Sent all fields of the structure %s", type->name);
    
    break;
  }

  case e_gras_datadesc_type_cat_union: {
    gras_dd_cat_union_t union_data;
    gras_dd_cat_field_t field=NULL;
    int                 field_num;
    
    union_data = type->category.union_data;
    
    xbt_assert1(union_data.closed,
		"Please call gras_datadesc_declare_union_close on %s before sending it",
		type->name);
    /* retrieve the field number */
    field_num = union_data.selector(state, data);
    
    xbt_assert1(field_num > 0,
		 "union field selector of %s gave a negative value", 
		 type->name);
    
    xbt_assert3(field_num < xbt_dynar_length(union_data.fields),
	 "union field selector of %s returned %d but there is only %lu fields",
		 type->name, field_num, xbt_dynar_length(union_data.fields));

    /* Send the field number */
    TRY(gras_dd_send_int(sock, field_num));
    
    /* Send the content */
    field = xbt_dynar_get_as(union_data.fields, field_num, gras_dd_cat_field_t);
    sub_type = field->type;
    
    if (field->pre)
      field->pre(state,data);
    
    TRY(gras_datadesc_send_rec(sock,state,refs, sub_type, data, detect_cycle || sub_type->cycle));
      
    if (field->post)
      field->post(state,data);
    
    break;
  }
    
  case e_gras_datadesc_type_cat_ref: {
    gras_dd_cat_ref_t      ref_data;

    void                 **ref=(void**)data;
    void *dummy;
    
    ref_data = type->category.ref_data;
    
    /* Detect the referenced type and send it to peer if needed */
    sub_type = ref_data.type;
    if (sub_type == NULL) {
      sub_type = (*ref_data.selector)(state,data);
      TRY(gras_dd_send_int(sock, sub_type->code));
    }
    
    /* Send the actual value of the pointer for cycle handling */
    if (!pointer_type) {
      pointer_type = gras_datadesc_by_name("data pointer");
      xbt_assert(pointer_type);
    }
     
    TRY(gras_trp_chunk_send(sock, (char*)data,
			    pointer_type->size[GRAS_THISARCH]));
    
    /* Send the pointed data only if not already sent */
    if (*(void**)data == NULL) {
      VERB0("Not sending NULL referenced data");
      break;
    }
    errcode = detect_cycle 
            ? xbt_dict_get_ext(refs,(char*)ref, sizeof(void*), &dummy)
            : mismatch_error;
    if (errcode == mismatch_error) {
       VERB1("Sending data referenced at %p", (void*)*ref);
       if (detect_cycle)
	 xbt_dict_set_ext(refs, (char*)ref, sizeof(void*), ref, NULL);
       TRY(gras_datadesc_send_rec(sock,state,refs, sub_type, *ref, detect_cycle || sub_type->cycle));
	  
    } else if (errcode == no_error) {
       VERB1("Not sending data referenced at %p (already done)", (void*)*ref);
    } else {
       return errcode;
    }
    
    break;
  }

  case e_gras_datadesc_type_cat_array: {
    gras_dd_cat_array_t    array_data;
    long int               count;
    char                  *ptr=data;
    long int               elm_size;
    
    array_data = type->category.array_data;
    
    /* determine and send the element count */
    count = array_data.fixed_size;
    if (count == 0) {
      count = array_data.dynamic_size(state,data);
      xbt_assert1(count >=0,
		   "Invalid (negative) array size for type %s",type->name);
      TRY(gras_dd_send_int(sock, count));
    }
    
    /* send the content */
    sub_type = array_data.type;
    elm_size = sub_type->aligned_size[GRAS_THISARCH];
    if (sub_type->category_code == e_gras_datadesc_type_cat_scalar) {
      VERB1("Array of %ld scalars, send it in one shot",count);
      TRY(gras_trp_chunk_send(sock, data, 
			      sub_type->aligned_size[GRAS_THISARCH] * count));
    } else if (sub_type->category_code == e_gras_datadesc_type_cat_array &&
	       sub_type->category.array_data.fixed_size > 0 &&
	       sub_type->category.array_data.type->category_code == e_gras_datadesc_type_cat_scalar) {
       
      VERB1("Array of %ld fixed array of scalars, send it in one shot",count);
      TRY(gras_trp_chunk_send(sock, data, 
			      sub_type->category.array_data.type->aligned_size[GRAS_THISARCH] 
			         * count * sub_type->category.array_data.fixed_size));
       
    } else {
      for (cpt=0; cpt<count; cpt++) {
	TRY(gras_datadesc_send_rec(sock,state,refs, sub_type, ptr, detect_cycle || sub_type->cycle));
	ptr += elm_size;
      }
    }
    break;
  }

  default:
    xbt_assert0(0, "Invalid type");
  }

  return no_error;
}

/**
 * gras_datadesc_send:
 *
 * Copy the data pointed by src and described by type to the socket
 *
 */
xbt_error_t gras_datadesc_send(gras_socket_t        sock, 
				gras_datadesc_type_t type, 
				void *src) {

  xbt_error_t errcode;
  gras_cbps_t  state;
  xbt_dict_t  refs; /* all references already sent */
 
  refs = xbt_dict_new();
  state = gras_cbps_new();
   
  errcode = gras_datadesc_send_rec(sock,state,refs,type,(char*)src, type->cycle);

  xbt_dict_free(&refs);
  gras_cbps_free(&state);

  return errcode;
}

/**
 * gras_datadesc_recv_rec:
 *
 * Do the data reception job recursively.
 *
 * subsize used only to deal with vicious case of reference to dynamic array.
 *  This size is needed at the reference reception level (to allocate enough 
 * space) and at the array reception level (to fill enough room). 
 * 
 * Having this size passed as an argument of the recursive function is a crude
 * hack, but I was told that working code is sometimes better than neat one ;)
 */
xbt_error_t
gras_datadesc_recv_rec(gras_socket_t         sock, 
		       gras_cbps_t           state,
		       xbt_dict_t           refs,
		       gras_datadesc_type_t  type,
		       int                   r_arch,
		       char                **r_data,
		       long int              r_lgr,
		       char                 *l_data,
		       int                   subsize,
		       int                   detect_cycle) {

  xbt_error_t         errcode;
  int                  cpt;
  gras_datadesc_type_t sub_type;

  VERB2("Recv a %s @%p", type->name, (void*)l_data);
  xbt_assert(l_data);

  switch (type->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    if (type->size[GRAS_THISARCH] == type->size[r_arch]) {
      TRY(gras_trp_chunk_recv(sock, (char*)l_data, type->size[r_arch]));
      if (r_arch != GRAS_THISARCH)
	TRY(gras_dd_convert_elm(type,1,r_arch, l_data,l_data));
    } else {
      void *ptr = xbt_malloc(type->size[r_arch]);

      TRY(gras_trp_chunk_recv(sock, (char*)ptr, type->size[r_arch]));
      if (r_arch != GRAS_THISARCH)
	TRY(gras_dd_convert_elm(type,1,r_arch, ptr,l_data));
      xbt_free(ptr);
    }
    break;

  case e_gras_datadesc_type_cat_struct: {
    gras_dd_cat_struct_t struct_data;
    gras_dd_cat_field_t  field;

    struct_data = type->category.struct_data;

    xbt_assert1(struct_data.closed,
		"Please call gras_datadesc_declare_struct_close on %s before receiving it",
		type->name);
    VERB1(">> Receive all fields of the structure %s",type->name);
    xbt_dynar_foreach(struct_data.fields, cpt, field) {
      char                 *field_data = l_data + field->offset[GRAS_THISARCH];

      sub_type = field->type;

      TRY(gras_datadesc_recv_rec(sock,state,refs, sub_type,
				 r_arch,NULL,0,
				 field_data,-1, 
				 detect_cycle || sub_type->cycle));
    }
    VERB1("<< Received all fields of the structure %s", type->name);
    
    break;
  }

  case e_gras_datadesc_type_cat_union: {
    gras_dd_cat_union_t union_data;
    gras_dd_cat_field_t field=NULL;
    int                 field_num;

    union_data = type->category.union_data;

    xbt_assert1(union_data.closed,
		"Please call gras_datadesc_declare_union_close on %s before receiving it",
		type->name);
    /* retrieve the field number */
    TRY(gras_dd_recv_int(sock, r_arch, &field_num));
    if (field_num < 0)
      RAISE1(mismatch_error,
	     "Received union field for %s is negative", type->name);
    if (field_num < xbt_dynar_length(union_data.fields)) 
      RAISE3(mismatch_error,
	     "Received union field for %s is %d but there is only %lu fields",
	     type->name, field_num, xbt_dynar_length(union_data.fields));
    
    /* Recv the content */
    field = xbt_dynar_get_as(union_data.fields, field_num, gras_dd_cat_field_t);
    sub_type = field->type;
    
    TRY(gras_datadesc_recv_rec(sock,state,refs, sub_type,
			       r_arch,NULL,0,
			       l_data,-1, 
			       detect_cycle || sub_type->cycle));
    break;
  }

  case e_gras_datadesc_type_cat_ref: {
    char             **r_ref = NULL;
    char             **l_ref = NULL;
    gras_dd_cat_ref_t  ref_data;
    
    ref_data = type->category.ref_data;

    /* Get the referenced type locally or from peer */
    sub_type = ref_data.type;
    if (sub_type == NULL) {
      int ref_code;
      TRY(gras_dd_recv_int(sock, r_arch, &ref_code));
      TRY(gras_datadesc_by_id(ref_code, &sub_type));
    }

    /* Get the actual value of the pointer for cycle handling */
    if (!pointer_type) {
      pointer_type = gras_datadesc_by_name("data pointer");
      xbt_assert(pointer_type);
    }

    r_ref = xbt_malloc(pointer_type->size[r_arch]);

    TRY(gras_trp_chunk_recv(sock, (char*)r_ref,
			    pointer_type->size[r_arch]));

    /* Receive the pointed data only if not already sent */
    if (gras_dd_is_r_null(r_ref, pointer_type->size[r_arch])) {
      VERB1("Not receiving data remotely referenced @%p since it's NULL",
	    *(void **)r_ref);
      *(void**)l_data = NULL;
      xbt_free(r_ref);
      break;
    }
         
    errcode = detect_cycle
            ? xbt_dict_get_ext(refs,
				(char*)r_ref, pointer_type->size[r_arch],
				(void**)&l_ref)
            : mismatch_error;

    if (errcode == mismatch_error) {
      int subsubcount = 0;
      void *l_referenced=NULL;

      VERB2("Receiving a ref to '%s', remotely @%p",
	    sub_type->name, *(void**)r_ref);
      if (sub_type->category_code == e_gras_datadesc_type_cat_array) {
	/* Damn. Reference to a dynamic array. Allocating the size for it 
	   is more complicated */
	gras_dd_cat_array_t array_data = sub_type->category.array_data;
	gras_datadesc_type_t subsub_type;

	subsubcount = array_data.fixed_size;
	if (subsubcount == 0)
	  TRY(gras_dd_recv_int(sock, r_arch, &subsubcount));

	subsub_type = array_data.type;


	TRY(gras_dd_alloc_ref(refs,
			      subsub_type->size[GRAS_THISARCH] * subsubcount, 
			      r_ref,pointer_type->size[r_arch], 
			      (char**)&l_referenced,
			      detect_cycle));
      } else {
	TRY(gras_dd_alloc_ref(refs,sub_type->size[GRAS_THISARCH], 
			      r_ref,pointer_type->size[r_arch], 
			      (char**)&l_referenced,
			      detect_cycle));
      }

      TRY(gras_datadesc_recv_rec(sock,state,refs, sub_type,
				 r_arch,r_ref,pointer_type->size[r_arch],
				 (char*)l_referenced, subsubcount, 
				 detect_cycle || sub_type->cycle));
      *(void**)l_data=l_referenced;
      VERB3("'%s' remotely referenced at %p locally at %p",
	    sub_type->name, *(void**)r_ref, l_referenced);
      
    } else if (errcode == no_error) {
      VERB2("NOT receiving data remotely referenced @%p (already done, @%p here)",
	    *(void**)r_ref, *(void**)l_ref);

      *(void**)l_data=*l_ref;

    } else {
      return errcode;
    }
    xbt_free(r_ref);
    break;
  }

  case e_gras_datadesc_type_cat_array: {
    gras_dd_cat_array_t    array_data;
    int       count;
    char     *ptr;
    long int  elm_size;

    array_data = type->category.array_data;
    /* determine element count locally, or from caller, or from peer */
    count = array_data.fixed_size;
    if (count == 0)
      count = subsize;
    if (count == 0)
      TRY(gras_dd_recv_int(sock, r_arch, &count));
    if (count == 0)
      RAISE1(mismatch_error,
	     "Invalid (=0) array size for type %s",type->name);

    /* receive the content */
    sub_type = array_data.type;
    if (sub_type->category_code == e_gras_datadesc_type_cat_scalar) {
      VERB1("Array of %d scalars, get it in one shoot", count);
      if (sub_type->aligned_size[GRAS_THISARCH] >= 
	  sub_type->aligned_size[r_arch]) {
	TRY(gras_trp_chunk_recv(sock, (char*)l_data, 
				sub_type->aligned_size[r_arch] * count));
	if (r_arch != GRAS_THISARCH)
	  TRY(gras_dd_convert_elm(sub_type,count,r_arch, l_data,l_data));
      } else {
	ptr = xbt_malloc(sub_type->aligned_size[r_arch] * count);

	TRY(gras_trp_chunk_recv(sock, (char*)ptr, 
				sub_type->size[r_arch] * count));
	if (r_arch != GRAS_THISARCH)
	  TRY(gras_dd_convert_elm(sub_type,count,r_arch, ptr,l_data));
	xbt_free(ptr);
      }
    } else if (sub_type->category_code == e_gras_datadesc_type_cat_array &&
	       sub_type->category.array_data.fixed_size > 0 &&
	       sub_type->category.array_data.type->category_code == e_gras_datadesc_type_cat_scalar) {
      gras_datadesc_type_t subsub_type;
      array_data = sub_type->category.array_data;
      subsub_type = array_data.type;
       
      VERB1("Array of %d fixed array of scalars, get it in one shot",count);
      if (subsub_type->aligned_size[GRAS_THISARCH] >= 
	  subsub_type->aligned_size[r_arch]) {
	TRY(gras_trp_chunk_recv(sock, (char*)l_data, 
				subsub_type->aligned_size[r_arch] * count * 
				  array_data.fixed_size));
	if (r_arch != GRAS_THISARCH)
	  TRY(gras_dd_convert_elm(subsub_type,count*array_data.fixed_size,r_arch, l_data,l_data));
      } else {
	ptr = xbt_malloc(subsub_type->aligned_size[r_arch] * count*array_data.fixed_size);

	TRY(gras_trp_chunk_recv(sock, (char*)ptr, 
				subsub_type->size[r_arch] * count*array_data.fixed_size));
	if (r_arch != GRAS_THISARCH)
	  TRY(gras_dd_convert_elm(subsub_type,count*array_data.fixed_size,r_arch, ptr,l_data));
	xbt_free(ptr);
      }
      
       
    } else {
      /* not scalar content, get it recursively (may contain pointers) */
      elm_size = sub_type->aligned_size[GRAS_THISARCH];
      VERB2("Receive a %d-long array of %s",count, sub_type->name);

      ptr = l_data;
      for (cpt=0; cpt<count; cpt++) {
	TRY(gras_datadesc_recv_rec(sock,state,refs, sub_type,
				   r_arch, NULL, 0, ptr,-1,
				   detect_cycle || sub_type->cycle));
	ptr += elm_size;
      }
    }
    break;
  }
        
  default:
    xbt_assert0(0, "Invalid type");
  }
  
  if (type->recv)
    type->recv(state,l_data);

  return no_error;
}

/**
 * gras_datadesc_recv:
 *
 * Get an instance of the datatype described by @type from the @socket, 
 * and store a pointer to it in @dst
 *
 */
xbt_error_t
gras_datadesc_recv(gras_socket_t         sock, 
		   gras_datadesc_type_t  type,
		   int                   r_arch,
		   void                 *dst) {

  xbt_error_t errcode;
  gras_cbps_t  state; /* callback persistent state */
  xbt_dict_t  refs;  /* all references already sent */

  refs = xbt_dict_new();
  state = gras_cbps_new();

  errcode = gras_datadesc_recv_rec(sock, state, refs, type, 
				   r_arch, NULL, 0,
				   (char *) dst,-1, 
				   type->cycle);

  xbt_dict_free(&refs);
  gras_cbps_free(&state);

  return errcode;
}
