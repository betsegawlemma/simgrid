/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_MAXMIN_H
#define _SURF_MAXMIN_H

#include "xbt/misc.h"

#define MAXMIN_PRECISION 0.00001
static XBT_INLINE void double_update(double *variable, double value) 
{
  *variable -= value;
  if(*variable< MAXMIN_PRECISION) *variable = 0.0;
}

static XBT_INLINE int double_positive(double value) 
{
  return (value>MAXMIN_PRECISION);
}

typedef struct lmm_element *lmm_element_t;
typedef struct lmm_variable *lmm_variable_t;
typedef struct lmm_constraint *lmm_constraint_t;
typedef struct lmm_system *lmm_system_t;

XBT_PUBLIC(lmm_system_t) lmm_system_new(void);
XBT_PUBLIC(void) lmm_system_free(lmm_system_t sys);
void lmm_variable_disable(lmm_system_t sys, lmm_variable_t var);

XBT_PUBLIC(lmm_constraint_t) lmm_constraint_new(lmm_system_t sys, void *id,
				    double bound_value);
void lmm_constraint_shared(lmm_constraint_t cnst);

void lmm_constraint_free(lmm_system_t sys, lmm_constraint_t cnst);

XBT_PUBLIC(lmm_variable_t) lmm_variable_new(lmm_system_t sys, void *id,
				double weight_value,
				double bound, int number_of_constraints);
void lmm_variable_free(lmm_system_t sys, lmm_variable_t var);
XBT_PUBLIC(double) lmm_variable_getvalue(lmm_variable_t var);
XBT_PUBLIC(double) lmm_variable_getbound(lmm_variable_t var);

XBT_PUBLIC(void) lmm_expand(lmm_system_t sys, lmm_constraint_t cnst,
		lmm_variable_t var, double value);
void lmm_expand_add(lmm_system_t sys, lmm_constraint_t cnst,
		    lmm_variable_t var, double value);
void lmm_elem_set_value(lmm_system_t sys, lmm_constraint_t cnst,
			lmm_variable_t var, double value);

lmm_constraint_t lmm_get_cnst_from_var(lmm_system_t sys,
				       lmm_variable_t var, int num);
int lmm_get_number_of_cnst_from_var(lmm_system_t sys, lmm_variable_t var);
lmm_variable_t lmm_get_var_from_cnst(lmm_system_t sys,
				     lmm_constraint_t cnst,
				     lmm_element_t * elem);

lmm_constraint_t lmm_get_first_active_constraint(lmm_system_t sys);
lmm_constraint_t lmm_get_next_active_constraint(lmm_system_t sys, lmm_constraint_t cnst);

void *lmm_constraint_id(lmm_constraint_t cnst);
void *lmm_variable_id(lmm_variable_t var);

void lmm_update(lmm_system_t sys, lmm_constraint_t cnst,
		lmm_variable_t var, double value);
void lmm_update_variable_bound(lmm_system_t sys, lmm_variable_t var,
			       double bound);


XBT_PUBLIC(void) lmm_update_variable_latency(lmm_system_t sys, lmm_variable_t var,
				 double delta);



XBT_PUBLIC(void) lmm_update_variable_weight(lmm_system_t sys, lmm_variable_t var,
				double weight);
double lmm_get_variable_weight(lmm_variable_t var);

XBT_PUBLIC(void) lmm_update_constraint_bound(lmm_system_t sys, lmm_constraint_t cnst,
				 double bound);

int lmm_constraint_used(lmm_system_t sys, lmm_constraint_t cnst);


XBT_PUBLIC(void) lmm_solve(lmm_system_t sys);

#ifdef HAVE_SDP
XBT_PUBLIC(void) sdp_solve(lmm_system_t sys);
#endif /* HAVE_SDP */

XBT_PUBLIC(void) lagrange_solve(lmm_system_t sys);
XBT_PUBLIC(void) bottleneck_solve(lmm_system_t sys);

/**
 * Default functions associated to the chosen protocol. When
 * using the lagrangian approach.
 */

XBT_PUBLIC(void) lmm_set_default_protocol_function(double (* func_f)  (lmm_variable_t var, double x),
				       double (* func_fp) (lmm_variable_t var, double x),
				       double (* func_fpi)(lmm_variable_t var, double x));

XBT_PUBLIC(double func_reno_f)  (lmm_variable_t var, double x);
XBT_PUBLIC(double func_reno_fp) (lmm_variable_t var, double x);
XBT_PUBLIC(double func_reno_fpi)(lmm_variable_t var, double x);

XBT_PUBLIC(double func_vegas_f)  (lmm_variable_t var, double x);
XBT_PUBLIC(double func_vegas_fp) (lmm_variable_t var, double x);
XBT_PUBLIC(double func_vegas_fpi)(lmm_variable_t var, double x);


#endif				/* _SURF_MAXMIN_H */
