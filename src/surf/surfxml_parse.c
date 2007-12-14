/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "surf/surfxml_parse_private.h"
#include "surf/surf_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_parse, surf,
				"Logging specific to the SURF parsing module");

#undef CLEANUP
#include "surfxml.c"

/* Initialize the parsing globals */
int route_action = 0;
xbt_dict_t traces_set_list = NULL;
xbt_dynar_t traces_connect_list = NULL;

/* This buffer is used to store the original buffer before substituing it by out own buffer. Usefull for the foreach tag */
char* old_buff;
/* Stores the set name reffered to by the foreach tag */
static const char* foreach_set_name;
static xbt_dynar_t main_STag_surfxml_host_cb_list = NULL;
static xbt_dynar_t main_ETag_surfxml_host_cb_list = NULL;
static xbt_dynar_t main_STag_surfxml_link_cb_list = NULL;
static xbt_dynar_t main_ETag_surfxml_link_cb_list = NULL;

/* make sure these symbols are defined as strong ones in this file so that the linked can resolve them */
xbt_dynar_t STag_surfxml_platform_cb_list = NULL;
xbt_dynar_t ETag_surfxml_platform_cb_list = NULL;
xbt_dynar_t STag_surfxml_host_cb_list = NULL;
xbt_dynar_t ETag_surfxml_host_cb_list = NULL;
xbt_dynar_t STag_surfxml_router_cb_list = NULL;
xbt_dynar_t ETag_surfxml_router_cb_list = NULL;
xbt_dynar_t STag_surfxml_link_cb_list = NULL;
xbt_dynar_t ETag_surfxml_link_cb_list = NULL;
xbt_dynar_t STag_surfxml_route_cb_list = NULL;
xbt_dynar_t ETag_surfxml_route_cb_list = NULL;
xbt_dynar_t STag_surfxml_link_c_ctn_cb_list = NULL;
xbt_dynar_t ETag_surfxml_link_c_ctn_cb_list = NULL;
xbt_dynar_t STag_surfxml_process_cb_list = NULL;
xbt_dynar_t ETag_surfxml_process_cb_list = NULL;
xbt_dynar_t STag_surfxml_argument_cb_list = NULL;
xbt_dynar_t ETag_surfxml_argument_cb_list = NULL;
xbt_dynar_t STag_surfxml_prop_cb_list = NULL;
xbt_dynar_t ETag_surfxml_prop_cb_list = NULL;
xbt_dynar_t STag_surfxml_set_cb_list = NULL;
xbt_dynar_t ETag_surfxml_set_cb_list = NULL;
xbt_dynar_t STag_surfxml_foreach_cb_list = NULL;
xbt_dynar_t ETag_surfxml_foreach_cb_list = NULL;
xbt_dynar_t STag_surfxml_route_c_multi_cb_list = NULL;
xbt_dynar_t ETag_surfxml_route_c_multi_cb_list = NULL;
xbt_dynar_t STag_surfxml_cluster_cb_list = NULL;
xbt_dynar_t ETag_surfxml_cluster_cb_list = NULL;
xbt_dynar_t STag_surfxml_trace_cb_list = NULL;
xbt_dynar_t ETag_surfxml_trace_cb_list = NULL;
xbt_dynar_t STag_surfxml_trace_c_connect_cb_list = NULL;
xbt_dynar_t ETag_surfxml_trace_c_connect_cb_list = NULL;
xbt_dynar_t STag_surfxml_random_cb_list = NULL;
xbt_dynar_t ETag_surfxml_random_cb_list = NULL;

/* Stores the sets defined in the XML */
xbt_dict_t set_list = NULL;

xbt_dict_t current_property_set = NULL;

/* For the route:multi tag */
xbt_dict_t route_table = NULL;
xbt_dict_t route_multi_table = NULL;
xbt_dynar_t route_multi_elements = NULL;
xbt_dynar_t route_link_list = NULL;
xbt_dynar_t links = NULL;
xbt_dynar_t keys = NULL;

xbt_dict_t random_data_list = NULL;

static xbt_dynar_t surf_input_buffer_stack = NULL;
static xbt_dynar_t surf_file_to_parse_stack = NULL;

static XBT_INLINE void surfxml_call_cb_functions(xbt_dynar_t);

YY_BUFFER_STATE surf_input_buffer;
FILE *surf_file_to_parse = NULL;

static void convert_route_multi_to_routes(void);

void surf_parse_free_callbacks(void)
{
  xbt_dynar_free(&STag_surfxml_platform_cb_list);
  xbt_dynar_free(&ETag_surfxml_platform_cb_list);
  xbt_dynar_free(&STag_surfxml_host_cb_list);
  xbt_dynar_free(&ETag_surfxml_host_cb_list);
  xbt_dynar_free(&STag_surfxml_router_cb_list);
  xbt_dynar_free(&ETag_surfxml_router_cb_list);
  xbt_dynar_free(&STag_surfxml_link_cb_list);
  xbt_dynar_free(&ETag_surfxml_link_cb_list);
  xbt_dynar_free(&STag_surfxml_route_cb_list);
  xbt_dynar_free(&ETag_surfxml_route_cb_list);
  xbt_dynar_free(&STag_surfxml_link_c_ctn_cb_list);
  xbt_dynar_free(&ETag_surfxml_link_c_ctn_cb_list);
  xbt_dynar_free(&STag_surfxml_process_cb_list);
  xbt_dynar_free(&ETag_surfxml_process_cb_list);
  xbt_dynar_free(&STag_surfxml_argument_cb_list);
  xbt_dynar_free(&ETag_surfxml_argument_cb_list);
  xbt_dynar_free(&STag_surfxml_prop_cb_list);
  xbt_dynar_free(&ETag_surfxml_prop_cb_list);
  xbt_dynar_free(&STag_surfxml_set_cb_list);
  xbt_dynar_free(&ETag_surfxml_set_cb_list);
  xbt_dynar_free(&STag_surfxml_foreach_cb_list);
  xbt_dynar_free(&ETag_surfxml_foreach_cb_list);
  xbt_dynar_free(&STag_surfxml_route_c_multi_cb_list);
  xbt_dynar_free(&ETag_surfxml_route_c_multi_cb_list);
  xbt_dynar_free(&STag_surfxml_cluster_cb_list);
  xbt_dynar_free(&ETag_surfxml_cluster_cb_list);
  xbt_dynar_free(&STag_surfxml_trace_cb_list);
  xbt_dynar_free(&ETag_surfxml_trace_cb_list);
  xbt_dynar_free(&STag_surfxml_trace_c_connect_cb_list);
  xbt_dynar_free(&ETag_surfxml_trace_c_connect_cb_list);
  xbt_dynar_free(&STag_surfxml_random_cb_list);
  xbt_dynar_free(&ETag_surfxml_random_cb_list);
}

void surf_parse_reset_parser(void)
{
  surf_parse_free_callbacks();
  STag_surfxml_platform_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_platform_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_router_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_router_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_route_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_route_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_link_c_ctn_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_link_c_ctn_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_process_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_process_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_argument_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_argument_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_prop_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_prop_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_set_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_set_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_foreach_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_foreach_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_route_c_multi_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_route_c_multi_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_cluster_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_cluster_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_trace_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_trace_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_trace_c_connect_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_trace_c_connect_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_random_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_random_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
}

void STag_surfxml_include(void)
{
  xbt_dynar_push(surf_input_buffer_stack, &surf_input_buffer);
  xbt_dynar_push(surf_file_to_parse_stack, &surf_file_to_parse);

  surf_file_to_parse = surf_fopen(A_surfxml_include_file, "r");
  xbt_assert1((surf_file_to_parse), "Unable to open \"%s\"\n",
	      A_surfxml_include_file);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, 10);
  surf_parse__switch_to_buffer(surf_input_buffer);
  printf("STAG\n");
  fflush(NULL);
}

void ETag_surfxml_include(void)
{
  printf("ETAG\n");
  fflush(NULL);
  surf_parse__delete_buffer(surf_input_buffer);
  fclose(surf_file_to_parse);
  xbt_dynar_pop(surf_file_to_parse_stack, &surf_file_to_parse);
  xbt_dynar_pop(surf_input_buffer_stack, &surf_input_buffer);
}

void STag_surfxml_platform(void)
{
  double version = 0.0;

  sscanf(A_surfxml_platform_version, "%lg", &version);

  xbt_assert0((version >= 1.0), "******* BIG FAT WARNING *********\n "
	      "You're using an ancient XML file. "
	      "Since SimGrid 3.1, units are Bytes, Flops, and seconds "
	      "instead of MBytes, MFlops and seconds. "
	      "A script (surfxml_update.pl) to help you convert your old "
	      "platform files "
	      "is available in the contrib/platform_generation directory "
	      "of the simgrid repository. Please check also out the "
	      "SURF section of the ChangeLog for the 3.1 version. "
	      "Last, do not forget to also update your values for "
	      "the calls to MSG_task_create (if any).");
  xbt_assert0((version >= 2.0), "******* BIG FAT WARNING *********\n "
	      "You're using an old XML file. "
	      "A script (surfxml_update.pl) to help you convert your old "
	      "platform files "
	      "is available in the contrib/platform_generation directory "
	      "of the simgrid repository.");

  if (set_list == NULL) set_list = xbt_dict_new(); 

  surfxml_call_cb_functions(STag_surfxml_platform_cb_list);

}

void ETag_surfxml_platform(void)
{
  convert_route_multi_to_routes();

  surfxml_call_cb_functions(ETag_surfxml_platform_cb_list);

  xbt_dict_free(&random_data_list);
  xbt_dict_free(&set_list);

}

void STag_surfxml_host(void)
{
  surfxml_call_cb_functions(STag_surfxml_host_cb_list);
}

void ETag_surfxml_host(void)
{
  surfxml_call_cb_functions(ETag_surfxml_host_cb_list);
}

void STag_surfxml_router(void)
{
  surfxml_call_cb_functions(STag_surfxml_router_cb_list);
}

void ETag_surfxml_router(void)
{
  surfxml_call_cb_functions(ETag_surfxml_router_cb_list);
}

void STag_surfxml_link(void)
{
  surfxml_call_cb_functions(STag_surfxml_link_cb_list);
}

void ETag_surfxml_link(void)
{
  surfxml_call_cb_functions(ETag_surfxml_link_cb_list);
}

void STag_surfxml_route(void)
{
  surfxml_call_cb_functions(STag_surfxml_route_cb_list);
}

void ETag_surfxml_route(void)
{
  surfxml_call_cb_functions(ETag_surfxml_route_cb_list);
}

void STag_surfxml_link_c_ctn(void)
{
  surfxml_call_cb_functions(STag_surfxml_link_c_ctn_cb_list);
}

void ETag_surfxml_link_c_ctn(void)
{
  surfxml_call_cb_functions(ETag_surfxml_link_c_ctn_cb_list);
}

void STag_surfxml_process(void)
{
  surfxml_call_cb_functions(STag_surfxml_process_cb_list);
}

void ETag_surfxml_process(void)
{
  surfxml_call_cb_functions(ETag_surfxml_process_cb_list);
}

void STag_surfxml_argument(void)
{
  surfxml_call_cb_functions(STag_surfxml_argument_cb_list);
}

void ETag_surfxml_argument(void)
{
  surfxml_call_cb_functions(ETag_surfxml_argument_cb_list);
}

void STag_surfxml_prop(void)
{
  surfxml_call_cb_functions(STag_surfxml_prop_cb_list);
}
void ETag_surfxml_prop(void)
{
  surfxml_call_cb_functions(ETag_surfxml_prop_cb_list);
}

void STag_surfxml_set(void)
{
  surfxml_call_cb_functions(STag_surfxml_set_cb_list);
}

void ETag_surfxml_set(void)
{
  surfxml_call_cb_functions(ETag_surfxml_set_cb_list);
}

void STag_surfxml_foreach(void)
{
  /* Save the current buffer */
  old_buff = surfxml_bufferstack;
  surfxml_call_cb_functions(STag_surfxml_foreach_cb_list);
}

void ETag_surfxml_foreach(void)
{ 
  surfxml_call_cb_functions(ETag_surfxml_foreach_cb_list);

  /* free the temporary dynar and restore original */
  xbt_dynar_free(&STag_surfxml_host_cb_list);
  xbt_dynar_free(&ETag_surfxml_host_cb_list);

  STag_surfxml_host_cb_list = main_STag_surfxml_host_cb_list;
  ETag_surfxml_host_cb_list = main_ETag_surfxml_host_cb_list;

  /* free the temporary dynar and restore original */
  xbt_dynar_free(&STag_surfxml_link_cb_list);
  xbt_dynar_free(&ETag_surfxml_link_cb_list);

  STag_surfxml_link_cb_list = main_STag_surfxml_link_cb_list;
  ETag_surfxml_link_cb_list = main_ETag_surfxml_link_cb_list;

}

void STag_surfxml_route_c_multi(void)
{
  surfxml_call_cb_functions(STag_surfxml_route_c_multi_cb_list);
}

void ETag_surfxml_route_c_multi(void)
{
  surfxml_call_cb_functions(ETag_surfxml_route_c_multi_cb_list);
}

void STag_surfxml_cluster(void)
{
  surfxml_call_cb_functions(STag_surfxml_cluster_cb_list);
}

void ETag_surfxml_cluster(void)
{
  surfxml_call_cb_functions(ETag_surfxml_cluster_cb_list);
}

void STag_surfxml_trace(void)
{
  surfxml_call_cb_functions(STag_surfxml_trace_cb_list);
}

void ETag_surfxml_trace(void)
{
  surfxml_call_cb_functions(ETag_surfxml_trace_cb_list);
}

void STag_surfxml_trace_c_connect(void)
{
  surfxml_call_cb_functions(STag_surfxml_trace_c_connect_cb_list);
}

void ETag_surfxml_trace_c_connect(void)
{
  surfxml_call_cb_functions(ETag_surfxml_trace_c_connect_cb_list);
}

void STag_surfxml_random(void)
{
  surfxml_call_cb_functions(STag_surfxml_random_cb_list);
}

void ETag_surfxml_random(void)
{
  surfxml_call_cb_functions(ETag_surfxml_random_cb_list);
}

void surf_parse_open(const char *file)
{
  static int warned = 0;	/* warn only once */
  if (!file) {
    if (!warned) {
      WARN0
	  ("Bypassing the XML parser since surf_parse_open received a NULL pointer. If it is not what you want, go fix your code.");
      warned = 1;
    }
    return;
  }
  if (!surf_input_buffer_stack)
    surf_input_buffer_stack = xbt_dynar_new(sizeof(YY_BUFFER_STATE), NULL);
  if (!surf_file_to_parse_stack)
    surf_file_to_parse_stack = xbt_dynar_new(sizeof(FILE *), NULL);

  surf_file_to_parse = surf_fopen(file, "r");
  xbt_assert1((surf_file_to_parse), "Unable to open \"%s\"\n", file);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, 10);
  surf_parse__switch_to_buffer(surf_input_buffer);
  surf_parse_lineno = 1;
}

void surf_parse_close(void)
{
  if (surf_input_buffer_stack)
    xbt_dynar_free(&surf_input_buffer_stack);
  if (surf_file_to_parse_stack)
    xbt_dynar_free(&surf_file_to_parse_stack);

  if (surf_file_to_parse) {
    surf_parse__delete_buffer(surf_input_buffer);
    fclose(surf_file_to_parse);
  }
}


static int _surf_parse(void)
{
  return surf_parse_lex();
}

int_f_void_t surf_parse = _surf_parse;

void surf_parse_get_double(double *value, const char *string)
{
  int ret = 0;

  ret = sscanf(string, "%lg", value);
  xbt_assert2((ret == 1), "Parse error line %d : %s not a number",
	      surf_parse_lineno, string);
}

void surf_parse_get_int(int *value, const char *string)
{
  int ret = 0;

  ret = sscanf(string, "%d", value);
  xbt_assert2((ret == 1), "Parse error line %d : %s not a number",
	      surf_parse_lineno, string);
}

void surf_parse_get_trace(tmgr_trace_t * trace, const char *string)
{
  if ((!string) || (strcmp(string, "") == 0))
    *trace = NULL;
  else
    *trace = tmgr_trace_new(string);
}

void parse_properties(void)
{
  char *value = NULL;

  if(!current_property_set) current_property_set = xbt_dict_new();

   value = xbt_strdup(A_surfxml_prop_value);  
   xbt_dict_set(current_property_set, A_surfxml_prop_id, value, free);
}

void free_string(void *d)
{
  free(*(void**)d);
}

void surfxml_add_callback(xbt_dynar_t cb_list, void_f_void_t function)
{
  xbt_dynar_push(cb_list, &function);
}

static XBT_INLINE void surfxml_call_cb_functions(xbt_dynar_t cb_list)
{
  unsigned int iterator;
  void_f_void_t fun;
  xbt_dynar_foreach(cb_list, iterator, fun){
       DEBUG2("call %p %p",fun,*fun);
       (*fun)();
    }
}

void init_data(void)
{
  xbt_dict_free(&route_table);
  xbt_dynar_free(&route_link_list);
  route_table = xbt_dict_new();

  route_multi_table = xbt_dict_new();
  route_multi_elements = xbt_dynar_new(sizeof(char*), NULL);
  traces_set_list = xbt_dict_new();
  traces_connect_list = xbt_dynar_new(sizeof(char*), NULL);
  random_data_list = xbt_dict_new();

}

void parse_platform_file(const char* file)
{
  surf_parse_open(file);
  xbt_assert1((!(*surf_parse)()), "Parse error in %s", file);
  surf_parse_close();
}

/* Functions to bypass route, host and link tags. Used by the foreach and route:multi tags */

static void parse_make_temporary_route(const char *src, const char *dst, int action)
{
  int AX_ptr = 0;
  surfxml_bufferstack = xbt_new0(char, 2048);
  
  A_surfxml_route_action = action;
  SURFXML_BUFFER_SET(route_src,                     src);
  SURFXML_BUFFER_SET(route_dst,                     dst);
}

static void parse_change_cpu_data(const char* hostName, const char* surfxml_host_power, const char* surfxml_host_availability,
					const char* surfxml_host_availability_file, const char* surfxml_host_state_file)
{
  int AX_ptr = 0;
  surfxml_bufferstack = xbt_new0(char, 2048);
 
  SURFXML_BUFFER_SET(host_id,                     hostName);
  SURFXML_BUFFER_SET(host_power,                  surfxml_host_power /*hostPower*/);
  SURFXML_BUFFER_SET(host_availability,           surfxml_host_availability);
  SURFXML_BUFFER_SET(host_availability_file,      surfxml_host_availability_file);
  SURFXML_BUFFER_SET(host_state_file,             surfxml_host_state_file);
}

static void parse_change_link_data(const char* linkName, const char* surfxml_link_bandwidth, const char* surfxml_link_bandwidth_file,
					const char* surfxml_link_latency, const char* surfxml_link_latency_file, const char* surfxml_link_state_file)
{
  int AX_ptr = 0;
  surfxml_bufferstack = xbt_new0(char, 2048);
 
  SURFXML_BUFFER_SET(link_id,                linkName);
  SURFXML_BUFFER_SET(link_bandwidth,         surfxml_link_bandwidth);
  SURFXML_BUFFER_SET(link_bandwidth_file,    surfxml_link_bandwidth_file);
  SURFXML_BUFFER_SET(link_latency,           surfxml_link_latency);
  SURFXML_BUFFER_SET(link_latency_file,      surfxml_link_latency_file);
  SURFXML_BUFFER_SET(link_state_file,        surfxml_link_state_file);
}

/**
* \brief Restores the original surfxml buffer
*/
static void parse_restore_original_buffer(void)
{
  free(surfxml_bufferstack);
  surfxml_bufferstack = old_buff;
}

/* Functions for the sets and foreach tags */

void parse_sets(void)
{
  char *id, *suffix, *prefix, *radical;
  int start, end;
  xbt_dynar_t radical_ends;
  xbt_dynar_t current_set;
  char *value;
  int i;

  id = xbt_strdup(A_surfxml_set_id);
  prefix = xbt_strdup(A_surfxml_set_prefix);
  suffix = xbt_strdup(A_surfxml_set_suffix);
  radical = xbt_strdup(A_surfxml_set_radical);
  
  xbt_assert1(!xbt_dict_get_or_null(set_list, id),
	      "Set '%s' declared several times in the platform file.",id);  
  radical_ends = xbt_str_split(radical, "-");
  xbt_assert1((xbt_dynar_length(radical_ends)==2), "Radical must be in the form lvalue-rvalue! Provided value: %s", radical);

  surf_parse_get_int(&start, xbt_dynar_get_as(radical_ends, 0, char*));
  surf_parse_get_int(&end, xbt_dynar_get_as(radical_ends, 1, char*));

  current_set = xbt_dynar_new(sizeof(char*), NULL);

  
  for (i=start; i<end; i++) {
     value = bprintf("%s%d%s", prefix, i, suffix);
     xbt_dynar_push(current_set, &value);
  } 
  
  xbt_dict_set(set_list, id, current_set, NULL);
}

static const char* surfxml_host_power;
static const char* surfxml_host_availability;
static const char* surfxml_host_availability_file;
static const char* surfxml_host_state_file;

static void parse_host_foreach(void)
{
  surfxml_host_power = A_surfxml_host_power;
  surfxml_host_availability = A_surfxml_host_availability;
  surfxml_host_availability_file = A_surfxml_host_availability_file;
  surfxml_host_state_file = A_surfxml_host_state_file;
}

static void finalize_host_foreach(void)
{
  xbt_dynar_t names = NULL;
  unsigned int cpt = 0;
  char *name;
  xbt_dict_cursor_t cursor = NULL;
  char *key,*data; 

  xbt_dict_t cluster_host_props = current_property_set;
  
  xbt_assert1((names = xbt_dict_get_or_null(set_list, foreach_set_name)),
	      "Set name '%s' reffered by foreach tag not found.", foreach_set_name);  

  xbt_assert1((strcmp(A_surfxml_host_id, "$1") == 0), "The id of the host within the foreach should point to the foreach set_id (use $1). Your value: %s", A_surfxml_host_id);

	
  /* foreach name in set call the main host callback */
  xbt_dynar_foreach (names, cpt, name) {
    parse_change_cpu_data(name, surfxml_host_power, surfxml_host_availability,
					surfxml_host_availability_file, surfxml_host_state_file);
    surfxml_call_cb_functions(main_STag_surfxml_host_cb_list);

    xbt_dict_foreach(cluster_host_props,cursor,key,data) {
           xbt_dict_set(current_property_set, xbt_strdup(key), xbt_strdup(data), free);
    }

    surfxml_call_cb_functions(main_ETag_surfxml_host_cb_list);
    free(surfxml_bufferstack);
  }

  current_property_set = xbt_dict_new();

  surfxml_bufferstack = old_buff;
  
}

static const char* surfxml_link_bandwidth;
static const char* surfxml_link_bandwidth_file;
static const char* surfxml_link_latency;
static const char* surfxml_link_latency_file;
static const char* surfxml_link_state_file;

static void parse_link_foreach(void)
{
  surfxml_link_bandwidth = A_surfxml_link_bandwidth;
  surfxml_link_bandwidth_file = A_surfxml_link_bandwidth_file;
  surfxml_link_latency = A_surfxml_link_latency;
  surfxml_link_latency_file = A_surfxml_link_latency_file;
  surfxml_link_state_file = A_surfxml_link_state_file;
}

static void finalize_link_foreach(void)
{
  xbt_dynar_t names = NULL;
  unsigned int cpt = 0;
  char *name;
  xbt_dict_cursor_t cursor = NULL;
  char *key,*data; 

  xbt_dict_t cluster_link_props = current_property_set;

  xbt_assert1((names = xbt_dict_get_or_null(set_list, foreach_set_name)),
	      "Set name '%s' reffered by foreach tag not found.", foreach_set_name); 

  xbt_assert1((strcmp(A_surfxml_link_id, "$1") == 0), "The id of the link within the foreach should point to the foreach set_id (use $1). Your value: %s", A_surfxml_link_id);


  /* for each name in set call the main link callback */
  xbt_dynar_foreach (names, cpt, name) {
    parse_change_link_data(name, surfxml_link_bandwidth, surfxml_link_bandwidth_file,
					surfxml_link_latency, surfxml_link_latency_file, surfxml_link_state_file);
    surfxml_call_cb_functions(main_STag_surfxml_link_cb_list);

    xbt_dict_foreach(cluster_link_props,cursor,key,data) {
           xbt_dict_set(current_property_set, xbt_strdup(key), xbt_strdup(data), free);
    }

    surfxml_call_cb_functions(main_ETag_surfxml_link_cb_list);
   free(surfxml_bufferstack);

  }

  current_property_set = xbt_dict_new();

  surfxml_bufferstack = old_buff;
}

void parse_foreach(void)
{
  /* save the host & link callbacks */
  main_STag_surfxml_host_cb_list = STag_surfxml_host_cb_list;
  main_ETag_surfxml_host_cb_list = ETag_surfxml_host_cb_list;
  main_STag_surfxml_link_cb_list = STag_surfxml_link_cb_list;
  main_ETag_surfxml_link_cb_list = ETag_surfxml_link_cb_list;

  /* define host & link callbacks to be used only by the foreach tag */
  STag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  STag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
  ETag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);

  surfxml_add_callback(STag_surfxml_host_cb_list, &parse_host_foreach);
  surfxml_add_callback(ETag_surfxml_host_cb_list, &finalize_host_foreach);
  surfxml_add_callback(STag_surfxml_link_cb_list, &parse_link_foreach);
  surfxml_add_callback(ETag_surfxml_link_cb_list, &finalize_link_foreach);

  /* get set name */
  foreach_set_name = xbt_strdup(A_surfxml_foreach_set_id); 
}

/* Route:multi functions */

static int route_multi_size=0;
static char* src_name, *dst_name;
static int is_symmetric_route;

void parse_route_elem(void)
{
  char *val;

  val = xbt_strdup(A_surfxml_link_c_ctn_id);
  
  xbt_dynar_push(route_link_list, &val);
}

void parse_route_multi_set_endpoints(void)
{
  src_name = xbt_strdup(A_surfxml_route_c_multi_src); 
  dst_name = xbt_strdup(A_surfxml_route_c_multi_dst); 
  route_action = A_surfxml_route_c_multi_action;
  is_symmetric_route = A_surfxml_route_c_multi_symmetric;
  route_multi_size++;

  route_link_list = xbt_dynar_new(sizeof(char *), &free_string);
}

static int contains(xbt_dynar_t list, const char* value)
{
 unsigned int cpt;
 char * val;
 xbt_dynar_foreach(list, cpt, val) {
   if (strcmp(val, value) == 0)
      return 1;
  }
  return 0;
}

/* 
   This function is used to append or override the contents of an alread existing route in the case a new one with its name is found.
   The decision is based upon the value of action specified in the xml route:multi attribute action
 */
void manage_route(xbt_dict_t routing_table, const char *route_name, int action, int isMultiRoute)
{
  unsigned int cpt;
  xbt_dynar_t links;
  char *value;

  /* get already existing list if it exists */
  links = xbt_dict_get_or_null(routing_table, route_name);
  DEBUG1("ROUTE: %s", route_name);
  if (links != NULL) {
     switch (action) {
        case A_surfxml_route_action_PREPEND: /* add existing links at the end; route_link_list + links */
				    xbt_dynar_foreach(links, cpt, value) {	
                                       xbt_dynar_push(route_link_list,&value);
				    }
                                    break;
        case A_surfxml_route_action_POSTPEND: /* add existing links in front; links + route_link_list */ 
				    xbt_dynar_foreach(route_link_list, cpt, value) {
                                       xbt_dynar_push(links,&value);
				    }
                                    route_link_list = links;
				    break;
        case A_surfxml_route_action_OVERRIDE:
                                    break;
        default:break;
     }
  }
  /* this is the final route; do not add if name is a set; add only if name is in set list */
  if (!isMultiRoute){    
    xbt_dict_set(routing_table, route_name, route_link_list, NULL);
  }
}

void parse_route_multi_set_route(void)
{
  char* route_name;

  route_name = bprintf("%s#%s#%d#%d#%d", src_name, dst_name, route_action, is_symmetric_route, route_multi_size);

  xbt_dynar_push(route_multi_elements, &route_name);

  /* Add route */
  xbt_dict_set(route_multi_table, route_name, route_link_list, NULL);
  /* add symmetric if it is the case */ 
  if (is_symmetric_route == 1) {
    char * symmetric_name = bprintf("%s#%s#%d#%d#%d", dst_name, src_name, route_action, !is_symmetric_route, route_multi_size);
  
    xbt_dict_set(route_multi_table, symmetric_name, route_link_list, NULL);
    xbt_dynar_push(route_multi_elements, &symmetric_name);
    is_symmetric_route = 0;
  }
  free(src_name);
  free(dst_name);
}

static void add_multi_links(const char* src, const char* dst, xbt_dynar_t links, const char* src_name, const char* dst_name)
{
  unsigned int cpt;
  char* value, *val;
   parse_make_temporary_route(src_name, dst_name, route_action);
   surfxml_call_cb_functions(STag_surfxml_route_cb_list);
   DEBUG2("\tADDING ROUTE: %s -> %s", src_name, dst_name);
   /* Build link list */ 
   xbt_dynar_foreach(links, cpt, value) {
     if (strcmp(value, src) == 0)
       val =  xbt_strdup(src_name);
     else if (strcmp(value, dst) == 0)
       val = xbt_strdup(dst_name);
     else if (strcmp(value, "$dst") == 0)
       val = xbt_strdup(dst_name);
     else if (strcmp(value, "$src") == 0)
       val = xbt_strdup(src_name);
     else
       val = xbt_strdup(value);
     DEBUG1("\t\tELEMENT: %s", val);
     xbt_dynar_push(route_link_list, &val);
   }    
   surfxml_call_cb_functions(ETag_surfxml_route_cb_list);
   free(surfxml_bufferstack);
}

static void convert_route_multi_to_routes(void)
{
  xbt_dict_cursor_t cursor_w;
  int symmetric;
  unsigned int cpt, cpt2, cursor;
  char *src_host_name, *dst_host_name, *key, *src, *dst, *val, *key_w, *data_w; 
  const char* sep="#";
  xbt_dynar_t src_names = NULL, dst_names = NULL, links;

  if (!route_multi_elements) return;

  xbt_dict_t set = cpu_set;
  DEBUG1("%d", xbt_dict_length(workstation_set));				
  if (workstation_set != NULL && xbt_dict_length(workstation_set) > 0)
     set = workstation_set;
  

  old_buff = surfxml_bufferstack;
  /* Get all routes in the exact order they were entered in the platform file */
  xbt_dynar_foreach(route_multi_elements, cursor, key) {
     /* Get links for the route */     
     links = (xbt_dynar_t)xbt_dict_get_or_null(route_multi_table, key);
     keys = xbt_str_split_str(key, sep);
     /* Get route ends */
     src = xbt_dynar_get_as(keys, 0, char*);
     dst = xbt_dynar_get_as(keys, 1, char*);
     route_action = atoi(xbt_dynar_get_as(keys, 2, char*));
     symmetric = atoi(xbt_dynar_get_as(keys, 3, char*));

    /* Create the dynar of src and dst hosts for the new routes */ 
    /* NOTE: src and dst can be either set names or simple host names */
    src_names = (xbt_dynar_t)xbt_dict_get_or_null(set_list, src);
    dst_names = (xbt_dynar_t)xbt_dict_get_or_null(set_list, dst);
    /* Add to dynar even if they are simple names */
    if (src_names == NULL) {
       src_names = xbt_dynar_new(sizeof(char *), &free_string);
       val = xbt_strdup(src);
       xbt_dynar_push(src_names, &val);
       if (strcmp(val,"$*") != 0 && NULL == xbt_dict_get_or_null(set, val))
         THROW3(unknown_error,0,"(In route:multi (%s -> %s) source %s does not exist (not a set or a host)", src, dst, src);
    }
    if (dst_names == NULL) {
       dst_names = xbt_dynar_new(sizeof(char *), &free_string);
       val = xbt_strdup(dst);
       if (strcmp(val,"$*") != 0 && NULL == xbt_dict_get_or_null(set, val))
         THROW3(unknown_error,0,"(In route:multi (%s -> %s) destination %s does not exist (not a set or a host)", src, dst, dst);
       xbt_dynar_push(dst_names, &val);
    }

    /* Build the routes */
    DEBUG2("ADDING MULTI ROUTE: %s -> %s", xbt_dynar_get_as(keys, 0, char*), xbt_dynar_get_as(keys, 1, char*));
    xbt_dynar_foreach(src_names, cpt, src_host_name) {
      xbt_dynar_foreach(dst_names, cpt2, dst_host_name) {
        /* If dst is $* then set this route to have its dst point to all hosts */
        if (strcmp(src_host_name,"$*") != 0 && strcmp(dst_host_name,"$*") == 0){
		  xbt_dict_foreach(set, cursor_w, key_w, data_w) {
                          //int n = xbt_dynar_member(src_names, (char*)key_w);
       			    add_multi_links(src, dst, links, src_host_name, key_w);               
		  }
        }
        /* If src is $* then set this route to have its dst point to all hosts */
        if (strcmp(src_host_name,"$*") == 0 && strcmp(dst_host_name,"$*") != 0){
	   	  xbt_dict_foreach(set, cursor_w, key_w, data_w) {
                     // if (!symmetric || (symmetric && !contains(dst_names, key_w)))
      			add_multi_links(src, dst, links, key_w, dst_host_name);               
                }
        }
        /* if none of them are equal to $* */
        if (strcmp(src_host_name,"$*") != 0 && strcmp(dst_host_name,"$*") != 0) {
   			add_multi_links(src, dst, links, src_host_name, dst_host_name);               
	}     
      }
    }
  }  
  surfxml_bufferstack = old_buff;
  xbt_dict_free(&route_multi_table);
  xbt_dynar_free(&route_multi_elements);
}

/* Cluster tag functions */

void parse_cluster(void)
{  
   static int AX_ptr = 0;
   static int surfxml_bufferstack_size = 2048;
 
   char* cluster_id = A_surfxml_cluster_id;
   char* cluster_prefix = A_surfxml_cluster_prefix;
   char* cluster_suffix = A_surfxml_cluster_suffix;
   char* cluster_radical = A_surfxml_cluster_radical;
   char* cluster_power = A_surfxml_cluster_power;
   char* cluster_bw = A_surfxml_cluster_bw;
   char* cluster_lat = A_surfxml_cluster_lat;
   char* cluster_bb_bw = A_surfxml_cluster_bb_bw;
   char* cluster_bb_lat = A_surfxml_cluster_bb_lat;
 
   char* saved_buff = surfxml_bufferstack;

   char * backbone_name;

   surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);

   /* Make set */
   SURFXML_BUFFER_SET(set_id, cluster_id);
   SURFXML_BUFFER_SET(set_prefix, cluster_prefix);
   SURFXML_BUFFER_SET(set_suffix, cluster_suffix);
   SURFXML_BUFFER_SET(set_radical, cluster_radical);
  
   SURFXML_START_TAG(set);
   SURFXML_END_TAG(set);

   /* Make foreach */
   SURFXML_BUFFER_SET(foreach_set_id, cluster_id);

   SURFXML_START_TAG(foreach);

     /* Make host for the foreach */
     parse_change_cpu_data("$1", cluster_power, "1.0", "", "");
     A_surfxml_host_state = A_surfxml_host_state_ON;

     SURFXML_START_TAG(host);
     SURFXML_END_TAG(host);

     /* Make link for the foreach */
     parse_change_link_data("$1", cluster_bw, "", cluster_lat, "", "");
     A_surfxml_link_state = A_surfxml_link_state_ON;
     A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
 
     SURFXML_START_TAG(link);
     SURFXML_END_TAG(link);

   SURFXML_END_TAG(foreach);

   /* Make backbone link */
   backbone_name = bprintf("%s_bb", cluster_id);
   parse_change_link_data(backbone_name, cluster_bb_bw, "", cluster_bb_lat, "", "");
   A_surfxml_link_state = A_surfxml_link_state_ON;
   A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_FATPIPE;
 
   SURFXML_START_TAG(link);
   SURFXML_END_TAG(link);

   /* Make route multi with the outside world, i.e. cluster->$* */
   SURFXML_BUFFER_SET(route_c_multi_src, cluster_id);
   SURFXML_BUFFER_SET(route_c_multi_dst, "$*");
   A_surfxml_route_c_multi_symmetric = A_surfxml_route_c_multi_symmetric_NO;
   A_surfxml_route_c_multi_action = A_surfxml_route_c_multi_action_OVERRIDE;
   
   SURFXML_START_TAG(route_c_multi);

     SURFXML_BUFFER_SET(link_c_ctn_id, "$src");
    
     SURFXML_START_TAG(link_c_ctn);
     SURFXML_END_TAG(link_c_ctn);

   SURFXML_END_TAG(route_c_multi);

   /* Make route multi between cluster hosts, i.e. cluster->cluster */
   SURFXML_BUFFER_SET(route_c_multi_src, cluster_id);
   SURFXML_BUFFER_SET(route_c_multi_dst, cluster_id);
   A_surfxml_route_c_multi_action = A_surfxml_route_c_multi_action_POSTPEND;
   A_surfxml_route_c_multi_symmetric = A_surfxml_route_c_multi_symmetric_NO;
   
   SURFXML_START_TAG(route_c_multi);
    
     SURFXML_BUFFER_SET(link_c_ctn_id, backbone_name);
    
     SURFXML_START_TAG(link_c_ctn);
     SURFXML_END_TAG(link_c_ctn);
    
   SURFXML_END_TAG(route_c_multi);


   /* Restore buff */
   free(surfxml_bufferstack);
   surfxml_bufferstack = saved_buff;
}

/* Trace management functions */

static double trace_periodicity = -1.0;
static char* trace_file = NULL;
static char* trace_id;

void parse_trace_init(void)
{
   trace_id = strdup(A_surfxml_trace_id);
   trace_file = strdup(A_surfxml_trace_file);
   surf_parse_get_double(&trace_periodicity, A_surfxml_trace_periodicity);
}

void parse_trace_finalize(void)
{
  tmgr_trace_t trace;
  if (!trace_file || strcmp(trace_file,"") != 0) {
    surf_parse_get_trace(&trace, trace_file);
  }
  else {
    if (strcmp(surfxml_pcdata, "") == 0) trace = NULL;
    else
      trace = tmgr_trace_new_from_string(trace_id, surfxml_pcdata, trace_periodicity);  
  }
  xbt_dict_set(traces_set_list, trace_id, (void *)trace, NULL);
}

void parse_trace_c_connect(void)
{
	char* trace_connect;
   xbt_assert1(xbt_dict_get_or_null(traces_set_list, A_surfxml_trace_c_connect_trace_id),
	      "Trace %s undefined", A_surfxml_trace_c_connect_trace_id);
   trace_connect = bprintf("%s#%d#%d#%s", A_surfxml_trace_c_connect_trace_id, A_surfxml_trace_c_connect_element, 
                                   A_surfxml_trace_c_connect_kind, A_surfxml_trace_c_connect_connector_id);
   xbt_dynar_push(traces_connect_list, &trace_connect);
}

/* Random tag functions */

double get_cpu_power(const char *power)
{ 
  double power_scale = 0.0;
  const char *p, *q;
  char *generator;
  random_data_t random = NULL; 
  /* randomness is inserted like this: power="$rand(my_random)" */
  if (((p = strstr(power, "$rand(")) != NULL) && ((q = strstr(power, ")")) != NULL)) {
     if (p < q) {
       generator = xbt_malloc(q - (p + 6) + 1);
       memcpy(generator, p + 6, q - (p + 6)); 
       generator[q - (p + 6)] = '\0';
       xbt_assert1((random = xbt_dict_get_or_null(random_data_list, generator)),
	      "Random generator %s undefined", generator);
       power_scale = random_generate(random);
     }
  }
  else {
    surf_parse_get_double(&power_scale, power);
  }
  return power_scale;
}

int random_min, random_max, random_mean, random_std_deviation, random_generator;
char *random_id;

void init_randomness(void)
{
  random_id = A_surfxml_random_id;
  surf_parse_get_int(&random_min, A_surfxml_random_min);
  surf_parse_get_int(&random_max, A_surfxml_random_max);
  surf_parse_get_int(&random_mean, A_surfxml_random_mean);
  surf_parse_get_int(&random_std_deviation, A_surfxml_random_std_deviation);
  random_generator = A_surfxml_random_generator;
}

void add_randomness(void)
{
   /* If needed aditional properties can be added by using the prop tag */
   random_data_t random = random_new(random_generator, random_min, random_max, random_mean, random_std_deviation);
   xbt_dict_set(random_data_list, random_id, (void *)random, NULL);
}

