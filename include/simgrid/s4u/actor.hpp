/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTOR_HPP
#define SIMGRID_S4U_ACTOR_HPP

#include "simgrid/simix.h"

namespace simgrid {
namespace s4u {

class Host;
class Mailbox;

/** @brief Simulation Agent
 *
 * An actor may be defined as a code executing in a location (host).
 *
 * All actors should be started from the XML deployment file (using the @link{s4u::Engine::loadDeployment()}),
 * even if you can also start new actors directly.
 * Separating the deployment in the XML from the logic in the code is a good habit as it makes your simulation easier
 * to adapt to new settings.
 *
 * The code that you define for a given actor should be placed in the main method that is virtual.
 * For example, a Worker actor should be declared as follows:
 *
 * \verbatim
 * #include "s4u/actor.hpp"
 *
 * class Worker : simgrid::s4u::Actor {
 *
 * 	  int main(int argc, char **argv) {
 *		printf("Hello s4u");
 *	  }
 * };
 * \endverbatim
 *
 */
class Actor {
public:
	Actor(const char*name, s4u::Host *host, int argc, char **argv);
	Actor(const char*name, s4u::Host *host, int argc, char **argv, double killTime);
	virtual ~Actor() {}

	/** The main method of your agent */
	virtual int main(int argc, char **argv)=0;

	/** The Actor that is currently running */
	static Actor *current();
	/** Retrieves the actor that have the given PID (or NULL if not existing) */
	static Actor *byPid(int pid);

	/** Retrieves the name of that actor */
	const char*getName();
	/** Retrieves the host on which that actor is running */
	s4u::Host *getHost();
	/** Retrieves the PID of that actor */
	int getPid();

	/** If set to true, the actor will automatically restart when its host reboots */
	void setAutoRestart(bool autorestart);
	/** Sets the time at which that actor should be killed */
	void setKillTime(double time);
	/** Retrieves the time at which that actor will be killed (or -1 if not set) */
	double getKillTime();

	/** Ask kindly to all actors to die. Only the issuer will survive. */
	static void killAll();
	/** Ask the actor to die.
	 *
	 * It will only notice your request when doing a simcall next time (a communication or similar).
	 * SimGrid sometimes have issues when you kill actors that are currently communicating and such. We are working on it to fix the issues.
	 */
	void kill();

	/** Block the actor sleeping for that amount of seconds (may throws hostFailure) */
	void sleep(double duration);

	/** Block the actor, computing the given amount of flops */
	void execute(double flop);

	/** Block the actor until it gets a message from the given mailbox */
	//void* recv(const char *mailbox);

	/** Block the actor until it gets a string message (to be freed after use) from the given mailbox */
	char *recvstr(Mailbox &chan);

	/** Block the actor until it delivers a string message (that will be copied) to the given mailbox */
	void sendstr(Mailbox &chan, const char*msg);

protected:
	smx_process_t getInferior() {return p_smx_process;}
private:
	smx_process_t p_smx_process;
};
}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_ACTOR_HPP */

#if 0

public abstract class Actor implements Runnable {
	/** Suspends the process. See {@link #resume()} to resume it afterward */
	public native void suspend();
	/** Resume a process that was suspended by {@link #suspend()}. */
	public native void resume();	
	/** Tests if a process is suspended. */
	public native boolean isSuspended();
	
	/**
	 * Returns the value of a given process property. 
	 */
	public native String getProperty(String name);


	/**
	 * Migrates a process to another host.
	 *
	 * @param host			The host where to migrate the process.
	 *
	 */
	public native void migrate(Host host);	

	public native void exit();    
	/**
	 * This static method returns the current amount of processes running
	 *
	 * @return			The count of the running processes
	 */ 
	public native static int getCount();

}
#endif
