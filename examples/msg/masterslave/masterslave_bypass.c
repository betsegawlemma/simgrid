/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"  /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "surf/surfxml_parse.h" /* to override surf_parse and bypass the parser */
#include "surf/surfxml.h"       /* to hijack surf_parse_lex */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,"Messages specific for this msg example");
#define FINALIZE ((void*)221297) /* a magic number to tell people to stop working */

static int surf_parse_bypass_platform(void)
{
  static int AX_ptr = 0;
  static int surfxml_bufferstack_size = 2048;

  /* allocating memory for the buffer, I think 2kB should be enough */
  surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);
  
  /* <platform_description> */
  SURFXML_BUFFER_SET(platform_description_version,"2");

  SURFXML_START_TAG(platform_description);

/*   <host id="host A" power="100000000.00"/> */
  SURFXML_BUFFER_SET(host_id,"host A");
  SURFXML_BUFFER_SET(host_power, "100000000.00");
  SURFXML_BUFFER_SET(host_availability, "1.0");
  SURFXML_BUFFER_SET(host_availability_file, "");
  A_surfxml_host_state = A_surfxml_host_state_ON;
  SURFXML_BUFFER_SET(host_state_file, "");
  SURFXML_BUFFER_SET(host_interference_send, "1.0");
  SURFXML_BUFFER_SET(host_interference_recv, "1.0");
  SURFXML_BUFFER_SET(host_interference_send_recv, "1.0");
  SURFXML_BUFFER_SET(host_max_outgoing_rate, "-1.0");

  SURFXML_START_TAG(host);
  SURFXML_END_TAG(host);

/*   <host id="host B" power="100000000.00"/> */
  SURFXML_BUFFER_SET(host_id, "host B");
  SURFXML_BUFFER_SET(host_power, "100000000.00");
  SURFXML_BUFFER_SET(host_availability, "1.0");
  SURFXML_BUFFER_SET(host_availability_file, "");
  A_surfxml_host_state = A_surfxml_host_state_ON;
  SURFXML_BUFFER_SET(host_state_file, "");
  SURFXML_BUFFER_SET(host_interference_send, "1.0");
  SURFXML_BUFFER_SET(host_interference_recv, "1.0");
  SURFXML_BUFFER_SET(host_interference_send_recv, "1.0");
  SURFXML_BUFFER_SET(host_max_outgoing_rate, "-1.0");

  SURFXML_START_TAG(host);
  SURFXML_END_TAG(host);

/*   <link id="LinkA" bandwidth="10000000.0" latency="0.2"/> */
  SURFXML_BUFFER_SET(link_id, "LinkA");
  SURFXML_BUFFER_SET(link_bandwidth, "10000000.0");
  SURFXML_BUFFER_SET(link_bandwidth_file, "");
  SURFXML_BUFFER_SET(link_latency, "0.2");
  SURFXML_BUFFER_SET(link_latency_file, "");
  A_surfxml_link_state = A_surfxml_link_state_ON;
  SURFXML_BUFFER_SET(link_state_file, "");
  A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
  SURFXML_START_TAG(link);
  SURFXML_END_TAG(link);

/*   <route src="host A" dst="host B"><link:ctn id="LinkA"/></route> */
  SURFXML_BUFFER_SET(route_src, "host A");
  SURFXML_BUFFER_SET(route_dst, "host B");
  SURFXML_BUFFER_SET(route_impact_on_src, "0.0");
  SURFXML_BUFFER_SET(route_impact_on_dst, "0.0");
  SURFXML_BUFFER_SET(route_impact_on_src_with_other_recv, "0.0");
  SURFXML_BUFFER_SET(route_impact_on_dst_with_other_send, "0.0");

  SURFXML_START_TAG(route);

  SURFXML_BUFFER_SET(link_c_ctn_id, "LinkA");
  SURFXML_START_TAG(link_c_ctn);
  SURFXML_END_TAG(link_c_ctn);

  SURFXML_END_TAG(route);

/*   <route src="host B" dst="host A"><link:ctn id="LinkA"/></route> */
  SURFXML_BUFFER_SET(route_src, "host B");
  SURFXML_BUFFER_SET(route_dst, "host A");
  SURFXML_BUFFER_SET(route_impact_on_src, "0.0");
  SURFXML_BUFFER_SET(route_impact_on_dst, "0.0");
  SURFXML_BUFFER_SET(route_impact_on_src_with_other_recv, "0.0");
  SURFXML_BUFFER_SET(route_impact_on_dst_with_other_send, "0.0");

  SURFXML_START_TAG(route);

  SURFXML_BUFFER_SET(link_c_ctn_id, "LinkA");
  SURFXML_START_TAG(link_c_ctn);
  SURFXML_END_TAG(link_c_ctn);

  SURFXML_END_TAG(route);
/* </platform_description> */
  SURFXML_END_TAG(platform_description);

  free(surfxml_bufferstack);
  return 0;
}

static int surf_parse_bypass_application(void) {   
  static int AX_ptr;
  static int surfxml_bufferstack_size = 2048;

  /* allocating memory to the buffer, I think 2MB should be enough */
  surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);
  
  /* <platform_description> */
  SURFXML_BUFFER_SET(platform_description_version,"2");

  SURFXML_START_TAG(platform_description);
   
/*   <process host="host A" function="master"> */
  SURFXML_BUFFER_SET(process_host, "host A");
  SURFXML_BUFFER_SET(process_function, "master");
  SURFXML_BUFFER_SET(process_start_time, "-1.0");
  SURFXML_BUFFER_SET(process_kill_time, "-1.0");
  SURFXML_START_TAG(process);

/*      <argument value="20"/> */
  SURFXML_BUFFER_SET(argument_value, "20");
  SURFXML_START_TAG(argument);
  SURFXML_END_TAG(argument);

/*      <argument value="5000000"/> */
  SURFXML_BUFFER_SET(argument_value, "5000000");
  SURFXML_START_TAG(argument);
  SURFXML_END_TAG(argument);

/*      <argument value="100000"/> */
  SURFXML_BUFFER_SET(argument_value, "100000");
  SURFXML_START_TAG(argument);
  SURFXML_END_TAG(argument);

/*      <argument value="host B"/> */
  SURFXML_BUFFER_SET(argument_value, "host B");
  SURFXML_START_TAG(argument);
  SURFXML_END_TAG(argument);

/*   </process> */
  SURFXML_END_TAG(process);

/*   <process host="host B" function="slave"/> */
  SURFXML_BUFFER_SET(process_host, "host B");
  SURFXML_BUFFER_SET(process_function, "slave");
  SURFXML_BUFFER_SET(process_start_time, "-1.0");
  SURFXML_BUFFER_SET(process_kill_time, "-1.0");
  SURFXML_START_TAG(process);
  SURFXML_END_TAG(process);

/* </platform_description> */
  SURFXML_END_TAG(platform_description);

  free(surfxml_bufferstack);
  return 0;
}

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
MSG_error_t test_all(void);

typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

/** Emitter function  */
int master(int argc, char *argv[])
{
  int slaves_count = 0;
  m_host_t *slaves = NULL;
  m_task_t *todo = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;


  int i;

  xbt_assert1(sscanf(argv[1],"%d", &number_of_tasks),
	 "Invalid argument %s\n",argv[1]);
  xbt_assert1(sscanf(argv[2],"%lg", &task_comp_size),
	 "Invalid argument %s\n",argv[2]);
  xbt_assert1(sscanf(argv[3],"%lg", &task_comm_size),
	 "Invalid argument %s\n",argv[3]);

  {                  /*  Task creation */
    char sprintf_buffer[64];

    todo = calloc(number_of_tasks, sizeof(m_task_t));

    for (i = 0; i < number_of_tasks; i++) {
      sprintf(sprintf_buffer, "Task_%d", i);
      todo[i] = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, NULL);
    }
  }

  {                  /* Process organisation */
    slaves_count = argc - 4;
    slaves = calloc(slaves_count, sizeof(m_host_t));
    
    for (i = 4; i < argc; i++) {
      slaves[i-4] = MSG_get_host_by_name(argv[i]);
      if(slaves[i-4]==NULL) {
	INFO1("Unknown host %s. Stopping Now! ", argv[i]);
	abort();
      }
    }
  }

  INFO1("Got %d slave(s) :", slaves_count);
  for (i = 0; i < slaves_count; i++)
    INFO1("\t %s", slaves[i]->name);

  INFO1("Got %d task to process :", number_of_tasks);

  for (i = 0; i < number_of_tasks; i++)
    INFO1("\t\"%s\"", todo[i]->name);

  for (i = 0; i < number_of_tasks; i++) {
    INFO2("Sending \"%s\" to \"%s\"",
                  todo[i]->name,
                  slaves[i % slaves_count]->name);
    if(MSG_host_self()==slaves[i % slaves_count]) {
      INFO0("Hey ! It's me ! :)");
    }
    MSG_task_put(todo[i], slaves[i % slaves_count],
                 PORT_22);
    INFO0("Send completed");
  }
  
  INFO0("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) 
    MSG_task_put(MSG_task_create("finalize", 0, 0, FINALIZE),
		 slaves[i], PORT_22);
  
  INFO0("Goodbye now!");
  free(slaves);
  free(todo);
  return 0;
} /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
  INFO0("I'm a slave");
  while(1) {
    m_task_t task = NULL;
    int a;
    a = MSG_task_get(&(task), PORT_22);
    if (a == MSG_OK) {
      INFO1("Received \"%s\" ", MSG_task_get_name(task));
      if(MSG_task_get_data(task)==FINALIZE) {
	MSG_task_destroy(task);
	break;
      }
      INFO1("Processing \"%s\" ", MSG_task_get_name(task));
      MSG_task_execute(task);
      INFO1("\"%s\" done ", MSG_task_get_name(task));
      MSG_task_destroy(task);
    } else {
      INFO0("Hey ?! What's up ? ");
      xbt_assert0(0,"Unexpected behavior");
    }
  }
  INFO0("I'm done. See you!");
  return 0;
} /* end_of_slave */

/** Test function */
MSG_error_t test_all(void)
{
  MSG_error_t res = MSG_OK;

   /*  Simulation setting */
   MSG_set_channel_number(MAX_CHANNEL);
   MSG_paje_output("msg_test.trace");
   surf_parse = surf_parse_bypass_platform;
   MSG_create_environment(NULL);
  
   /*   Application deployment */
   MSG_function_register("master", master);
   MSG_function_register("slave", slave);
   surf_parse = surf_parse_bypass_application;
   MSG_launch_application(NULL);
   
   res = MSG_main();
  
  INFO1("Simulation time %g",MSG_get_clock());
  return res;
} /* end_of_test_all */

/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc,argv);
  res = test_all();
  MSG_clean();

  if(res==MSG_OK) return 0; 
  else return 1;
} /* end_of_main */

