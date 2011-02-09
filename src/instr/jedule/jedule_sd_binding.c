/*
 * jedule_sd_binding.c
 *
 *  Created on: Dec 2, 2010
 *      Author: sascha
 */


#include <stdio.h>

#include "xbt/asserts.h"
#include "xbt/dynar.h"

#include "surf/surf_private.h"
#include "surf/surf.h"

#include "instr/jedule/jedule_sd_binding.h"
#include "instr/jedule/jedule_events.h"
#include "instr/jedule/jedule_platform.h"
#include "instr/jedule/jedule_output.h"

#ifdef HAVE_JEDULE

jedule_t jedule;

void jedule_log_sd_event(SD_task_t task) {
	xbt_dynar_t host_list;
	jed_event_t event;
	int i;

	xbt_assert(task != NULL);

	host_list = xbt_dynar_new(sizeof(char*), NULL);

	for(i=0; i<task->workstation_nb; i++) {
		char *hostname = surf_resource_name(task->workstation_list[i]->surf_workstation);
		xbt_dynar_push(host_list, &hostname);
	}

	create_jed_event(&event,
			SD_task_get_name(task),
			task->start_time,
			task->finish_time,
			"SD");

	jed_event_add_resources(event, host_list);
	jedule_store_event(event);

	xbt_dynar_free(&host_list);
}

static void create_hierarchy(routing_component_t current_comp,
		jed_simgrid_container_t current_container) {
	xbt_dict_cursor_t cursor = NULL;
	char *key;
	routing_component_t elem;
	network_element_t network_elem;

	if( xbt_dict_length(current_comp->routing_sons) == 0 ) {
		// I am no AS
		// add hosts to jedule platform
		xbt_dynar_t hosts;

		hosts = xbt_dynar_new(sizeof(char*), NULL);

		xbt_dict_foreach(current_comp->to_index, cursor, key, network_elem) {
			char *hostname;
			printf("key %s value %d\n", key, network_elem);
			//xbt_dynar_push_as(hosts, char*, key);
			hostname = strdup(key);
			xbt_dynar_push(hosts, &hostname);
		}

		jed_simgrid_add_resources(current_container, hosts);

	} else {
		xbt_dict_foreach(current_comp->routing_sons, cursor, key, elem) {
			jed_simgrid_container_t child_container;
			jed_simgrid_create_container(&child_container, elem->name);
			jed_simgrid_add_container(current_container, child_container);
			printf("name : %s\n", elem->name);
			create_hierarchy(elem, child_container);
		}
	}
}

void jedule_setup_platform() {

	routing_component_t root_comp;
	e_surf_network_element_type_t type;

	jed_simgrid_container_t root_container;


	jed_create_jedule(&jedule);

	root_comp = global_routing->root;
	printf("root name %s\n", root_comp->name);

	// that doesn't work
	type = root_comp->get_network_element_type(root_comp->name);

	jed_simgrid_create_container(&root_container, root_comp->name);
	jedule->root_container = root_container;

	create_hierarchy(root_comp, root_container);

}


void jedule_sd_cleanup() {

	jedule_cleanup_output();
}

void jedule_sd_init() {

	jedule_init_output();
}

void jedule_sd_dump() {

	write_jedule_output(stdout, jedule, jedule_event_list, NULL);
}

#endif
