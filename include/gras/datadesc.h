/* $Id$ */

/* gras/datadesc.h - Describing the data you want to exchange               */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_DATADESC_H
#define GRAS_DATADESC_H

#include "xbt/misc.h" /* BEGIN_DECL */

BEGIN_DECL

/**
 * gras_datadesc_type_t:
 * 
 * Opaque type describing a type description you don't want to open.
 */
typedef struct s_gras_datadesc_type *gras_datadesc_type_t;

typedef struct s_gras_cbps *gras_cbps_t;

/* callbacks prototypes */
typedef void (*gras_datadesc_type_cb_void_t)(gras_cbps_t vars, void *data);
typedef int (*gras_datadesc_type_cb_int_t)(gras_cbps_t vars, void *data);
typedef gras_datadesc_type_t (*gras_datadesc_selector_t)(gras_cbps_t vars, void *data);

/***********************************************
 **** Search and retrieve declared datatype ****
 ***********************************************/
gras_datadesc_type_t gras_datadesc_by_name(const char *name);

/******************************************
 **** Declare datadescription yourself ****
 ******************************************/

gras_datadesc_type_t gras_datadesc_struct(const char *name);

void
  gras_datadesc_struct_append(gras_datadesc_type_t  struct_type,
			      const char           *name,
			      gras_datadesc_type_t  field_type);
void
  gras_datadesc_struct_close(gras_datadesc_type_t   struct_type);

gras_datadesc_type_t 
  gras_datadesc_union(const char                   *name,
		      gras_datadesc_type_cb_int_t   selector);
void
  gras_datadesc_union_append(gras_datadesc_type_t   union_type,
			     const char            *name,
			     gras_datadesc_type_t   field_type);
void
  gras_datadesc_union_close(gras_datadesc_type_t    union_type);

gras_datadesc_type_t 
  gras_datadesc_ref(const char                     *name,
		    gras_datadesc_type_t            referenced_type);
gras_datadesc_type_t 
  gras_datadesc_ref_generic(const char                *name,
			    gras_datadesc_selector_t   selector);

gras_datadesc_type_t 
  gras_datadesc_array_fixed(const char             *name,
			    gras_datadesc_type_t    element_type,
			    long int                fixed_size);
gras_datadesc_type_t 
  gras_datadesc_array_dyn(const char                 *name,
			  gras_datadesc_type_t        element_type,
			  gras_datadesc_type_cb_int_t dynamic_size);

gras_datadesc_type_t 
  gras_datadesc_ref_pop_arr(gras_datadesc_type_t  element_type);

/*********************************
 * Change stuff within datadescs *
 *********************************/

void gras_datadesc_cycle_set(gras_datadesc_type_t type);
void gras_datadesc_cycle_unset(gras_datadesc_type_t type);

void gras_datadesc_cb_send (gras_datadesc_type_t         type,
			    gras_datadesc_type_cb_void_t pre);
void gras_datadesc_cb_recv(gras_datadesc_type_t          type,
			   gras_datadesc_type_cb_void_t  post);
void gras_datadesc_cb_field_send (gras_datadesc_type_t   type,
				  const char            *field_name,
				  gras_datadesc_type_cb_void_t  pre);
void gras_datadesc_cb_field_recv(gras_datadesc_type_t    type,
				 const char             *field_name,
				 gras_datadesc_type_cb_void_t  post);
void gras_datadesc_cb_field_push (gras_datadesc_type_t   type,
				  const char            *field_name);

/******************************
 * Get stuff within datadescs *
 ******************************/
char * gras_datadesc_get_name(gras_datadesc_type_t ddt);
int gras_datadesc_get_id(gras_datadesc_type_t ddt);

/********************************************************
 * Advanced data describing: callback persistent states *
 ********************************************************/
/* simple one: push/pop sizes of arrays */
void
gras_cbps_i_push(gras_cbps_t ps, int val);
int 
gras_cbps_i_pop(gras_cbps_t ps);

int gras_datadesc_cb_pop(gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_int(gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_uint(gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_lint(gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_ulint(gras_cbps_t vars, void *data);



/* complex one: complete variable environment support */
xbt_error_t
  gras_cbps_v_pop (gras_cbps_t            ps, 
		   const char            *name,
      	 /* OUT */ gras_datadesc_type_t  *ddt,
	 /* OUT */ void                 **res);
xbt_error_t
gras_cbps_v_push(gras_cbps_t            ps,
		 const char            *name,
		 void                  *data,
		 gras_datadesc_type_t   ddt);
void
gras_cbps_v_set (gras_cbps_t            ps,
		 const char            *name,
		 void                  *data,
		 gras_datadesc_type_t   ddt);

void *
gras_cbps_v_get (gras_cbps_t            ps, 
		 const char            *name,
       /* OUT */ gras_datadesc_type_t  *ddt);

void
gras_cbps_block_begin(gras_cbps_t ps);
void
gras_cbps_block_end(gras_cbps_t ps);




/*******************************
 **** About data convertion ****
 *******************************/
int gras_arch_selfid(void); /* ID of this arch */

/****************************
 **** Parse C statements ****
 ****************************/
gras_datadesc_type_t 
gras_datadesc_parse(const char *name,
		    const char *Cdefinition);
#define GRAS_DEFINE_TYPE(name,def) \
  static const char * _gras_this_type_symbol_does_not_exist__##name=#def; def
#define GRAS_ANNOTE(key,val)
 
#define gras_datadesc_by_symbol(name)  \
  (gras_datadesc_by_name(#name) ?      \
   gras_datadesc_by_name(#name) :      \
     gras_datadesc_parse(#name,        \
			 _gras_this_type_symbol_does_not_exist__##name) \
  )

/*****************************
 **** NWS datadescription ****
 *****************************/

/**
 * Basic types we can embeed in DataDescriptors.
 */
typedef enum
  {CHAR_TYPE, DOUBLE_TYPE, FLOAT_TYPE, INT_TYPE, LONG_TYPE, SHORT_TYPE,
   UNSIGNED_INT_TYPE, UNSIGNED_LONG_TYPE, UNSIGNED_SHORT_TYPE, STRUCT_TYPE}
  DataTypes;
#define SIMPLE_TYPE_COUNT 9

/*!  \brief Describe a collection of data.
 * 
** A description of a collection of #type# data.  #repetitions# is used only
** for arrays; it contains the number of elements.  #offset# is used only for
** struct members in host format; it contains the offset of the member from the
** beginning of the struct, taking into account internal padding added by the
** compiler for alignment purposes.  #members#, #length#, and #tailPadding# are
** used only for STRUCT_TYPE data; the #length#-long array #members# describes
** the members of the nested struct, and #tailPadding# indicates how many
** padding bytes the compiler adds to the end of the structure.
*/

typedef struct DataDescriptorStruct {
  DataTypes type;
  size_t repetitions;
  size_t offset;
  /*@null@*/ struct DataDescriptorStruct *members;
  size_t length;
  size_t tailPadding;
} DataDescriptor;
/** DataDescriptor for an array */
#define SIMPLE_DATA(type,repetitions) \
  {type, repetitions, 0, NULL, 0, 0}
/** DataDescriptor for an structure member */
#define SIMPLE_MEMBER(type,repetitions,offset) \
  {type, repetitions, offset, NULL, 0, 0}
/** DataDescriptor for padding bytes */
#define PAD_BYTES(structType,lastMember,memberType,repetitions) \
  sizeof(structType) - offsetof(structType, lastMember) - \
  sizeof(memberType) * repetitions

xbt_error_t
gras_datadesc_import_nws(const char           *name,
			 const DataDescriptor *desc,
			 unsigned long         howmany,
	       /* OUT */ gras_datadesc_type_t *dst);

END_DECL

#endif /* GRAS_DATADESC_H */
