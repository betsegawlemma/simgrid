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

public class SuspendTest {
    
    public static void main(String[] args) throws JniException, NativeException {
    	
	/* initialize the MSG simulation. Must be done before anything else (even logging). */
	Msg.init(args);

    	if(args.length < 2) {
    		
    		Msg.info("Usage   : Suspend platform_file deployment_file");
        	Msg.info("example : Suspend msg_test_suspend_platform.xml msg_test_suspend_deployment.xml");
        	System.exit(1);
    	}
    	
	/* specify the number of channel for the process of the simulation. */
	Channel.setNumber(1);
        /* specify a paje output file. */
        Msg.pajeOutput("suspend.trace");
		
	/* construct the platform and deploy the application */
	Msg.createEnvironment(args[0]);
	Msg.deployApplication(args[1]);
		
	/*  execute the simulation. */
        Msg.run();
    }
}
