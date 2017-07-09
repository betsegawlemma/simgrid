/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include "xbt/sysdep.h"
#include "simgrid/s4u/Link.hpp"
#include <simgrid/s4u.hpp>
#include <string>
#include "simgrid/plugins/link_energy.h"
#include "simgrid/msg.h"

#include <random>

/* Parameters of the random generation of the flow size */
static const unsigned long int min_size = 1e6;
static const unsigned long int max_size = 1e9;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_energyconsumption,
    "Messages specific for this s4u example");

void sender(std::vector<std::string> args) {
   xbt_assert(args.size() == 2, "The master function expects 2 arguments.");
   int flow_amount = std::stoi(args.at(0));
   double comm_size = std::stod(args.at(1));
   XBT_INFO("Send %.0f bytes, in %d flows", comm_size, flow_amount);

   simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(std::string("message"));

   /* - Send the task to the @ref worker */
   char* payload = bprintf("%f", comm_size);

   if (flow_amount == 1) {
	   simgrid::s4u::this_actor::send(mailbox, payload, comm_size);
   } else {
	   // Start all comms in parallel
	   std::vector<simgrid::s4u::CommPtr> comms;
	   for (int i=0; i<flow_amount; i++)
		   comms.push_back(simgrid::s4u::this_actor::isend(mailbox,
				   	   const_cast<char*>("message"), comm_size));

	   // And now, wait for all comms. Manually since wait_all is not part of this_actor yet
	   for (int i=0; i<flow_amount; i++) {
		   simgrid::s4u::CommPtr comm = comms.at(i);
		   comm->wait();
	   }
	   comms.clear();
   }
   XBT_INFO("sender done %f",MSG_get_clock());
}

void receiver(std::vector<std::string> args) {
	int flow_amount = std::stoi(args.at(0));

   XBT_INFO("Receiving %d flows ...", flow_amount);

   simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(std::string("message"));

   if (flow_amount == 1) {
	   char* res = static_cast<char*>(simgrid::s4u::this_actor::recv(mailbox));
	   xbt_assert(res != nullptr, "Some problem");
	   xbt_free(res);
   } else {
	   void *ignored;

	   // Start all comms in parallel
	   std::vector<simgrid::s4u::CommPtr> comms;
	   for (int i=0; i<flow_amount; i++)
		   comms.push_back(simgrid::s4u::this_actor::irecv(mailbox, &ignored));

	   // And now, wait for all comms. Manually since wait_all is not part of this_actor yet
	   for (int i=0; i<flow_amount; i++)
		   comms.at(i)->wait();
	   comms.clear();
   }
   XBT_INFO("receiver done %f",MSG_get_clock());
}

int main(int argc, char* argv[]) {

  /* Check if we got --NS3 on the command line, and activate ecofen if so */
  bool NS3 = false;
  for (int i = 0; i<argc; i++) {
     if (strcmp(argv[i], "--NS3") == 0) {
	NS3 = true;
     }
     if (NS3)  // Found the --NS3 parameter; shift the rest of the line
       argv[i] = argv[i+1];  
  }
  if (NS3) {
    XBT_INFO("Activating the Ecofen energy plugin");
    ns3_link_energy_plugin_init();
    argc -= 1; // We removed it from the parameters
  } else {
    XBT_INFO("Activating the SimGrid link energy plugin");
    sg_link_energy_plugin_init();
  }

  /* Now initialize the S4U engine, and load the platform */
  simgrid::s4u::Engine* e = new simgrid::s4u::Engine(&argc, argv);
  if (NS3)
     xbt_cfg_set_parse("network/model:NS3");
  else
	  /* Use the simple, intuitive model of SimGrid, even if it's wrong on slow start */
	 xbt_cfg_set_parse("network/model:CM02");


  xbt_assert(argc > 1, "\nUsage: %s platform_file [datasize] [--NS3]\n"
      "\tExample: %s s4uplatform.xml \n"
      "\tIf you add NS3 as last parameter, this module will try to activate the ecofen plugin.\n"
      "\tWithout it, it will use the SimGrid link energy plugin.\n", argv[0],
      argv[0]);
  e->loadPlatform(argv[1]);

  /* prepare to launch the actors */
  std::vector<std::string> argSender;
  std::vector<std::string> argReceiver;
  if (argc > 2) {
	  argSender.push_back(argv[2]); // Take the amount of flows from the command line
	  argReceiver.push_back(argv[2]);
  } else {
	  argSender.push_back("1"); // Default value
	  argReceiver.push_back("1");
  }
  if (argc > 3) {
	 if (strcmp(argv[3], "random") == 0) { // We're asked to get a random size
	  /* Initialize the random number generator */
	  std::random_device rd;
	  std::default_random_engine generator(rd());

	  /* Distribution on which to apply the generator */
	  std::uniform_int_distribution<unsigned long int> distribution(min_size, max_size);

	  char* size = bprintf("%lu", distribution(generator));
	  argSender.push_back(std::string(size));
	  xbt_free(size);
	 } else { // Not "random" ? Then it should be the size to use
		 argSender.push_back(argv[3]); // Take the datasize from the command line
	 }
  } else { // No parameter at all? Then use the default value
	  argSender.push_back("1000");
  }
  simgrid::s4u::Actor::createActor("sender", simgrid::s4u::Host::by_name("SRC"), sender, argSender);
  simgrid::s4u::Actor::createActor("receiver", simgrid::s4u::Host::by_name("DST"), receiver, argReceiver);

  /* And now, launch the simulation */
  e->run();

  return 0;
}
