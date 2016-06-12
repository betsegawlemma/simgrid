/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CHECKER_HPP
#define SIMGRID_MC_CHECKER_HPP

#include <functional>
#include <memory>
#include <string>

#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_record.h"
#include "src/mc/Session.hpp"

namespace simgrid {
namespace mc {

/** A model-checking algorithm
 *
 *  The goal is to move the data/state/configuration of a model-checking
 *  algorithms in subclasses. Implementing this interface will probably
 *  not be really mandatory, you might be able to write your
 *  model-checking algorithm as plain imperative code instead.
 *
 *  It works by manipulating a model-checking Session.
 */
// abstract
class Checker {
  Session* session_;
public:
  Checker(Session& session);

  // No copy:
  Checker(Checker const&) = delete;
  Checker& operator=(Checker const&) = delete;

  virtual ~Checker();
  virtual int run() = 0;

  // Give me your internal state:

  /** Show the current trace/stack
   *
   *  Could this be handled in the Session/ModelChecker instead?
   */
  virtual RecordTrace getRecordTrace();
  virtual std::vector<std::string> getTextualTrace();
  virtual void logState();

protected:
  Session& getSession() { return *session_; }
};

XBT_PUBLIC() Checker* createLivenessChecker(Session& session);
XBT_PUBLIC() Checker* createSafetyChecker(Session& session);
XBT_PUBLIC() Checker* createCommunicationDeterminismChecker(Session& session);

}
}

#endif
