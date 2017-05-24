/* Copyright (c) 2010-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include "xbt/sysdep.h"
#include "simgrid/s4u/Link.hpp"
#include <simgrid/s4u.hpp>
#include <string>
#include "simgrid/plugins/energy.h"
#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_energyconsumption,
		"Messages specific for this s4u example");

class Sender {

	double comm_size;

	simgrid::s4u::MailboxPtr mailbox { };
	simgrid::s4u::Host* src_host { };
	simgrid::s4u::Host* dst_host { };
	std::vector<simgrid::s4u::Link*>* links { };

public:
	explicit Sender(std::vector<std::string> args) {
		xbt_assert(args.size() == 2,
				"The master function expects 1 arguments from the XML deployment file");

		comm_size = std::stod(args[1]);

		links = new std::vector<simgrid::s4u::Link*>();
		src_host = simgrid::s4u::Host::by_name("Host0");
		dst_host = simgrid::s4u::Host::by_name("Host2");

	}

	void operator()() {

		double total_energy = 0.0;

		for (int i = 0; i < 1; i++) {

			mailbox = simgrid::s4u::Mailbox::byName(std::string("message"));

			/* - Send the task to the @ref worker */
			char* payload = bprintf("%f", comm_size);

			XBT_INFO("Traffic %f", comm_size);

			simgrid::s4u::this_actor::send(mailbox, payload, comm_size);

		}
		XBT_INFO("Sender done");

		mailbox = simgrid::s4u::Mailbox::byName(std::string("message"));
		simgrid::s4u::this_actor::send(mailbox, xbt_strdup("finalize"), 0);
	}
};

class Receiver {

	simgrid::s4u::MailboxPtr mailbox { };
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
		mailbox = {};

	}

	void operator()() {

		XBT_INFO("Receiving ...");
		double total_energy = 0.0;

		while (1) {

			mailbox = simgrid::s4u::Mailbox::byName(std::string("message"));

			char* res = static_cast<char*>(simgrid::s4u::this_actor::recv(mailbox));
			xbt_assert(res != nullptr, "Some problem");

			if (strcmp(res, "finalize") == 0) { /* - Exit if 'finalize' is received */
				xbt_free(res);
				break;
			}
			xbt_free(res);
		}
		XBT_INFO("Receiver done");

		src_host->routeTo(dst_host, links, nullptr);

		xbt_assert(!links->empty(),
				"You're trying to send data from %s to %s but there is no connecting path between these two hosts.",
				src_host->cname(), dst_host->cname());

		for (auto link : *links) {

			XBT_INFO("From \"%s\" to \"%s\" link \"%s\" bandwidth %f energy %f link usage %f",
					src_host->cname(), dst_host->cname(), link->name(),
					link->bandwidth(), sg_link_get_consumed_energy(link), sg_link_get_usage(link));
			total_energy += sg_link_get_consumed_energy(link);

		}
		XBT_INFO("Total energy from the receiver: %f", total_energy);

	}
};

class Terminator {

public:
	explicit Terminator(std::vector<std::string> args) {

		XBT_INFO("Terminator ...");
	}

	void operator()() {

		MSG_process_sleep(1);
		sg_on_simulation_end();

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
	e->registerFunction<Terminator>("terminator");

	e->loadDeployment(argv[2]); /** - Deploy the application */
	e->run(); /** - Run the simulation */

	return 0;
}
