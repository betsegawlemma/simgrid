/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier         
 * All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class BasicTest {
   
   /* This only contains the launcher. If you do nothing more than than you can run 
    *   java simgrid.msg.Msg
    * which also contains such a launcher
    */
   
    public static void main(String[] args) throws JniException, NativeException {
       
       /* initialize the MSG simulation. Must be done before anything else (even logging). */
       Msg.init(args);

       if(args.length < 2) {
    		
	  Msg.info("Usage   : Basic platform_file deployment_file");
	  Msg.info("example : Basic basic_platform.xml basic_deployment.xml");
	  System.exit(1);
       }
		
        /* specify a paje output file. */
        Msg.pajeOutput("basic.trace");
		
	/* construct the platform and deploy the application */
	Msg.createEnvironment(args[0]);
	Msg.deployApplication(args[1]);
		
	/*  execute the simulation. */
        Msg.run();
    }
}
