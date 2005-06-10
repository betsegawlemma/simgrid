/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Warning, this module is done to be efficient and performs tons of
   cast and dirty things. So avoid using it unless you really know
   what you are doing. */

/* This type should be added to a type that is to be used in such a swag */

#include "xbt/sysdep.h"
#include "xbt/error.h"
#include "xbt/swag.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(swag,xbt,"Swag : O(1) set library");

#define PREV(obj,offset) xbt_swag_getPrev(obj,offset)
#define NEXT(obj,offset) xbt_swag_getNext(obj,offset)


/** Creates a new swag.
 * \param offset where the hookup is located in the structure
 * \see xbt_swag_offset
 *
 * Usage : xbt_swag_new(&obj.setA-&obj); 
 */
xbt_swag_t xbt_swag_new(size_t offset)
{
  xbt_swag_t swag = xbt_new0(s_xbt_swag_t, 1);

  swag->tail = NULL;
  swag->head = NULL;
  swag->offset = offset;
  swag->count = 0;

  return swag;
}

/** 
 * \param swag poor victim
 * 
 * kilkil a swag but not it's content. If you do not understand why
 * xbt_swag_free should not free its content, don't use swags.
 */
void xbt_swag_free(xbt_swag_t swag)
{
  free(swag);
}

/** Creates a new swag.
 * \param swag the swag to initialize
 * \param offset where the hookup is located in the structure
 * \see xbt_swag_offset
 *
 * Usage : xbt_swag_init(swag,&obj.setA-&obj); 
 */
void xbt_swag_init(xbt_swag_t swag, size_t offset)
{
  swag->tail = NULL;
  swag->head = NULL;
  swag->offset = offset;
  swag->count = 0;
}


/** 
 * \param obj the objet to insert in the swag
 * \param swag a swag
 *
 * insert \a obj in \a swag
 */
void xbt_swag_insert(void *obj, xbt_swag_t swag)
{

  if (xbt_swag_belongs(obj, swag))
    return;

  (swag->count)++;
  if (swag->head == NULL) {
    xbt_assert0(!(swag->tail), "Inconsistent swag.");
    swag->head = obj;
    swag->tail = obj;
    return;
  }

  PREV(obj, swag->offset) = swag->tail;
  NEXT(PREV(obj, swag->offset), swag->offset) = obj;

  swag->tail = obj;
}

/** 
 * \param obj the objet to insert in the swag
 * \param swag a swag
 *
 * insert (at the head... you probably had a very good reason to do
 * that, I hope you know what you're doing) \a obj in \a swag
 */
void xbt_swag_insert_at_head(void *obj, xbt_swag_t swag)
{

  if (xbt_swag_belongs(obj, swag))
    return;

  (swag->count)++;
  if (swag->head == NULL) {
    xbt_assert0(!(swag->tail), "Inconsistent swag.");
    swag->head = obj;
    swag->tail = obj;
    return;
  }

  NEXT(obj, swag->offset) = swag->head;
  PREV(NEXT(obj, swag->offset), swag->offset) = obj;

  swag->head = obj;
}

/** 
 * \param obj the objet to insert in the swag
 * \param swag a swag
 *
 * insert (at the tail... you probably had a very good reason to do
 * that, I hope you know what you're doing) \a obj in \a swag
 */
void xbt_swag_insert_at_tail(void *obj, xbt_swag_t swag)
{

  if (xbt_swag_belongs(obj, swag))
    return;

  (swag->count)++;
  if (swag->head == NULL) {
    xbt_assert0(!(swag->tail), "Inconsistent swag.");
    swag->head = obj;
    swag->tail = obj;
    return;
  }

  PREV(obj, swag->offset) = swag->tail;
  NEXT(PREV(obj, swag->offset), swag->offset) = obj;

  swag->tail = obj;
}

/** 
 * \param obj the objet to remove from the swag
 * \param swag a swag
 * \return \a obj if it was in the \a swag and NULL otherwise
 *
 * removes \a obj from \a swag
 */
void *xbt_swag_remove(void *obj, xbt_swag_t swag)
{
  size_t offset = swag->offset;

  if ((!obj) || (!swag))
    return NULL;
  if(!xbt_swag_belongs(obj, swag)) /* Trying to remove an object that
				      was not in this swag */
      return NULL;

  if (swag->head == swag->tail) {	/* special case */
    if (swag->head != obj)	/* Trying to remove an object that was not in this swag */
      return NULL;
    swag->head = NULL;
    swag->tail = NULL;
    NEXT(obj, offset) = PREV(obj, offset) = NULL;
  } else if (obj == swag->head) {	/* It's the head */
    swag->head = NEXT(obj, offset);
    PREV(swag->head, offset) = NULL;
    NEXT(obj, offset) = NULL;
  } else if (obj == swag->tail) {	/* It's the tail */
    swag->tail = PREV(obj, offset);
    NEXT(swag->tail, offset) = NULL;
    PREV(obj, offset) = NULL;
  } else {			/* It's in the middle */
    NEXT(PREV(obj, offset), offset) = NEXT(obj, offset);
    PREV(NEXT(obj, offset), offset) = PREV(obj, offset);
    PREV(obj, offset) = NEXT(obj, offset) = NULL;
  }
  (swag->count)--;
  return obj;
}

/** 
 * \param swag a swag
 * \return an object from the \a swag
 */
void *xbt_swag_extract(xbt_swag_t swag)
{
  size_t offset = swag->offset;
  void *obj = NULL;

  if ((!swag) || (!(swag->head)))
    return NULL;

  obj = swag->head;

  if (swag->head == swag->tail) {	/* special case */
    swag->head = swag->tail = NULL;
    PREV(obj, offset) = NEXT(obj, offset) = NULL;
  } else {
    swag->head = NEXT(obj, offset);
    PREV(swag->head, offset) = NULL;
    NEXT(obj, offset) = NULL;
  }
  (swag->count)--;

  return obj;
}
/** 
 * \param swag a swag
 * \return the number of objects in \a swag
 */
int xbt_swag_size(xbt_swag_t swag)
{
  return (swag->count);
}

/** 
 * \param obj an object
 * \param swag a swag
 * \return 1 if \a obj is in the \a swag and 0 otherwise
 */
int xbt_swag_belongs(void *obj, xbt_swag_t swag)
{
  return ((NEXT(obj, swag->offset)) || (PREV(obj, swag->offset))
	  || (swag->head == obj));
}
