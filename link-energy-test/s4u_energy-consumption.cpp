/* Copyright (c) 2010-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include "xbt/sysdep.h"
#include <simgrid/s4u.hpp>
#include <string>
#include "simgrid/plugins/energy.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_masterworker,
		"Messages specific for this s4u example");

class Master {
//  long number_of_tasks             = 0; /* - Number of tasks      */
	double comp_size = 0; /* - Task compute cost    */
	double comm_size = 0; /* - Task communication size */
	long workers_count = 0; /* - Number of workers    */
	simgrid::s4u::MailboxPtr mailbox = nullptr;

public:
	explicit Master(std::vector<std::string> args) {
		xbt_assert(args.size() == 4,
				"The master function expects 3 arguments from the XML deployment file");

		comp_size = std::stod(args[1]);
		comm_size = std::stod(args[2]);
		workers_count = std::stol(args[3]);

		XBT_INFO("Got some work to do");
	}

	void operator()() {
		mailbox = simgrid::s4u::Mailbox::byName(std::string("task-0"));

		/* - Send the task to the @ref worker */
		char* payload = bprintf("%f", comp_size);
		simgrid::s4u::this_actor::send(mailbox, payload, comm_size);

		XBT_INFO(
				"All tasks have been dispatched. Let's tell everybody the computation is over.");

		/* - Eventually tell all the workers to stop by sending a "finalize" task */
		mailbox = simgrid::s4u::Mailbox::byName(std::string("task-0"));
		simgrid::s4u::this_actor::send(mailbox, xbt_strdup("finalize"), 0);

	}
};

class Worker {
	long id = -1;
	simgrid::s4u::MailboxPtr mailbox1 = nullptr;
	double comm_size;

public:
	explicit Worker(std::vector<std::string> args) {
		xbt_assert(args.size() == 2,
				"The worker expects a single argument from the XML deployment file: "
						"its worker ID (its numerical rank)");
		id = std::stol(args[1]);
		mailbox1 = simgrid::s4u::Mailbox::byName(std::string("task-0"));

	}

	void operator()() {
		while (1) { /* The worker waits in an infinite loop for tasks sent by the \ref master */
			char* res1 = static_cast<char*>(simgrid::s4u::this_actor::recv(
					mailbox1));

			xbt_assert(res1 != nullptr, "MSG_task_get failed");

			if (strcmp(res1, "finalize") == 0) { /* - Exit if 'finalize' is received */
				xbt_free(res1);
				break;
			}
			/*  - Otherwise, process the task */
			double comp_size1 = std::stod(res1);

			xbt_free(res1);
			simgrid::s4u::this_actor::execute(comp_size1);


		}
		XBT_INFO("I'm done. See you!");
	}
};

int main(int argc, char* argv[]) {
	//sg_link_energy_plugin_init();
	simgrid::s4u::Engine* e = new simgrid::s4u::Engine(&argc, argv);
	xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
			"\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0],
			argv[0]);

	e->loadPlatform(argv[1]); /** - Load the platform description */
	e->registerFunction<Master>("master");
	e->registerFunction<Worker>("worker"); /** - Register the function to be executed by the processes */
	e->loadDeployment(argv[2]); /** - Deploy the application */

	e->run(); /** - Run the simulation */

	XBT_INFO("Simulation time %g", e->getClock());

	return 0;
}
