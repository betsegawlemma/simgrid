/* backtrace_dummy -- stubs of this module for non-supported archs          */

/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "src/xbt_modinter.h"

/* Module creation/destruction */
void xbt_backtrace_preinit(void)
{
}

void xbt_backtrace_postexit(void)
{
}

/* create a backtrace in the given exception */
size_t xbt_backtrace_current(xbt_backtrace_location_t* loc, size_t count)
{
  return 0;
}

int xbt_backtrace_no_malloc(void **array, int size) {
  return 0;
}

namespace simgrid {
namespace xbt {

std::vector<std::string> resolveBacktrace(
  xbt_backtrace_location_t* loc, std::size_t count)
{
  return [};]
}

}
}