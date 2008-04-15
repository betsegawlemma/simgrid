/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */

package simgrid.msg;

import java.lang.Thread;
import java.util.*;

/**
 * A process may be defined as a code, with some private data, executing 
 * in a location (host). All the process used by your simulation must be
 * declared in the deployment file (XML format).
 * To create your ouwn process you must inherite your own process from this 
 * class and override the method "main()". For example if you want to use 
 * a process named Slave proceed as it :
 *
 * (1) import the class Process of the package simgrid.msg
 * import simgrid.msg.Process;
 * 
 * public class Slave extends simgrid.msg.Process {
 *
 *  (2) Override the method function
 * 
 *  \verbatim
 * 	public void main(String[] args) {
 *		System.out.println("Hello MSG");
 *	}
 *  \endverbatim
 * }
 * The name of your process must be declared in the deployment file of your simulation.
 * For the exemple, for the previouse process Slave this file must contains a line :
 * <process host="Maxims" function="Slave"/>, where Maxims is the host of the process
 * Slave. All the process of your simulation are automaticaly launched and managed by Msg.
 * A process use tasks to simulate communications or computations with another process. 
 * For more information see Task. For more information on host concept 
 * see Host.
 * 
 * @author  Abdelmalek Cherier
 * @author  Martin Quinson
 * @since SimGrid 3.3
 */

public abstract class Process extends Thread {
    /**
     * This attribute represents a bind between a java process object and
     * a native process. Even if this attribute is public you must never
     * access to it. It is set automaticatly during the build of the object.
     */
  public long bind;

    /**
     * Even if this attribute is public you must never access to it.
     * It is used to compute the id of an MSG process.
     */
  public static long nextProcessId = 0;

    /**
     * Even if this attribute is public you must never access to it.
     * It is compute automaticaly during the creation of the object. 
     * The native functions use this identifier to synchronize the process.
     */
  public long id;
  
  public Hashtable properties;

    /**
     * The name of the process.							
     */
  protected String name;
  public String msgName() {
    return this.name;
  }
  /*
   * The arguments of the method function of the process.     
   */ public Vector args;

  /* process synchronisation tools */
  protected Sem schedBegin, schedEnd;

    /**
     * Default constructor. (used in ApplicationHandler to initialize it)
     */
  protected Process() {
    super();
    this.id = 0;
    this.name = null;
    this.bind = 0;
    this.args = new Vector();
    this.properties = null;
    schedBegin = new Sem(0);
    schedEnd = new Sem(0);
  }


    /**
     * Constructs a new process from the name of a host and his name. The method
     * function of the process doesn't have argument.
     *
     * @param hostName		The name of the host of the process to create.
     * @param name			The name of the process.
     *
     * @exception			HostNotFoundException  if no host with this name exists.
     *                              NullPointerException if the provided name is null
     *                              JniException on JNI madness
     *
     */
  public Process(String hostname, String name)
  throws NullPointerException, HostNotFoundException, JniException,
    NativeException {
    this(Host.getByName(hostname), name, null);
  }
    /**
     * Constructs a new process from the name of a host and his name. The arguments
     * of the method function of the process are specified by the parameter args.
     *
     * @param hostName		The name of the host of the process to create.
     * @param name			The name of the process.
     * @param args			The arguments of the main function of the process.
     *
     * @exception			HostNotFoundException  if no host with this name exists.
     *                              NullPointerException if the provided name is null
     *                              JniException on JNI madness
     *
     */ public Process(String hostname, String name, String args[])
  throws NullPointerException, HostNotFoundException, JniException,
    NativeException {
    this(Host.getByName(hostname), name, args);
  }
    /**
     * Constructs a new process from a host and his name. The method function of the 
     * process doesn't have argument.
     *
     * @param host			The host of the process to create.
     * @param name			The name of the process.
     *
     * @exception			NullPointerException if the provided name is null
     *                              JniException on JNI madness
     *
     */
    public Process(Host host, String name) throws NullPointerException,
    JniException {
    this(host, name, null);
  }
    /**
     * Constructs a new process from a host and his name, the arguments of here method function are
     * specified by the parameter args.
     *
     * @param host			The host of the process to create.
     * @param name			The name of the process.
     * @param args			The arguments of main method of the process.
     *
     * @exception		    NullPointerException if the provided name is null
     *                              JniException on JNI madness
     *
     */
    public Process(Host host, String name,
                     String[]args) throws NullPointerException, JniException {
    /* This is the constructor called by all others */

    if (name == null)
      throw new NullPointerException("Process name cannot be NULL");

	  this.properties = null;
	  
      this.args = new Vector();

    if (null != args)
        this.args.addAll(Arrays.asList(args));

      this.name = name;
      this.id = nextProcessId++;

      schedBegin = new Sem(0);
      schedEnd = new Sem(0);

      MsgNative.processCreate(this, host);
  }
  
 
    /**
     * This method kills all running process of the simulation.
     *
     * @param resetPID		Should we reset the PID numbers. A negative number means no reset
     *						and a positive number will be used to set the PID of the next newly
     *						created process.
     *
     * @return				The function returns the PID of the next created process.
     *			
     */ public static int killAll(int resetPID) {
    return MsgNative.processKillAll(resetPID);
  }

    /**
     * This method adds an argument in the list of the arguments of the main function
     * of the process. 
     *
     * @param arg			The argument to add.
     */
  protected void addArg(String arg) {
    args.add(arg);
  }

    /**
     * This method suspends the process by suspending the task on which it was
     * waiting for the completion.
     *
     * @exception			JniException on JNI madness
     *                              NativeException on error in the native SimGrid code
     */
  public void pause() throws JniException, NativeException {
    MsgNative.processSuspend(this);
  }
    /**
     * This method resumes a suspended process by resuming the task on which it was
     * waiting for the completion.
     *
     * @exception			JniException on JNI madness
     *                              NativeException on error in the native SimGrid code
     *
     */ public void restart() throws JniException, NativeException {
    MsgNative.processResume(this);
  }
    /**
     * This method tests if a process is suspended.
     *
     * @return				The method returns true if the process is suspended.
     *						Otherwise the method returns false.
     *
     * @exception			JniException on JNI madness
     */ public boolean isSuspended() throws JniException {
    return MsgNative.processIsSuspended(this);
  }
    /**
     * This method returns the host of a process.
     *
     * @return				The host instance of the process.
     *
     * @exception			JniException on JNI madness
     *                              NativeException on error in the native SimGrid code
     *
     */ public Host getHost() throws JniException, NativeException {
    return MsgNative.processGetHost(this);
  }
    /**
     * This static method get a process from a PID.
     *
     * @param PID			The process identifier of the process to get.
     *
     * @return				The process with the specified PID.
     *
     * @exception			NativeException on error in the native SimGrid code
     */ public static Process fromPID(int PID) throws NativeException {
    return MsgNative.processFromPID(PID);
  }
    /**
     * This method returns the PID of the process.
     *
     * @return				The PID of the process.
     *
     * @exception			JniException on JNI madness
     *                              NativeException on error in the native SimGrid code
     */ public int getPID() throws JniException, NativeException {
    return MsgNative.processGetPID(this);
  }
    /**
     * This method returns the PID of the parent of a process.
     *
     * @return				The PID of the parent of the process.
     *
     * @exception			NativeException on error in the native SimGrid code
     */ public int getPPID() throws NativeException {
    return MsgNative.processGetPPID(this);
  }
    /**
     * This static method returns the currently running process.
     *
     * @return				The current process.
     *
     * @exception			NativeException on error in the native SimGrid code
     *
     *
     */ public static Process currentProcess() throws NativeException {
    return MsgNative.processSelf();
  }
    /**
     * This static method returns the PID of the currently running process.
     *
     * @return				The PID of the current process.		
     */ public static int currentProcessPID() {
    return MsgNative.processSelfPID();
  }

    /**
     * This static method returns the PID of the parent of the currently running process.
     *
     * @return				The PID of the parent of current process.		
     */
  public static int currentProcessPPID() {
    return MsgNative.processSelfPPID();
  }

    /**
     * This function migrates a process to another host.
     *
     * @parm host			The host where to migrate the process.
     *
     * @exception			JniException on JNI madness
     *                              NativeException on error in the native SimGrid code
     */
  public void migrate(Host host) throws JniException, NativeException {
    MsgNative.processChangeHost(this, host);
  }
    /**
     * This method makes the current process sleep until time seconds have elapsed.
     *
     * @param seconds		The time the current process must sleep.
     *
     * @exception			NativeException on error in the native SimGrid code
     */ public static void waitFor(double seconds) throws NativeException {
    MsgNative.processWaitFor(seconds);
  } public void showArgs() {
    try {
      Msg.info("[" + this.name + "/" + this.getHost().getName() + "] argc=" +
               this.args.size());
      for (int i = 0; i < this.args.size(); i++)
        Msg.info("[" + this.msgName() + "/" + this.getHost().getName() +
                 "] args[" + i + "]=" + (String) (this.args.get(i)));
    } catch(MsgException e) {
      Msg.info("Damn JNI stuff");
      e.printStackTrace();
      System.exit(1);
    }
  }
    /**
     * This method runs the process. Il calls the method function that you must overwrite.
     */
  public synchronized void run() {

      String[]args = null;      /* do not fill it before the signal or this.args will be empty */

      //waitSignal(); /* wait for other people to fill the process in */


     try {
        schedBegin.acquire();
     } catch(InterruptedException e) {
     }



    try {
      args = new String[this.args.size()];
      if (this.args.size() > 0) {
        this.args.toArray(args);
      }

      this.main(args);
      MsgNative.processExit(this);
      schedEnd.release();
    }
    catch(MsgException e) {
      e.printStackTrace();
      Msg.info("Unexpected behavior. Stopping now");
      System.exit(1);
    }
  }

    /**
     * The main function of the process (to implement).
     */
  public abstract void main(String[]args)
  throws JniException, NativeException;


  public void unschedule() {
    try {
      schedEnd.release();
      schedBegin.acquire();
    } catch(InterruptedException e) {
    }
  }

  public void schedule() {
    try {
      schedBegin.release();
      schedEnd.acquire();
    } catch(InterruptedException e) {
    }
  }
  
  /** Send the given task to given host on given channel */
  public void taskPut(Host host, int channel,
                       Task task) throws NativeException, JniException {
    MsgNative.hostPut(host, channel, task, -1);
  }
  
   /** Send the given task to given host on given channel (waiting at most given time)*/
   public void taskPut(Host host, int channel,
                       Task task, double timeout) throws NativeException, JniException {
    MsgNative.hostPut(host, channel, task, timeout);
  }
   /** Receive a task on given channel */
    public Task taskGet(int channel) throws NativeException,
    JniException {
    return MsgNative.taskGet(channel, -1, null);
  }
   /** Receive a task on given channel (waiting at most given time) */
    public Task taskGet(int channel,
                            double timeout) throws NativeException,
    JniException {
    return MsgNative.taskGet(channel, timeout, null);
  }
   /** Receive a task on given channel from given sender */
    public Task taskGet(int channel, Host host) throws NativeException,
    JniException {
    return MsgNative.taskGet(channel, -1, host);
  }
   /** Receive a task on given channel from given sender (waiting at most given time) */
    public Task taskGet(int channel, double timeout,
                            Host host) throws NativeException, JniException {
    return MsgNative.taskGet(channel, timeout, host);
  }
  
  /** Send the given task in the mailbox associated with the specified alias  (waiting at most given time) */
  public void taskSend(String alias,
                       Task task, double timeout) throws NativeException, JniException {
    MsgNative.taskSend(alias, task, timeout);
  }
  
  /** Send the given task in the mailbox associated with the specified alias*/
  public void taskSend(String alias,
                       Task task) throws NativeException, JniException {
    MsgNative.taskSend(alias, task, -1);
  }
  
  /** Send the given task in the mailbox associated with the default alias  (defaultAlias = "hostName:processName") */
  public void taskSend(Task task) throws NativeException, JniException {
  	
  	String alias = Host.currentHost().getName() + ":" + this.msgName();
    MsgNative.taskSend(alias, task, -1);
  }
  
  /** Send the given task in the mailbox associated with the default alias (waiting at most given time) */
  public void taskSend(Task task, double timeout) throws NativeException, JniException {
  	
  	String alias = Host.currentHost().getName() + ":" + this.msgName();
    MsgNative.taskSend(alias, task, timeout);
  }
  
  
   /** Receive a task on mailbox associated with the specified alias */
    public Task taskReceive(String alias) throws NativeException,
    JniException {
    return MsgNative.taskReceive(alias, -1.0, null);
  }
  
  /** Receive a task on mailbox associated with the default alias */
   public Task taskReceive() throws NativeException,
    JniException {
    String alias = Host.currentHost().getName() + ":" + this.msgName();
    return MsgNative.taskReceive(alias, -1.0, null);
  }
  
  /** Receive a task on mailbox associated with the specified alias (waiting at most given time) */
    public Task taskReceive(String alias,
                            double timeout) throws NativeException,
    JniException {
    return MsgNative.taskReceive(alias, timeout, null);
  }
  
  /** Receive a task on mailbox associated with the default alias (waiting at most given time) */
   public Task taskReceive(double timeout) throws NativeException,
    JniException {
    String alias = Host.currentHost().getName() + ":" + this.msgName();
    return MsgNative.taskReceive(alias, timeout, null);
  }
  
   /** Receive a task on mailbox associated with the specified alias from given sender */
    public Task taskReceive(String alias,
                            double timeout, Host host) throws NativeException,
    JniException {
    return MsgNative.taskReceive(alias, timeout, host);
  }
  
  /** Receive a task on mailbox associated with the default alias from given sender  (waiting at most given time) */
  public Task taskReceive(double timeout, Host host) throws NativeException,
    JniException {
    String alias = Host.currentHost().getName() + ":" + this.msgName();
    return MsgNative.taskReceive(alias, timeout, host);
  }
  
   /** Receive a task on mailbox associated with the specified alias from given sender*/
     public Task taskReceive(String alias,
                            Host host) throws NativeException,
    JniException {
    return MsgNative.taskReceive(alias, -1.0, host);
  }
   /** Receive a task on mailbox associated with the default alias from given sender */
   public Task taskReceive( Host host) throws NativeException,
    JniException {
    	String alias = Host.currentHost().getName() + ":" + this.msgName();
    return MsgNative.taskReceive(alias, -1.0, host);
  }
}
