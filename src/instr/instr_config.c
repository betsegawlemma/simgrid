/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */


#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_CATEGORY(instr, "Logging the behavior of the tracing system (used for Visualization/Analysis of simulations)");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_config, instr, "Configuration");

#define OPT_TRACING               "tracing"
#define OPT_TRACING_PLATFORM      "tracing/platform"
#define OPT_TRACING_SMPI          "tracing/smpi"
#define OPT_TRACING_SMPI_GROUP    "tracing/smpi/group"
#define OPT_TRACING_CATEGORIZED   "tracing/categorized"
#define OPT_TRACING_UNCATEGORIZED "tracing/uncategorized"
#define OPT_TRACING_MSG_TASK      "tracing/msg/task"
#define OPT_TRACING_MSG_PROCESS   "tracing/msg/process"
#define OPT_TRACING_FILENAME      "tracing/filename"
#define OPT_TRACING_BUFFER        "tracing/buffer"
#define OPT_TRACING_ONELINK_ONLY  "tracing/onelink_only"
#define OPT_TRIVA_UNCAT_CONF      "triva/uncategorized"
#define OPT_TRIVA_CAT_CONF        "triva/categorized"

static int trace_enabled;
static int trace_platform;
static int trace_smpi_enabled;
static int trace_smpi_grouped;
static int trace_categorized;
static int trace_uncategorized;
static int trace_msg_task_enabled;
static int trace_msg_process_enabled;
static int trace_buffer;
static int trace_onelink_only;

static int trace_configured = 0;
static int trace_active = 0;

xbt_dict_t created_categories; //declared in instr_interface.c

static void TRACE_getopts(void)
{
  trace_enabled = xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING);
  trace_platform = xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_PLATFORM);
  trace_smpi_enabled = xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_SMPI);
  trace_smpi_grouped = xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_SMPI_GROUP);
  trace_categorized = xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_CATEGORIZED);
  trace_uncategorized = xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_UNCATEGORIZED);
  trace_msg_task_enabled = xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_TASK);
  trace_msg_process_enabled = xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_PROCESS);
  trace_buffer = xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_BUFFER);
  trace_onelink_only = xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_ONELINK_ONLY);
}

int TRACE_start()
{
  TRACE_getopts();

  // tracing system must be:
  //    - enabled (with --cfg=tracing:1)
  //    - already configured (TRACE_global_init already called)
  if (!(TRACE_is_enabled() && TRACE_is_configured())){
    return 0;
  }

  XBT_DEBUG("Tracing starts");

  /* open the trace file */
  TRACE_paje_start();

  /* activate trace */
  TRACE_activate ();

  /* other trace initialization */
  created_categories = xbt_dict_new();
  TRACE_surf_alloc();
  TRACE_smpi_alloc();
  return 0;
}

int TRACE_end()
{
  if (!TRACE_is_active())
    return 1;

  /* generate uncategorized graph configuration for triva */
  if (TRACE_get_triva_uncat_conf()){
    TRACE_generate_triva_uncat_conf();
  }

  /* generate categorized graph configuration for triva */
  if (TRACE_get_triva_cat_conf()){
    TRACE_generate_triva_cat_conf();
  }

  /* dump trace buffer */
  TRACE_last_timestamp_to_dump = surf_get_clock();
  TRACE_paje_dump_buffer(1);

  /* destroy all data structures of tracing (and free) */
  destroyAllContainers();

  /* close the trace file */
  TRACE_paje_end();

  /* activate trace */
  TRACE_desactivate ();
  XBT_DEBUG("Tracing system is shutdown");
  return 0;
}

void TRACE_activate (void)
{
  xbt_assert (trace_active==0, "Tracing is already active.");
  trace_active = 1;
  XBT_DEBUG ("Tracing is on");
}

void TRACE_desactivate (void)
{
  trace_active = 0;
  XBT_DEBUG ("Tracing is off");
}

int TRACE_is_active (void)
{
  return trace_active;
}

int TRACE_needs_platform (void)
{
  return TRACE_msg_process_is_enabled() ||
         TRACE_msg_task_is_enabled() ||
         TRACE_categorized() ||
         TRACE_uncategorized() ||
         TRACE_platform () ||
         (TRACE_smpi_is_enabled() && TRACE_smpi_is_grouped());
}

int TRACE_is_enabled(void)
{
  return trace_enabled;
}

int TRACE_platform(void)
{
  return trace_platform;
}

int TRACE_is_configured(void)
{
  return trace_configured;
}

int TRACE_smpi_is_enabled(void)
{
  return (xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_SMPI) ||
       TRACE_smpi_is_grouped())&&
      TRACE_is_enabled();
}

int TRACE_smpi_is_grouped(void)
{
  return trace_smpi_grouped;
}

int TRACE_categorized (void)
{
  return trace_categorized;
}

int TRACE_uncategorized (void)
{
  return trace_uncategorized;
}

int TRACE_msg_task_is_enabled(void)
{
  return trace_msg_task_enabled && TRACE_is_enabled();
}

int TRACE_msg_process_is_enabled(void)
{
  return trace_msg_process_enabled && TRACE_is_enabled();
}

int TRACE_buffer (void)
{
  return trace_buffer && TRACE_is_enabled();
}

int TRACE_onelink_only (void)
{
  return trace_onelink_only && TRACE_is_enabled();
}

char *TRACE_get_filename(void)
{
  return xbt_cfg_get_string(_surf_cfg_set, OPT_TRACING_FILENAME);
}

char *TRACE_get_triva_uncat_conf (void)
{
  return xbt_cfg_get_string(_surf_cfg_set, OPT_TRIVA_UNCAT_CONF);
}

char *TRACE_get_triva_cat_conf (void)
{
  return xbt_cfg_get_string(_surf_cfg_set, OPT_TRIVA_CAT_CONF);
}

void TRACE_global_init(int *argc, char **argv)
{
  /* name of the tracefile */
  char *default_tracing_filename = xbt_strdup("simgrid.trace");
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_FILENAME,
                   "Trace file created by the instrumented SimGrid.",
                   xbt_cfgelm_string, &default_tracing_filename, 1, 1,
                   NULL, NULL);

  /* tracing */
  int default_tracing = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING,
                   "Enable Tracing.",
                   xbt_cfgelm_int, &default_tracing, 0, 1,
                   NULL, NULL);

  /* tracing platform*/
  int default_tracing_platform = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_PLATFORM,
                   "Enable Tracing Platform.",
                   xbt_cfgelm_int, &default_tracing_platform, 0, 1,
                   NULL, NULL);

  /* smpi */
  int default_tracing_smpi = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_SMPI,
                   "Tracing of the SMPI interface.",
                   xbt_cfgelm_int, &default_tracing_smpi, 0, 1,
                   NULL, NULL);

  /* smpi grouped */
  int default_tracing_smpi_grouped = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_SMPI_GROUP,
                   "Group MPI processes by host.",
                   xbt_cfgelm_int, &default_tracing_smpi_grouped, 0, 1,
                   NULL, NULL);


  /* platform */
  int default_tracing_categorized = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_CATEGORIZED,
                   "Tracing of categorized platform (host and link) utilization.",
                   xbt_cfgelm_int, &default_tracing_categorized, 0, 1,
                   NULL, NULL);

  /* tracing uncategorized resource utilization */
  int default_tracing_uncategorized = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_UNCATEGORIZED,
                   "Tracing of uncategorized resource (host and link) utilization.",
                   xbt_cfgelm_int, &default_tracing_uncategorized, 0, 1,
                   NULL, NULL);

  /* msg task */
  int default_tracing_msg_task = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_MSG_TASK,
                   "Tracing of MSG task behavior.",
                   xbt_cfgelm_int, &default_tracing_msg_task, 0, 1,
                   NULL, NULL);

  /* msg process */
  int default_tracing_msg_process = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_MSG_PROCESS,
                   "Tracing of MSG process behavior.",
                   xbt_cfgelm_int, &default_tracing_msg_process, 0, 1,
                   NULL, NULL);

  /* tracing buffer */
  int default_buffer = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_BUFFER,
                   "Buffer trace events to put them in temporal order.",
                   xbt_cfgelm_int, &default_buffer, 0, 1,
                   NULL, NULL);

  /* tracing one link only */
  int default_onelink_only = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_ONELINK_ONLY,
                   "Use only routes with one link to trace platform.",
                   xbt_cfgelm_int, &default_onelink_only, 0, 1,
                   NULL, NULL);

  /* Triva graph configuration for uncategorized tracing */
  char *default_triva_uncat_conf_file = xbt_strdup ("");
  xbt_cfg_register(&_surf_cfg_set, OPT_TRIVA_UNCAT_CONF,
                   "Triva Graph configuration file for uncategorized resource utilization traces.",
                   xbt_cfgelm_string, &default_triva_uncat_conf_file, 1, 1,
                   NULL, NULL);

  /* Triva graph configuration for uncategorized tracing */
  char *default_triva_cat_conf_file = xbt_strdup ("");
  xbt_cfg_register(&_surf_cfg_set, OPT_TRIVA_CAT_CONF,
                   "Triva Graph configuration file for categorized resource utilization traces.",
                   xbt_cfgelm_string, &default_triva_cat_conf_file, 1, 1,
                   NULL, NULL);

  /* instrumentation can be considered configured now */
  trace_configured = 1;
}

static void print_line (const char *option, const char *desc, const char *longdesc, int detailed)
{
  char str[INSTR_DEFAULT_STR_SIZE];
  snprintf (str, INSTR_DEFAULT_STR_SIZE, "--cfg=%s ", option);

  int len = strlen (str);
  printf ("%s%*.*s %s\n", str, 30-len, 30-len, "", desc);
  if (!!longdesc && detailed){
    printf ("%s\n\n", longdesc);
  }
}

void TRACE_help (int detailed)
{
  printf(
      "Description of the tracing options accepted by this simulator:\n\n");
  print_line (OPT_TRACING, "Enable the tracing system",
      "  It activates the tracing system and register the simulation platform\n"
      "  in the trace file. You have to enable this option to others take effect.",
      detailed);
  print_line (OPT_TRACING_CATEGORIZED, "Trace categorized resource utilization",
      "  It activates the categorized resource utilization tracing. It should\n"
      "  be enabled if tracing categories are used by this simulator.",
      detailed);
  print_line (OPT_TRACING_UNCATEGORIZED, "Trace uncategorized resource utilization",
      "  It activates the uncategorized resource utilization tracing. Use it if\n"
      "  this simulator do not use tracing categories and resource use have to be\n"
      "  traced.",
      detailed);
  print_line (OPT_TRACING_FILENAME, "Filename to register traces",
      "  A file with this name will be created to register the simulation. The file\n"
      "  is in the Paje format and can be analyzed using Triva or Paje visualization\n"
      "  tools. More information can be found in these webpages:\n"
      "     http://triva.gforge.inria.fr/\n"
      "     http://paje.sourceforge.net/",
      detailed);
  print_line (OPT_TRACING_SMPI, "Trace the MPI Interface (SMPI)",
      "  This option only has effect if this simulator is SMPI-based. Traces the MPI\n"
      "  interface and generates a trace that can be analyzed using Gantt-like\n"
      "  visualizations. Every MPI function (implemented by SMPI) is transformed in a\n"
      "  state, and point-to-point communications can be analyzed with arrows.",
      detailed);
  print_line (OPT_TRACING_SMPI_GROUP, "Group MPI processes by host (SMPI)",
      "  This option only has effect if this simulator is SMPI-based. The processes\n"
      "  are grouped by the hosts where they were executed.",
      detailed);
  print_line (OPT_TRACING_MSG_TASK, "Trace task behavior (MSG)",
      "  This option only has effect if this simulator is MSG-based. It traces the\n"
      "  behavior of all categorized MSG tasks, grouping them by hosts.",
      detailed);
  print_line (OPT_TRACING_MSG_PROCESS, "Trace processes behavior (MSG)",
      "  This option only has effect if this simulator is MSG-based. It traces the\n"
      "  behavior of all categorized MSG processes, grouping them by hosts. This option\n"
      "  can be used to track process location if this simulator has process migration.",
      detailed);
  print_line (OPT_TRACING_BUFFER, "Buffer events to put them in temporal order",
      "  This option put some events in a time-ordered buffer using the insertion\n"
      "  sort algorithm. The process of acquiring and releasing locks to access this\n"
      "  buffer and the cost of the sorting algorithm make this process slow. The\n"
      "  simulator performance can be severely impacted if this option is activated,\n"
      "  but you are sure to get a trace file with events sorted.",
      detailed);
  print_line (OPT_TRACING_ONELINK_ONLY, "Consider only one link routes to trace platform",
      "  This option changes the way SimGrid register its platform on the trace file.\n"
      "  Normally, the tracing considers all routes (no matter their size) on the\n"
      "  platform file to re-create the resource topology. If this option is activated,\n"
      "  only the routes with one link are used to register the topology within an AS.\n"
      "  Routes among AS continue to be traced as usual.",
      detailed);
  print_line (OPT_TRIVA_UNCAT_CONF, "Generate graph configuration for Triva",
      "  This option can be used in all types of simulators build with SimGrid\n"
      "  to generate a uncategorized resource utilization graph to be used as\n"
      "  configuration for the Triva visualization analysis. This option\n"
      "  can be used with tracing/categorized:1 and tracing:1 options to\n"
      "  analyze an unmodified simulator before changing it to contain\n"
      "  categories.",
      detailed);
  print_line (OPT_TRIVA_CAT_CONF, "generate uncategorized graph configuration for Triva",
      "  This option can be used if this simulator uses tracing categories\n"
      "  in its code. The file specified by this option holds a graph configuration\n"
      "  file for the Triva visualization tool that can be used to analyze a categorized\n"
      "  resource utilization.",
      detailed);
}

void TRACE_generate_triva_uncat_conf (void)
{
  char *output = TRACE_get_triva_uncat_conf ();
  if (output && strlen(output) > 0){
    xbt_dict_cursor_t cursor=NULL;
    char *name, *value;

    FILE *file = fopen (output, "w");
    xbt_assert (file != NULL,
       "Unable to open file (%s) for writing triva graph "
       "configuration (uncategorized).", output);

    //open
    fprintf (file, "{\n");

    //register NODE types
    fprintf (file, "  node = (");
    xbt_dict_foreach(trivaNodeTypes, cursor, name, value) {
      fprintf (file, "%s, ", name);
    }

    //register EDGE types
    fprintf (file,
        ");\n"
        "  edge = (");
    xbt_dict_foreach(trivaEdgeTypes, cursor, name, value) {
      fprintf (file, "%s, ", name);
    }
    fprintf (file,
        ");\n"
        "\n");

    //configuration for all nodes
    fprintf (file,
        "  host = {\n"
        "    type = square;\n"
        "    size = power;\n"
        "    values = (power_used);\n"
        "  };\n"
        "  link = {\n"
        "    type = rhombus;\n"
        "    size = bandwidth;\n"
        "    values = (bandwidth_used);\n"
        "  };\n");
    //close
    fprintf (file, "}\n");
    fclose (file);
  }
}

void TRACE_generate_triva_cat_conf (void)
{
  char *output = TRACE_get_triva_cat_conf();
  if (output && strlen(output) > 0){
    xbt_dict_cursor_t cursor=NULL, cursor2=NULL;
    char *name, *name2, *value, *value2;

    //check if we do have categories declared
    if (xbt_dict_length(created_categories) == 0){
      XBT_INFO("No categories declared, ignoring generation of triva graph configuration");
      return;
    }

    FILE *file = fopen (output, "w");
    xbt_assert (file != NULL,
       "Unable to open file (%s) for writing triva graph "
       "configuration (categorized).", output);

    //open
    fprintf (file, "{\n");

    //register NODE types
    fprintf (file, "  node = (");
    xbt_dict_foreach(trivaNodeTypes, cursor, name, value) {
      fprintf (file, "%s, ", name);
    }

    //register EDGE types
    fprintf (file,
        ");\n"
        "  edge = (");
    xbt_dict_foreach(trivaEdgeTypes, cursor, name, value) {
      fprintf (file, "%s, ", name);
    }
    fprintf (file,
        ");\n"
        "\n");

    //configuration for all nodes
    fprintf (file,
        "  host = {\n"
        "    type = square;\n"
        "    size = power;\n"
        "    values = (");
    xbt_dict_foreach(created_categories,cursor2,name2,value2) {
      fprintf (file, "p%s, ", name2);
    }
    fprintf (file,
        ");\n"
        "  };\n"
        "  link = {\n"
        "    type = rhombus;\n"
        "    size = bandwidth;\n"
        "    values = (");
    xbt_dict_foreach(created_categories,cursor2,name2,value2) {
      fprintf (file, "b%s, ", name2);
    }
    fprintf (file,
        ");\n"
        "  };\n");
    //close
    fprintf (file, "}\n");
    fclose (file);
  }
}

#undef OPT_TRACING
#undef OPT_TRACING_PLATFORM
#undef OPT_TRACING_SMPI
#undef OPT_TRACING_SMPI_GROUP
#undef OPT_TRACING_CATEGORIZED
#undef OPT_TRACING_UNCATEGORIZED
#undef OPT_TRACING_MSG_TASK
#undef OPT_TRACING_MSG_PROCESS
#undef OPT_TRACING_MSG_VOLUME
#undef OPT_TRACING_FILENAME
#undef OPT_TRACING_PLATFORM_METHOD
#undef OPT_TRIVA_UNCAT_CONF
#undef OPT_TRIVA_CAT_CONF

#endif /* HAVE_TRACING */
