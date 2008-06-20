#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"

int main(int argc, char **argv) {

	/* creation of the tasks and their dependencies */
    SD_task_t taskInit;
    SD_task_t taskA;
    SD_task_t taskB;


    /* scheduling parameters */

    double no_cost[] = { 0., 0., 0., 0. };
    double amount[] = { 0., 100000., 0., 0.};
    double comput[] = { 10000000. };

    /* initialisation of SD */
    SD_init(&argc, argv);

    /* creation of the environment */
    SD_create_environment(argv[1]);

    /* creation of the tasks and their dependencies */
    taskInit = SD_task_create("Task Init",NULL,1.0);
    taskA = SD_task_create("Task A", NULL, 1.0);
    taskB = SD_task_create("Task B", NULL, 1.0);

    /* let's launch the simulation! */
    SD_task_schedule(taskInit, 1, SD_workstation_get_list(), no_cost, no_cost, -1.0);
    SD_task_schedule(taskA, 2, SD_workstation_get_list(), no_cost, amount, -1.0);
    SD_task_schedule(taskB, 1, SD_workstation_get_list(), comput, no_cost, -1.0);

    SD_task_dependency_add(NULL, NULL, taskInit, taskA);
    SD_task_dependency_add(NULL, NULL, taskInit, taskB);
    
    SD_simulate(-1.0);

    SD_exit();
    return 0;
}
