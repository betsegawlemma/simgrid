/*
   See examples/platforms/metaxml.xml and examples/platforms/metaxml_platform.xml files for examples on how to use the cluster, foreach, set, route:multi, trace and trace:connect tags
*/
#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/time.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_test,
			     "Logging specific to this SimDag example");

int main(int argc, char **argv) {
  /* initialisation of SD */
  SD_init(&argc, argv);

  /*  xbt_log_control_set("sd.thres=debug"); */

  if (argc < 2) {
    INFO1("Usage: %s platform_file", argv[0]);
    INFO1("example: %s sd_platform.xml", argv[0]);
    exit(1);
  }

  /* creation of the environment */
  const char * platform_file = argv[1];
  SD_create_environment(platform_file);

  /* test the estimation functions */
  const SD_workstation_t *workstations = SD_workstation_get_list();
  int ws_nr = SD_workstation_get_number();

  SD_workstation_t w1 = NULL;
  SD_workstation_t w2 = NULL;
  const char *name1, *name2;
  /* Show routes between all workstation */
  int i,j,k;
  for (i=0; i<ws_nr; i++){
    for (j=0;j<ws_nr; j++){
      w1 = workstations[i];
      w2 = workstations[j];
      name1 = SD_workstation_get_name(w1);
      name2 = SD_workstation_get_name(w2);
      INFO2("Route between %s and %s:", name1, name2);   
      const SD_link_t *route = SD_route_get_list(w1, w2);
      int route_size = SD_route_get_size(w1, w2);
      for (k = 0; k < route_size; k++) {
        INFO3("\tLink %s: latency = %f, bandwidth = %f", SD_link_get_name(route[k]),
	  SD_link_get_current_latency(route[k]), SD_link_get_current_bandwidth(route[k]));
      }
    }
  }

  SD_exit();
  return 0;
}

