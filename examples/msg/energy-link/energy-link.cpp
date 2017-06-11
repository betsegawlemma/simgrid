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

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_energyconsumption,
    "Messages specific for this s4u example");

void sender(std::vector<std::string> args) {
   xbt_assert(args.size() == 1,
	      "The master function expects 1 arguments from the XML deployment file");
   XBT_INFO("Send '%s' bytes", args.at(0).c_str());
   double comm_size = std::stod(args.at(0));

   simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(std::string("message"));

   /* - Send the task to the @ref worker */
   char* payload = bprintf("%f", comm_size);

   simgrid::s4u::this_actor::send(mailbox, payload, comm_size);
   XBT_INFO("sender done %f",MSG_get_clock());
}

void receiver() {
   XBT_INFO("Receiving ...");

   simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(std::string("message"));

   char* res = static_cast<char*>(simgrid::s4u::this_actor::recv(mailbox));
   xbt_assert(res != nullptr, "Some problem");

   xbt_free(res);
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
   
  xbt_assert(argc > 1, "\nUsage: %s platform_file [datasize] [--NS3]\n"
      "\tExample: %s s4uplatform.xml \n"
      "\tIf you add NS3 as last parameter, this module will try to activate the ecofen plugin.\n"
      "\tWithout it, it will use the SimGrid link energy plugin.\n", argv[0],
      argv[0]);
  e->loadPlatform(argv[1]);
  
  /* prepare to launch the actors */
  std::vector<std::string> args;
  if (argc > 2) 
     args.push_back(argv[2]); // Take the datasize from the command line
  else 
     args.push_back("1000"); // Use the default datasize
   
  simgrid::s4u::Actor::createActor("sender", simgrid::s4u::Host::by_name("SRC"), sender, args);
  simgrid::s4u::Actor::createActor("receiver", simgrid::s4u::Host::by_name("DST"), receiver);
   
  /* And now, launch the simulation */
  e->run();

  return 0;
}
