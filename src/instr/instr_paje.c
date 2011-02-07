/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje, instr, "Paje tracing event system (data structures)");

static type_t rootType = NULL;              /* the root type */
static container_t rootContainer = NULL;    /* the root container */
static xbt_dict_t allContainers = NULL;     /* all created containers indexed by name */
xbt_dict_t trivaNodeTypes = NULL;     /* all link types defined */
xbt_dict_t trivaEdgeTypes = NULL;     /* all host types defined */

void instr_paje_init (container_t root)
{
  allContainers = xbt_dict_new ();
  trivaNodeTypes = xbt_dict_new ();
  trivaEdgeTypes = xbt_dict_new ();
  rootContainer = root;
}

static val_t newValue (const char *valuename, const char *color, type_t father)
{
  val_t ret = xbt_new0(s_val_t, 1);
  ret->name = xbt_strdup (valuename);
  ret->father = father;
  ret->color = xbt_strdup (color);

  static long long int type_id = 0;
  char str_id[INSTR_DEFAULT_STR_SIZE];
  snprintf (str_id, INSTR_DEFAULT_STR_SIZE, "v%lld", type_id++);
  ret->id = xbt_strdup (str_id);

  xbt_dict_set (father->values, valuename, ret, NULL);
  DEBUG2("new value %s, child of %s", ret->name, ret->father->name);
  return ret;
}

val_t getValue (const char *valuename, const char *color, type_t father)
{
  if (father->kind == TYPE_VARIABLE) return NULL; //Variables can't have different values

  val_t ret = (val_t)xbt_dict_get_or_null (father->values, valuename);
  if (ret == NULL){
    ret = newValue (valuename, color, father);
    DEBUG4("EntityValue %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    new_pajeDefineEntityValue(ret);
  }
  return ret;
}

val_t getValueByName (const char *valuename, type_t father)
{
  return getValue (valuename, NULL, father);
}

static type_t newType (const char *typename, const char *key, const char *color, e_entity_types kind, type_t father)
{
  type_t ret = xbt_new0(s_type_t, 1);
  ret->name = xbt_strdup (typename);
  ret->father = father;
  ret->kind = kind;
  ret->children = xbt_dict_new ();
  ret->values = xbt_dict_new ();
  ret->color = xbt_strdup (color);

  static long long int type_id = 0;
  char str_id[INSTR_DEFAULT_STR_SIZE];
  snprintf (str_id, INSTR_DEFAULT_STR_SIZE, "%lld", type_id++);
  ret->id = xbt_strdup (str_id);

  if (father != NULL){
    xbt_dict_set (father->children, key, ret, NULL);
    DEBUG2("new type %s, child of %s", typename, father->name);
  }
  return ret;
}

type_t getRootType ()
{
  return rootType;
}

type_t getContainerType (const char *typename, type_t father)
{
  type_t ret;
  if (father == NULL){
    ret = newType (typename, typename, NULL, TYPE_CONTAINER, father);
    if (father) new_pajeDefineContainerType (ret);
    rootType = ret;
  }else{
    //check if my father type already has my typename
    ret = (type_t)xbt_dict_get_or_null (father->children, typename);
    if (ret == NULL){
      ret = newType (typename, typename, NULL, TYPE_CONTAINER, father);
      new_pajeDefineContainerType (ret);
    }
  }
  return ret;
}

type_t getEventType (const char *typename, const char *color, type_t father)
{
  type_t ret = xbt_dict_get_or_null (father->children, typename);
  if (ret == NULL){
    char white[INSTR_DEFAULT_STR_SIZE] = "1 1 1";
    if (!color){
      ret = newType (typename, typename, white, TYPE_EVENT, father);
    }else{
      ret = newType (typename, typename, color, TYPE_EVENT, father);
    }
    DEBUG4("EventType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    new_pajeDefineEventType(ret);
  }
  return ret;
}

type_t getVariableType (const char *typename, const char *color, type_t father)
{
  type_t ret = xbt_dict_get_or_null (father->children, typename);
  if (ret == NULL){
    char white[INSTR_DEFAULT_STR_SIZE] = "1 1 1";
    if (!color){
      ret = newType (typename, typename, white, TYPE_VARIABLE, father);
    }else{
      ret = newType (typename, typename, color, TYPE_VARIABLE, father);
    }
    DEBUG4("VariableType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    new_pajeDefineVariableType (ret);
  }
  return ret;
}

char *getVariableTypeIdByName (const char *name, type_t father)
{
  xbt_dict_cursor_t cursor = NULL;
  type_t type;
  char *key;
  xbt_dict_foreach(father->children, cursor, key, type) {
    if (strcmp (name, type->name) == 0) return type->id;
  }
  return NULL;
}

type_t getLinkType (const char *typename, type_t father, type_t source, type_t dest)
{
  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%s-%s-%s", typename, source->id, dest->id);
  type_t ret = xbt_dict_get_or_null (father->children, key);
  if (ret == NULL){
    ret = newType (typename, key, NULL, TYPE_LINK, father);
    DEBUG8("LinkType %s(%s), child of %s(%s)  %s(%s)->%s(%s)", ret->name, ret->id, father->name, father->id, source->name, source->id, dest->name, dest->id);
    new_pajeDefineLinkType(ret, source, dest);
  }
  return ret;
}

type_t getStateType (const char *typename, type_t father)
{
  type_t ret = xbt_dict_get_or_null (father->children, typename);
  if (ret == NULL){
    ret = newType (typename, typename, NULL, TYPE_STATE, father);
    DEBUG4("StateType %s(%s), child of %s(%s)", ret->name, ret->id, father->name, father->id);
    new_pajeDefineStateType(ret);
  }
  return ret;
}

container_t newContainer (const char *name, e_container_types kind, container_t father)
{
  static long long int container_id = 0;
  char id_str[INSTR_DEFAULT_STR_SIZE];
  snprintf (id_str, INSTR_DEFAULT_STR_SIZE, "%lld", container_id++);

  container_t new = xbt_new0(s_container_t, 1);
  new->name = xbt_strdup (name); // name of the container
  new->id = xbt_strdup (id_str); // id (or alias) of the container
  new->father = father;
  // level depends on level of father
  if (new->father){
    new->level = new->father->level+1;
    DEBUG2("new container %s, child of %s", name, father->name);
  }else{
    new->level = 0;
  }
  // type definition (method depends on kind of this new container)
  new->kind = kind;
  if (new->kind == INSTR_AS){
    //if this container is of an AS, its type name depends on its level
    char as_typename[INSTR_DEFAULT_STR_SIZE];
    snprintf (as_typename, INSTR_DEFAULT_STR_SIZE, "L%d", new->level);
    if (new->father){
      new->type = getContainerType (as_typename, new->father->type);
    }else{
      new->type = getContainerType ("0", NULL);
    }
  }else{
    //otherwise, the name is its kind
    switch (new->kind){
      case INSTR_HOST: new->type = getContainerType ("HOST", new->father->type); break;
      case INSTR_LINK: new->type = getContainerType ("LINK", new->father->type); break;
      case INSTR_ROUTER: new->type = getContainerType ("ROUTER", new->father->type); break;
      case INSTR_SMPI: new->type = getContainerType ("MPI", new->father->type); break;
      case INSTR_MSG_PROCESS: new->type = getContainerType ("MSG_PROCESS", new->father->type); break;
      case INSTR_MSG_TASK: new->type = getContainerType ("MSG_TASK", new->father->type); break;
      default: xbt_die ("Congratulations, you have found a bug on newContainer function of instr_routing.c"); break;
    }
  }
  new->children = xbt_dict_new();
  if (new->father){
    xbt_dict_set(new->father->children, new->name, new, NULL);
    new_pajeCreateContainer (new);
  }

  //register hosts, routers, links containers
  if (new->kind == INSTR_HOST || new->kind == INSTR_LINK || new->kind == INSTR_ROUTER) {
    xbt_dict_set (allContainers, new->name, new, NULL);

    //register NODE types for triva configuration
    xbt_dict_set (trivaNodeTypes, new->type->name, xbt_strdup("1"), xbt_free);
  }
  return new;
}

static container_t recursiveGetContainer (const char *name, container_t root)
{
  if (strcmp (root->name, name) == 0) return root;

  xbt_dict_cursor_t cursor = NULL;
  container_t child;
  char *child_name;
  xbt_dict_foreach(root->children, cursor, child_name, child) {
    container_t ret = recursiveGetContainer(name, child);
    if (ret) return ret;
  }
  return NULL;
}

container_t getContainer (const char *name)
{
  return recursiveGetContainer(name, rootContainer);
}

container_t getContainerByName (const char *name)
{
  return (container_t)xbt_dict_get_or_null (allContainers, name);
}

char *getContainerIdByName (const char *name)
{
  return getContainerByName(name)->id;
}

container_t getRootContainer ()
{
  return rootContainer;
}

static type_t recursiveGetType (const char *name, type_t root)
{
  if (strcmp (root->name, name) == 0) return root;

  xbt_dict_cursor_t cursor = NULL;
  type_t child;
  char *child_name;
  xbt_dict_foreach(root->children, cursor, child_name, child) {
    type_t ret = recursiveGetType(name, child);
    if (ret) return ret;
  }
  return NULL;
}

type_t getType (const char *name, type_t father)
{
  return recursiveGetType (name, father);
}

void destroyContainer (container_t container)
{
  //remove me from my father
  if (container->father){
    xbt_dict_remove(container->father->children, container->name);
  }

  DEBUG1("destroy container %s", container->name);

  //obligation to dump previous events because they might
  //reference the container that is about to be destroyed
  TRACE_last_timestamp_to_dump = surf_get_clock();
  TRACE_paje_dump_buffer(1);

  //trace my destruction
  new_pajeDestroyContainer(container);

  //free
  xbt_free (container->name);
  xbt_free (container->id);
  xbt_free (container->children);
  xbt_free (container);
  container = NULL;
}

static void recursiveDestroyContainer (container_t container)
{
  xbt_dict_cursor_t cursor = NULL;
  container_t child;
  char *child_name;
  xbt_dict_foreach(container->children, cursor, child_name, child) {
    recursiveDestroyContainer (child);
  }
  destroyContainer (container);
}

static void recursiveDestroyType (type_t type)
{
  xbt_dict_cursor_t cursor = NULL;
  type_t child;
  char *child_name;
  xbt_dict_foreach(type->children, cursor, child_name, child) {
    recursiveDestroyType (child);
  }
  xbt_free (type->name);
  xbt_free (type->id);
  xbt_free (type->children);
  xbt_free (type->values);
  xbt_free (type);
  type = NULL;
}

void destroyAllContainers ()
{
  if (getRootContainer()) recursiveDestroyContainer (getRootContainer());
  if (getRootType()) recursiveDestroyType (getRootType());
  rootContainer = NULL;
  rootType = NULL;
}


#endif /* HAVE_TRACING */
