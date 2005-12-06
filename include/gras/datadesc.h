/* $Id$ */

/* gras/datadesc.h - Describing the data you want to exchange               */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_DATADESC_H
#define GRAS_DATADESC_H

#include "xbt/misc.h" /* SG_BEGIN_DECL */
#include "xbt/dynar.h" /* void_f_pvoid_t */

SG_BEGIN_DECL()

/** @addtogroup GRAS_dd Data description
 *  @brief Describing data to be exchanged (Communication facility)
 *
 * Since GRAS takes care of potential representation conversion when the platform is heterogeneous, 
 * any data which transits on the network must be described beforehand.
 * 
 * There is several possible interfaces for this, ranging from the really completely automatic parsing to 
 * completely manual. Let's study each of them from the simplest to the more advanced:
 * 
 *   - Section \ref GRAS_dd_basic presents how to retrieve and use an already described type.
 *   - Section \ref GRAS_dd_auto shows how to get GRAS parsing your type description automagically. This
 *     is unfortunately not always possible (only works for some structures), but if it is for your data,
 *     this is definitly the way to go.
 *   - Section \ref GRAS_dd_manual presents how to build a description manually. This is useful when you want
 *     to describe an array or a pointer of pre-defined structures.
 *   - You sometimes need to exchange informations between descriptions at send or receive time. This is 
 *     for example useful when your structure contains an array which size is given by another field of the 
 *     structure.
 *     - Section \ref GRAS_dd_cb_simple provides a simple interface to do so, allowing to share integers stored on a stack.
 *     - Section \ref GRAS_dd_cb_full provides a full featured interface to do so, but it may reveal somehow difficult to use.
 **/

/** @defgroup GRAS_dd_basic Basic operations on data descriptions
 *  @ingroup GRAS_dd
 * \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Basics" --> \endhtmlonly
 *
 * If you only want to send pre-existing types, simply retrieve the pre-defined description with 
 * the \ref gras_datadesc_by_name function. Existing types entail:
 *  - char (both signed and unsigned)
 *  - int (short, regular, long and long long, both signed and unsigned)
 *  - float and double
 *  - string (which is indeed a reference to a dynamically sized array of char, strlen being used to retrive the size)
 * 
 * Example:\verbatim gras_datadesc_type_t i = gras_datadesc_by_name("int");
 gras_datadesc_type_t uc = gras_datadesc_by_name("unsigned char");
 gras_datadesc_type_t str = gras_datadesc_by_name("string");\endverbatim
 *
 */
/* @{ */
  
/** @brief Opaque type describing a type description. */
typedef struct s_gras_datadesc_type *gras_datadesc_type_t;

/** \brief Search a type description from its name */
gras_datadesc_type_t gras_datadesc_by_name(const char *name);

/* @} */
    
/** @defgroup GRAS_dd_auto Automatic parsing of data descriptions
 *  @ingroup GRAS_dd
 * \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Automatic parsing" --> \endhtmlonly
 * 
 *  If you need to declare a new datatype, this is the simplest way to describe it to GRAS. Simply
 *  enclose its type definition  into a \ref GRAS_DEFINE_TYPE macro call, and you're set. Here is 
 *  an type declaration  example: \verbatim GRAS_DEFINE_TYPE(mytype,struct mytype {
   int myfirstfield;
   char mysecondfield;
 });\endverbatim
 *  The type is then both copied verbatim into your source file and stored for further parsing. This allows
 *  you to let GRAS parse the exact version you are actually using in your program.
 *  You can then retrieve the corresponding type description with \ref gras_datadesc_by_symbol.
 *  Don't worry too much for the performances, the type is only parsed once and a binary representation 
 *  is stored and used in any subsequent calls.
 * 
 *  If your structure contains any pointer, you have to explain GRAS the size of the pointed array. This
 *  can be 1 in the case of simple references, or more in the case of regular arrays. For that, use the 
 *  \ref GRAS_ANNOTE macro within the type declaration you are passing to \ref GRAS_DEFINE_TYPE. This macro
 *  rewrites itself to nothing in the declaration (so they won't pollute the type definition copied verbatim
 *  into your code), and give some information to GRAS about your pointer. 
 
 *  GRAS_ANNOTE takes two arguments being the key name and the key value. For now, the only accepted key name 
 *  is "size", to specify the length of the pointed array. It can either be:
 *    - the string "1" (without the quote),
 *    - the name of another field of the structure
 *    - a sort of computed expression for multidimensional arrays (see below -- pay attention to the warnings below).
 *  
 *  Here is an example:\verbatim GRAS_DEFINE_TYPE(s_clause,
  struct s_array {
    struct s_array *father GRAS_ANNOTE(size,1);
    int length;
    int *data GRAS_ANNOTE(size,length);
    int rows;
    int cols;
    int *matrix GRAS_ANNOTE(size,rows*cols);
 }
;)\endverbatim
 * It specifies that the structure s_array contains five fields, that the \a father field is a simple reference,
 * that the size of the array pointed by \a data is the \a length field, and that the \a matrix field is an array
 * which size is the result of \a rows times \a cols.
 * 
 *  \warning The mecanism for multidimensional arrays is known to be fragile and cumbersome. If you want to use it, 
 *  you have to understand how it is implemented: the multiplication is performed using the sizes stack. In previous example,
 *  a \ref gras_datadesc_cb_push_int callback is added to the \a rows field and a \ref gras_datadesc_cb_push_int_mult one is 
 *  added to \a cols. So, when the structure is sent, the rows field push its value onto the stack, then the \a cols field 
 *  retrieve this value from the stack, compute (and push) the multiplication value. The \a matrix field can then retrive this
 *  value by poping the array. There is several ways for this to go wrong:
 *   - if the matrix field is placed before the sizes, the right value won't get pushed into the stack soon enough. Reorder your structure fields if needed.
 *   - if you write GRAS_ANNOTE(size,cols*rows); in previous example (inverting rows and cols in annotation),
 *     \a rows will be given a \ref gras_datadesc_cb_push_int_mult. This cannot work since it will try to 
 *     pop the value which will be pushed by \a cols <i>afterward</i>.
 *   - if you have more than one matrix in your structure, don't interleave the size. They are pushed/poped in the structure order.
 *   - if some of the sizes are used in more than one matrix, you cannot use this mecanism -- sorry.
 *
 * If you cannot express your datadescs with this mechanism, you'll have to use the more advanced 
 * (and somehow complex) one described below.
 *
 *  \warning Since GRAS_DEFINE_TYPE is a macro, you shouldn't put any comma in your type definition 
 *  (comma separates macro args). For example, change \verbatim int a, b;\endverbatim to \verbatim int a;
 int b;\endverbatim
 */
/** @{ */

 
/**   @brief Automatically parse C code
 *    @hideinitializer
 */
#define GRAS_DEFINE_TYPE(name,def) \
  static const char * _gras_this_type_symbol_does_not_exist__##name=#def; def
 
/** @brief Retrieve a datadesc which was previously parsed 
 *  @hideinitializer
 */
#define gras_datadesc_by_symbol(name)  \
  (gras_datadesc_by_name(#name) ?      \
   gras_datadesc_by_name(#name) :      \
     gras_datadesc_parse(#name,        \
			 _gras_this_type_symbol_does_not_exist__##name) \
  )

/** @def GRAS_ANNOTE
 *  @brief Add an annotation to a type to be automatically parsed
 */
#define GRAS_ANNOTE(key,val)

/* @} */

gras_datadesc_type_t 
gras_datadesc_parse(const char *name, const char *C_statement);

/** @defgroup GRAS_dd_manual Simple manual data description
 *  @ingroup GRAS_dd
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref GRAS_API]::[\ref GRAS_dd]
 *                <tr><td><b>Prev</b>   <td> [\ref GRAS_dd_auto]
 *                <tr><td><b>Next</b>   <td> [\ref GRAS_dd_cb_simple]            </table></center>
 * 
 * Here are the functions to use if you want to declare your description manually. 
 * The function names should be self-explanatory in most cases.
 * 
 * You can add callbacks to the datatypes doing any kind of action you may want. Usually, 
 * pre-send callbacks are used to prepare the type expedition while post-receive callbacks 
 * are used to fix any issue after the receive.
 * 
 * If your types are dynamic, you'll need to add some extra callback. For example, there is a
 * specific callback for the string type which is in charge of computing the length of the char
 * array. This is done with the cbps mechanism, explained in next section.
 * 
 * If your types may contain pointer cycle, you must specify it to GRAS using the @ref gras_datadesc_cycle_set. 
 * 
 * Example:\verbatim
 typedef struct {
   unsigned char c1;
   unsigned long int l1;
   unsigned char c2;
   unsigned long int l2;
 } mystruct;
 [...]
  my_type=gras_datadesc_struct("mystruct");
  gras_datadesc_struct_append(my_type,"c1", gras_datadesc_by_name("unsigned char"));
  gras_datadesc_struct_append(my_type,"l1", gras_datadesc_by_name("unsigned long"));
  gras_datadesc_struct_append(my_type,"c2", gras_datadesc_by_name("unsigned char"));
  gras_datadesc_struct_append(my_type,"l2", gras_datadesc_by_name("unsigned long int"));
  gras_datadesc_struct_close(my_type);

  my_type=gras_datadesc_ref("mystruct*", gras_datadesc_by_name("mystruct"));
  
  [Use my_type to send pointers to mystruct data]\endverbatim
 */
/* @{ */


/** \brief Opaque type describing a type description callback persistant state. */
typedef struct s_gras_cbps *gras_cbps_t;

/* callbacks prototypes */
/** \brief Prototype of type callbacks returning nothing. */
typedef void (*gras_datadesc_type_cb_void_t)(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);
/** \brief Prototype of type callbacks returning an int. */
typedef int (*gras_datadesc_type_cb_int_t)(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);
/** \brief Prototype of type callbacks selecting a type. */
typedef gras_datadesc_type_t (*gras_datadesc_selector_t)(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);


/******************************************
 **** Declare datadescription yourself ****
 ******************************************/

gras_datadesc_type_t gras_datadesc_struct(const char *name);
void gras_datadesc_struct_append(gras_datadesc_type_t  struct_type,
				 const char           *name,
				 gras_datadesc_type_t  field_type);
void gras_datadesc_struct_close(gras_datadesc_type_t   struct_type);


gras_datadesc_type_t gras_datadesc_union(const char                 *name,
					 gras_datadesc_type_cb_int_t selector);
void gras_datadesc_union_append(gras_datadesc_type_t   union_type,
				const char            *name,
				gras_datadesc_type_t   field_type);
void gras_datadesc_union_close(gras_datadesc_type_t    union_type);


gras_datadesc_type_t 
  gras_datadesc_ref(const char          *name,
		    gras_datadesc_type_t referenced_type);
gras_datadesc_type_t 
  gras_datadesc_ref_generic(const char              *name,
			    gras_datadesc_selector_t selector);

gras_datadesc_type_t 
  gras_datadesc_array_fixed(const char          *name,
			    gras_datadesc_type_t element_type,
			    long int             fixed_size);
gras_datadesc_type_t 
  gras_datadesc_array_dyn(const char                 *name,
			  gras_datadesc_type_t        element_type,
			  gras_datadesc_type_cb_int_t dynamic_size);
gras_datadesc_type_t 
  gras_datadesc_ref_pop_arr(gras_datadesc_type_t  element_type);

gras_datadesc_type_t 
  gras_datadesc_dynar(gras_datadesc_type_t elm_t,
		      void_f_pvoid_t *free_func);

/*********************************
 * Change stuff within datadescs *
 *********************************/

/** \brief Specify that this type may contain cycles */
void gras_datadesc_cycle_set(gras_datadesc_type_t type);
/** \brief Specify that this type do not contain any cycles (default) */
void gras_datadesc_cycle_unset(gras_datadesc_type_t type);
/** \brief Add a pre-send callback to this datadesc. */
void gras_datadesc_cb_send (gras_datadesc_type_t         type,
			    gras_datadesc_type_cb_void_t pre);
/** \brief Add a post-receive callback to this datadesc.*/
void gras_datadesc_cb_recv(gras_datadesc_type_t          type,
			   gras_datadesc_type_cb_void_t  post);
/** \brief Add a pre-send callback to the given field of the datadesc */
void gras_datadesc_cb_field_send (gras_datadesc_type_t   type,
				  const char            *field_name,
				  gras_datadesc_type_cb_void_t  pre);
/** \brief Add a post-receive callback to the given field of the datadesc */
void gras_datadesc_cb_field_recv(gras_datadesc_type_t    type,
				 const char             *field_name,
				 gras_datadesc_type_cb_void_t  post);
/** \brief Add a pre-send callback to the given field resulting in its value to be pushed */
void gras_datadesc_cb_field_push (gras_datadesc_type_t   type,
				  const char            *field_name);
/** \brief Add a pre-send callback to the given field resulting in its value multiplied to any previously pushed value and then pushed back */
void gras_datadesc_cb_field_push_multiplier (gras_datadesc_type_t type,
					     const char          *field_name);

/******************************
 * Get stuff within datadescs *
 ******************************/
/** \brief Returns the name of a datadescription */
const char * gras_datadesc_get_name(gras_datadesc_type_t ddt);
/** \brief Returns the identifier of a datadescription */
int gras_datadesc_get_id(gras_datadesc_type_t ddt);

/* @} */

/** @defgroup GRAS_dd_cb_simple Data description with Callback Persistant State: Simple push/pop mechanism
 *  @ingroup GRAS_dd
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref GRAS_API]::[\ref GRAS_dd]
 *                <tr><td><b>Prev</b>   <td> [\ref GRAS_dd_manual]
 *                <tr><td><b>Next</b>   <td> [\ref GRAS_dd_cb_full]            </table></center>
 * 
 * Sometimes, one of the callbacks need to leave information for the next ones. If this is a simple integer (such as
 * an array size), you can use the functions described here. If not, you'll have to play with the complete cbps interface.
 *
 * 
 * Here is an example:\verbatim
struct s_array {
  int length;
  int *data;
}
[...]
my_type=gras_datadesc_struct("s_array");
gras_datadesc_struct_append(my_type,"length", gras_datadesc_by_name("int"));
gras_datadesc_cb_field_send (my_type, "length", gras_datadesc_cb_push_int);

gras_datadesc_struct_append(my_type,"data",
                            gras_datadesc_array_dyn ("s_array::data",gras_datadesc_by_name("int"), gras_datadesc_cb_pop));
gras_datadesc_struct_close(my_type);
\endverbatim

 *
 * The *_mult versions are intended for multi-dimensional arrays: They multiply their value to the previously pushed one 
 * (by another field callback) and push the result of the multiplication back. An example of use follows. Please note
 * that the first field needs a regular push callback, not a multiplier one. Think of it as a stacked calculator (man dc(1)).\verbatim
struct s_matrix {
  int row;
  int col;
  int *data;
}
[...]
my_type=gras_datadesc_struct("s_matrix");
gras_datadesc_struct_append(my_type,"row", gras_datadesc_by_name("int"));
gras_datadesc_cb_field_send (my_type, "length", gras_datadesc_cb_push_int);
gras_datadesc_struct_append(my_type,"col", gras_datadesc_by_name("int"));
gras_datadesc_cb_field_send (my_type, "length", gras_datadesc_cb_push_int_mult);

gras_datadesc_struct_append(my_type,"data",
                            gras_datadesc_array_dyn ("s_matrix::data",gras_datadesc_by_name("int"), gras_datadesc_cb_pop));
gras_datadesc_struct_close(my_type);
\endverbatim
 
 */
/* @{ */

void
gras_cbps_i_push(gras_cbps_t ps, int val);
int 
gras_cbps_i_pop(gras_cbps_t ps);

int gras_datadesc_cb_pop(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);

void gras_datadesc_cb_push_int(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_uint(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_lint(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_ulint(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);

void gras_datadesc_cb_push_int_mult(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_uint_mult(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_lint_mult(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_ulint_mult(gras_datadesc_type_t typedesc, gras_cbps_t vars, void *data);


/* @} */

/** @defgroup GRAS_dd_cb_full Data description with Callback Persistant State: Full featured interface
 *  @ingroup GRAS_dd
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref GRAS_API]::[\ref GRAS_dd]
 *                <tr><td><b>Prev</b>   <td> [\ref GRAS_dd_cb_simple]
 *                <tr><td>Next          <td>             </table></center>
 * 
 * Sometimes, one of the callbacks need to leave information for the next ones. If the simple push/pop mechanism
 * introduced in previous section isn't enough, you can always use this full featured one.
 */

/* @{ */

void   gras_cbps_v_pop (gras_cbps_t            ps, 
			const char            *name,
	      /* OUT */ gras_datadesc_type_t  *ddt,
	      /* OUT */ void                 **res);
void   gras_cbps_v_push(gras_cbps_t            ps,
			const char            *name,
			void                  *data,
			gras_datadesc_type_t   ddt);
void   gras_cbps_v_set (gras_cbps_t            ps,
			const char            *name,
			void                  *data,
			gras_datadesc_type_t   ddt);

void * gras_cbps_v_get (gras_cbps_t            ps, 
			const char            *name,
	      /* OUT */ gras_datadesc_type_t  *ddt);

void gras_cbps_block_begin(gras_cbps_t ps);
void gras_cbps_block_end(gras_cbps_t ps);

/* @} */
/* @} */


/*******************************
 **** About data convertion ****
 *******************************/
int gras_arch_selfid(void); /* ID of this arch */


/*****************************
 **** NWS datadescription * FIXME: obsolete?
 *****************************/

/**
 * Basic types we can embeed in DataDescriptors.
 */
typedef enum
  {CHAR_TYPE, DOUBLE_TYPE, FLOAT_TYPE, INT_TYPE, LONG_TYPE, SHORT_TYPE,
   UNSIGNED_INT_TYPE, UNSIGNED_LONG_TYPE, UNSIGNED_SHORT_TYPE, STRUCT_TYPE}
  DataTypes;
#define SIMPLE_TYPE_COUNT 9

/**  \brief Describe a collection of data.
 * 
** A description of a collection of \a type data.  \a repetitions is used only
** for arrays; it contains the number of elements.  \a offset is used only for
** struct members in host format; it contains the offset of the member from the
** beginning of the struct, taking into account internal padding added by the
** compiler for alignment purposes.  \a members, \a length, and \a tailPadding are
** used only for STRUCT_TYPE data; the \a length -long array \a members describes
** the members of the nested struct, and \a tailPadding indicates how many
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

gras_datadesc_type_t
gras_datadesc_import_nws(const char           *name,
			 const DataDescriptor *desc,
			 unsigned long         howmany);


SG_END_DECL()

#endif /* GRAS_DATADESC_H */
