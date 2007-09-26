#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"

int main(int argc, char **argv) {
  /* initialisation of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* creation of the tasks and their dependencies */
  SD_task_t taskInit = SD_task_create(NULL,NULL,1.0);
  SD_task_t taskA = SD_task_create("Task Comm 1", NULL, 1.0);
  SD_task_t taskB = SD_task_create("Task Comm 2", NULL, 1.0);
  

  /* scheduling parameters */

  double communication_amount1[] = { 0, 100000000, 0, 0 };
  double communication_amount2[] = { 0, 1, 0, 0 };
  const double no_cost[] = {1.0, 1.0};
  
  /* let's launch the simulation! */

  SD_task_schedule(taskInit, 1, SD_workstation_get_list(), no_cost, no_cost, -1.0);
  SD_task_schedule(taskA, 2, SD_workstation_get_list(), no_cost, communication_amount1, -1.0);
  SD_task_schedule(taskB, 2, SD_workstation_get_list(), no_cost, communication_amount2, -1.0);

  SD_task_dependency_add(NULL, NULL, taskInit, taskA);
  SD_task_dependency_add(NULL, NULL, taskInit, taskB);

  SD_simulate(-1.0);

  SD_exit();
  return 0;
}

