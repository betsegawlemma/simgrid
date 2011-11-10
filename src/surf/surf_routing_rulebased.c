/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "surf_routing_private.h"
#include <pcre.h>               /* regular expression library */

/* Global vars */
extern routing_global_t global_routing;
extern xbt_dynar_t link_list;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_rulebased, surf, "Routing part of surf");

/* Routing model structure */

typedef struct {
  s_as_t generic_routing;
  xbt_dict_t dict_processing_units;
  xbt_dict_t dict_autonomous_systems;
  xbt_dynar_t list_route;
  xbt_dynar_t list_ASroute;
} s_routing_component_rulebased_t, *routing_component_rulebased_t;

typedef struct s_rule_route s_rule_route_t, *rule_route_t;
typedef struct s_rule_route_extended s_rule_route_extended_t,
    *rule_route_extended_t;

struct s_rule_route {
  xbt_dynar_t re_str_link;      // dynar of char*
  pcre *re_src;
  pcre *re_dst;
};

struct s_rule_route_extended {
  s_rule_route_t generic_rule_route;
  char *re_src_gateway;
  char *re_dst_gateway;
};

static void rule_route_free(void *e)
{
  rule_route_t *elem = (rule_route_t *) (e);
  if (*elem) {
    xbt_dynar_free(&(*elem)->re_str_link);
    pcre_free((*elem)->re_src);
    pcre_free((*elem)->re_dst);
    xbt_free(*elem);
  }
  *elem = NULL;
}

static void rule_route_extended_free(void *e)
{
  rule_route_extended_t *elem = (rule_route_extended_t *) e;
  if (*elem) {
    xbt_dynar_free(&(*elem)->generic_rule_route.re_str_link);
    pcre_free((*elem)->generic_rule_route.re_src);
    pcre_free((*elem)->generic_rule_route.re_dst);
    xbt_free((*elem)->re_src_gateway);
    xbt_free((*elem)->re_dst_gateway);
    xbt_free(*elem);
  }
}

/* Parse routing model functions */

static void model_rulebased_parse_PU(AS_t rc,
                                                const char *name)
{
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;
  xbt_dict_set(routing->dict_processing_units, name, (void *) (-1), NULL);
}

static void model_rulebased_parse_AS(AS_t rc,
                                                  const char *name)
{
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;
  xbt_dict_set(routing->dict_autonomous_systems, name, (void *) (-1),
               NULL);
}

static void model_rulebased_parse_route(AS_t rc,
                                      const char *src, const char *dst,
                                      route_extended_t route)
{
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;
  rule_route_t ruleroute = xbt_new0(s_rule_route_t, 1);
  const char *error;
  int erroffset;

  if(!strcmp(rc->routing->name,"Vivaldi")){
	  if(xbt_dynar_length(route->generic_route.link_list) != 0)
		  xbt_die("You can't have link_ctn with Model Vivaldi.");
  }

  ruleroute->re_src = pcre_compile(src, 0, &error, &erroffset, NULL);
  xbt_assert(ruleroute->re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, src, error);
  ruleroute->re_dst = pcre_compile(dst, 0, &error, &erroffset, NULL);
  xbt_assert(ruleroute->re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, dst, error);
  ruleroute->re_str_link = route->generic_route.link_list;
  xbt_dynar_push(routing->list_route, &ruleroute);
  xbt_free(route);
}

static void model_rulebased_parse_ASroute(AS_t rc,
                                        const char *src, const char *dst,
                                        route_extended_t route)
{
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;
  rule_route_extended_t ruleroute_e = xbt_new0(s_rule_route_extended_t, 1);
  const char *error;
  int erroffset;

  if(!strcmp(rc->routing->name,"Vivaldi")){
	  if(xbt_dynar_length(route->generic_route.link_list) != 0)
		  xbt_die("You can't have link_ctn with Model Vivaldi.");
  }

  ruleroute_e->generic_rule_route.re_src =
      pcre_compile(src, 0, &error, &erroffset, NULL);
  xbt_assert(ruleroute_e->generic_rule_route.re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, src, error);
  ruleroute_e->generic_rule_route.re_dst =
      pcre_compile(dst, 0, &error, &erroffset, NULL);
  xbt_assert(ruleroute_e->generic_rule_route.re_src,
              "PCRE compilation failed at offset %d (\"%s\"): %s\n",
              erroffset, dst, error);
  ruleroute_e->generic_rule_route.re_str_link =
      route->generic_route.link_list;
  ruleroute_e->re_src_gateway = route->src_gateway;
  ruleroute_e->re_dst_gateway = route->dst_gateway;
  xbt_dynar_push(routing->list_ASroute, &ruleroute_e);
//  xbt_free(route->src_gateway);
//  xbt_free(route->dst_gateway);
  xbt_free(route);
}

static void model_rulebased_parse_bypassroute(AS_t rc,
                                            const char *src,
                                            const char *dst,
                                            route_extended_t e_route)
{
  xbt_die("bypass routing not supported for Route-Based model");
}

#define BUFFER_SIZE 4096        /* result buffer size */
#define OVECCOUNT 30            /* should be a multiple of 3 */

static char *remplace(char *value, const char **src_list, int src_size,
                      const char **dst_list, int dst_size)
{
  char result[BUFFER_SIZE];
  int i_res = 0;
  int i = 0;

  while (value[i]) {
    if (value[i] == '$') {
      i++;                      /* skip the '$' */
      if (value[i] < '0' || value[i] > '9')
        xbt_die("bad string parameter, no number indication, at offset: "
                "%d (\"%s\")", i, value);

      /* solve the number */
      int number = value[i++] - '0';
      while (value[i] >= '0' && value[i] <= '9')
        number = 10 * number + (value[i++] - '0');

      /* solve the indication */
      const char **param_list;
      _XBT_GNUC_UNUSED int param_size;
      if (value[i] == 's' && value[i + 1] == 'r' && value[i + 2] == 'c') {
        param_list = src_list;
        param_size = src_size;
      } else if (value[i] == 'd' && value[i + 1] == 's'
                 && value[i + 2] == 't') {
        param_list = dst_list;
        param_size = dst_size;
      } else {
        xbt_die("bad string parameter, support only \"src\" and \"dst\", "
                "at offset: %d (\"%s\")", i, value);
      }
      i += 3;

      xbt_assert(number < param_size,
                 "bad string parameter, not enough length param_size, "
                 "at offset: %d (\"%s\") %d %d", i, value, param_size, number);

      const char *param = param_list[number];
      int j = 0;
      while (param[j] && i_res < BUFFER_SIZE)
        result[i_res++] = param[j++];
    } else {
      result[i_res++] = value[i++]; /* next char */
    }
    if (i_res >= BUFFER_SIZE)
      xbt_die("solving string \"%s\", small buffer size (%d)",
              value, BUFFER_SIZE);
  }
  result[i_res++] = '\0';
  char *res = xbt_malloc(i_res);
  return memcpy(res, result, i_res);
}

static route_extended_t rulebased_get_route(AS_t rc,
                                            const char *src,
                                            const char *dst);
static xbt_dynar_t rulebased_get_onelink_routes(AS_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new (sizeof(onelink_t), xbt_free);

  //We have already bypass cluster routes with network NS3
  if(!strcmp(surf_network_model->name,"network NS3"))
	return ret;

  routing_component_rulebased_t routing = (routing_component_rulebased_t)rc;

  xbt_dict_cursor_t c1 = NULL;
  char *k1, *d1;

  //find router
  char *router = NULL;
  xbt_dict_foreach(routing->dict_processing_units, c1, k1, d1) {
    if (routing_get_network_element_type(k1) == SURF_NETWORK_ELEMENT_ROUTER){
      router = k1;
    }
  }

  if (!router){
    xbt_die ("rulebased_get_onelink_routes works only if the AS is a cluster, sorry.");
  }

  xbt_dict_foreach(routing->dict_processing_units, c1, k1, d1) {
    route_extended_t route = rulebased_get_route (rc, router, k1);

    int number_of_links = xbt_dynar_length(route->generic_route.link_list);

    if(number_of_links == 1) {
		//loopback
    }
    else{
		if (number_of_links != 2) {
		  xbt_die ("rulebased_get_onelink_routes works only if the AS is a cluster, sorry.");
		}

		void *link_ptr;
		xbt_dynar_get_cpy (route->generic_route.link_list, 1, &link_ptr);
		onelink_t onelink = xbt_new0 (s_onelink_t, 1);
		onelink->src = xbt_strdup (k1);
		onelink->dst = xbt_strdup (router);
		onelink->link_ptr = link_ptr;
		xbt_dynar_push (ret, &onelink);
    }
  }
  return ret;
}

/* Business methods */
static route_extended_t rulebased_get_route(AS_t rc,
                                            const char *src,
                                            const char *dst)
{
  xbt_assert(rc && src
              && dst,
              "Invalid params for \"get_route\" function at AS \"%s\"",
              rc->name);

  /* set utils vars */
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;

  int are_processing_units=0;
  xbt_dynar_t rule_list;
  if (xbt_dict_get_or_null(routing->dict_processing_units, src)
      && xbt_dict_get_or_null(routing->dict_processing_units, dst)) {
    are_processing_units = 1;
    rule_list = routing->list_route;
  } else if (xbt_dict_get_or_null(routing->dict_autonomous_systems, src)
             && xbt_dict_get_or_null(routing->dict_autonomous_systems,
                                     dst)) {
    are_processing_units = 0;
    rule_list = routing->list_ASroute;
  } else
    xbt_die("Ask for route \"from\"(%s)  or \"to\"(%s) no found in "
            "the local table", src, dst);

  int rc_src = -1;
  int rc_dst = -1;
  int src_length = (int) strlen(src);
  int dst_length = (int) strlen(dst);

  xbt_dynar_t links_list =
      xbt_dynar_new(global_routing->size_of_link, NULL);

  rule_route_t ruleroute;
  unsigned int cpt;
  int ovector_src[OVECCOUNT];
  int ovector_dst[OVECCOUNT];
  const char **list_src = NULL;
  const char **list_dst = NULL;
  _XBT_GNUC_UNUSED int res;
  xbt_dynar_foreach(rule_list, cpt, ruleroute) {
    rc_src =
        pcre_exec(ruleroute->re_src, NULL, src, src_length, 0, 0,
                  ovector_src, OVECCOUNT);
    if (rc_src >= 0) {
      rc_dst =
          pcre_exec(ruleroute->re_dst, NULL, dst, dst_length, 0, 0,
                    ovector_dst, OVECCOUNT);
      if (rc_dst >= 0) {
        res = pcre_get_substring_list(src, ovector_src, rc_src, &list_src);
        xbt_assert(!res, "error solving substring list for src \"%s\"", src);
        res = pcre_get_substring_list(dst, ovector_dst, rc_dst, &list_dst);
        xbt_assert(!res, "error solving substring list for src \"%s\"", dst);
        char *link_name;
        xbt_dynar_foreach(ruleroute->re_str_link, cpt, link_name) {
          char *new_link_name =
              remplace(link_name, list_src, rc_src, list_dst, rc_dst);
          void *link =
        		  xbt_lib_get_or_null(link_lib, new_link_name, SURF_LINK_LEVEL);
          if (link)
            xbt_dynar_push(links_list, &link);
          else
            THROWF(mismatch_error, 0, "Link %s not found", new_link_name);
          xbt_free(new_link_name);
        }
      }
    }
    if (rc_src >= 0 && rc_dst >= 0)
      break;
  }

  route_extended_t new_e_route = NULL;
  if (rc_src >= 0 && rc_dst >= 0) {
    new_e_route = xbt_new0(s_route_extended_t, 1);
    new_e_route->generic_route.link_list = links_list;
  } else if (!strcmp(src, dst) && are_processing_units) {
    new_e_route = xbt_new0(s_route_extended_t, 1);
    xbt_dynar_push(links_list, &(global_routing->loopback));
    new_e_route->generic_route.link_list = links_list;
  } else {
    xbt_dynar_free(&link_list);
  }

  if (!are_processing_units && new_e_route) {
    rule_route_extended_t ruleroute_extended =
        (rule_route_extended_t) ruleroute;
    new_e_route->src_gateway =
        remplace(ruleroute_extended->re_src_gateway, list_src, rc_src,
                 list_dst, rc_dst);
    new_e_route->dst_gateway =
        remplace(ruleroute_extended->re_dst_gateway, list_src, rc_src,
                 list_dst, rc_dst);
  }

  if (list_src)
    pcre_free_substring_list(list_src);
  if (list_dst)
    pcre_free_substring_list(list_dst);

  return new_e_route;
}

static route_extended_t rulebased_get_bypass_route(AS_t rc,
                                                   const char *src,
                                                   const char *dst)
{
  return NULL;
}

static void rulebased_finalize(AS_t rc)
{
  routing_component_rulebased_t routing =
      (routing_component_rulebased_t) rc;
  if (routing) {
    xbt_dict_free(&routing->dict_processing_units);
    xbt_dict_free(&routing->dict_autonomous_systems);
    xbt_dynar_free(&routing->list_route);
    xbt_dynar_free(&routing->list_ASroute);
    /* Delete structure */
    xbt_free(routing);
  }
}

/* Creation routing model functions */
AS_t model_rulebased_create(void) {

  routing_component_rulebased_t new_component = (routing_component_rulebased_t)
      routmod_generic_create(sizeof(s_routing_component_rulebased_t));

  new_component->generic_routing.parse_PU = model_rulebased_parse_PU;
  new_component->generic_routing.parse_AS = model_rulebased_parse_AS;
  new_component->generic_routing.parse_route = model_rulebased_parse_route;
  new_component->generic_routing.parse_ASroute = model_rulebased_parse_ASroute;
  new_component->generic_routing.parse_bypassroute = model_rulebased_parse_bypassroute;
  new_component->generic_routing.get_onelink_routes = rulebased_get_onelink_routes;
  new_component->generic_routing.get_route = rulebased_get_route;
  new_component->generic_routing.get_bypass_route = rulebased_get_bypass_route;
  new_component->generic_routing.finalize = rulebased_finalize;
  /* initialization of internal structures */
  new_component->dict_processing_units = xbt_dict_new();
  new_component->dict_autonomous_systems = xbt_dict_new();
  new_component->list_route = xbt_dynar_new(sizeof(rule_route_t), &rule_route_free);
  new_component->list_ASroute =
      xbt_dynar_new(sizeof(rule_route_extended_t),
                    &rule_route_extended_free);

  return (AS_t) new_component;
}
