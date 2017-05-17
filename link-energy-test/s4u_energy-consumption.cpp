/* Copyright (c) 2010-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include "xbt/sysdep.h"
#include "simgrid/s4u/Link.hpp"
#include <simgrid/s4u.hpp>
#include <string>
#include "simgrid/plugins/energy.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_energyconsumption,
		"Messages specific for this s4u example");

class Sender {

	double traffic;
	long workers_count; /* - Number of workers    */
	simgrid::s4u::MailboxPtr mailbox { };
	simgrid::s4u::Host* src_host { };
	simgrid::s4u::Host* dst_host { };
	std::vector<simgrid::s4u::Link*>* links { };

public:
	explicit Sender(std::vector<std::string> args) {
		xbt_assert(args.size() == 3,
				"The master function expects 2 arguments from the XML deployment file");

		traffic = std::stod(args[1]);
		workers_count = std::stol(args[2]);
		links = new std::vector<simgrid::s4u::Link*>();
		src_host = simgrid::s4u::Host::by_name("Host0");
		dst_host = simgrid::s4u::Host::by_name("Host2");
		XBT_INFO("Worker count %ld", workers_count);
		XBT_INFO("Traffic to be sent %f", traffic);
	}

	void operator()() {

		mailbox = simgrid::s4u::Mailbox::byName(std::string("mail"));

		/* - Send the task to the @ref worker */
		char* payload = bprintf("%f", traffic);

		src_host->routeTo(dst_host, links, nullptr);

		xbt_assert(!links->empty(),
				"You're trying to send data from %s to %s but there is no connecting path between these two hosts.",
				src_host->cname(), dst_host->cname());
		simgrid::s4u::this_actor::send(mailbox, payload, 0.0);
		//simgrid::s4u::this_actor::send(mailbox, payload, comm_size);
		XBT_INFO("In Master --");


		for (auto link : *links) {



			XBT_INFO(
					"Sending  task-0 from \"%s\" to \"%s\" link \"%s\" bandwidth %f energy %f",
					src_host->cname(), dst_host->cname(), link->name(),
					link->bandwidth(),sg_link_get_consumed_energy(link));
		}
		XBT_INFO("Sender done");
	}
};

class Receiver {

	simgrid::s4u::MailboxPtr mailbox = nullptr;
	std::vector<simgrid::s4u::Link*>* links { };
	simgrid::s4u::Host* src_host { };
	simgrid::s4u::Host* dst_host { };
public:
	explicit Receiver(std::vector<std::string> args) {
		xbt_assert(args.size() == 1,
				"The worker expects a single argument from the XML deployment file: "
						"its worker ID (its numerical rank)");
		links = new std::vector<simgrid::s4u::Link*>();
		src_host = simgrid::s4u::Host::by_name("Host0");
		dst_host = simgrid::s4u::Host::by_name("Host2");
		mailbox = simgrid::s4u::Mailbox::byName("mail");

	}

	void operator()() {

		char* payload = static_cast<char*>(simgrid::s4u::this_actor::recv(
				mailbox));

		xbt_assert(payload != nullptr, "MSG_task_get failed");

		XBT_INFO("I have received: %s", payload);
		dst_host->routeTo(src_host, links, nullptr);
		for (auto link : *links) {

			XBT_INFO(
					"Route from \"%s\" to \"%s\" link \"%s\" bandwidth %f energy %f",
					src_host->cname(), dst_host->cname(), link->name(),
					link->bandwidth(),sg_link_get_consumed_energy(link));
		}

		xbt_free(payload);

		XBT_INFO("Receiver done!");

	}
};

int main(int argc, char* argv[]) {

	sg_link_energy_plugin_init();

	simgrid::s4u::Engine* e = new simgrid::s4u::Engine(&argc, argv);
	xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
			"\tExample: %s s4uplatform.xml s4u_deployment.xml\n", argv[0],
			argv[0]);

	e->loadPlatform(argv[1]); /** - Load the platform description */
	e->registerFunction<Sender>("sender");
	e->registerFunction<Receiver>("receiver"); /** - Register the function to be executed by the processes */
	e->loadDeployment(argv[2]); /** - Deploy the application */
	e->run(); /** - Run the simulation */

	XBT_INFO("Simulation time %g", e->getClock());

	return 0;
}
