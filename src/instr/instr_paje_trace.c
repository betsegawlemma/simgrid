/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_trace, instr, "Paje tracing event system");

typedef enum {
  PAJE_DefineContainerType,
  PAJE_DefineVariableType,
  PAJE_DefineStateType,
  PAJE_DefineEventType,
  PAJE_DefineLinkType,
  PAJE_CreateContainer,
  PAJE_DestroyContainer,
  PAJE_SetVariable,
  PAJE_AddVariable,
  PAJE_SubVariable,
  PAJE_SetState,
  PAJE_PushState,
  PAJE_PopState,
  PAJE_StartLink,
  PAJE_EndLink,
  PAJE_NewEvent,
} e_event_type;

typedef struct paje_event *paje_event_t;
typedef struct paje_event {
  double timestamp;
  e_event_type event_type;
  void (*print) (paje_event_t event);
  void (*free) (paje_event_t event);
  void *data;
} s_paje_event_t;

typedef struct s_defineContainerType *defineContainerType_t;
typedef struct s_defineContainerType {
  type_t type;
}s_defineContainerType_t;

typedef struct s_defineVariableType *defineVariableType_t;
typedef struct s_defineVariableType {
  type_t type;
}s_defineVariableType_t;

typedef struct s_defineStateType *defineStateType_t;
typedef struct s_defineStateType {
  type_t type;
}s_defineStateType_t;

typedef struct s_defineEventType *defineEventType_t;
typedef struct s_defineEventType {
  type_t type;
}s_defineEventType_t;

typedef struct s_defineLinkType *defineLinkType_t;
typedef struct s_defineLinkType {
  type_t type;
  type_t source;
  type_t dest;
}s_defineLinkType_t;

typedef struct s_createContainer *createContainer_t;
typedef struct s_createContainer {
  container_t container;
}s_createContainer_t;

typedef struct s_destroyContainer *destroyContainer_t;
typedef struct s_destroyContainer {
  container_t container;
}s_destroyContainer_t;

typedef struct s_setVariable *setVariable_t;
typedef struct s_setVariable {
  container_t container;
  type_t type;
  double value;
}s_setVariable_t;

typedef struct s_addVariable *addVariable_t;
typedef struct s_addVariable {
  container_t container;
  type_t type;
  double value;
}s_addVariable_t;

typedef struct s_subVariable *subVariable_t;
typedef struct s_subVariable {
  container_t container;
  type_t type;
  double value;
}s_subVariable_t;

typedef struct s_setState *setState_t;
typedef struct s_setState {
  container_t container;
  type_t type;
  char *value;
}s_setState_t;

typedef struct s_pushState *pushState_t;
typedef struct s_pushState {
  container_t container;
  type_t type;
  char *value;
}s_pushState_t;

typedef struct s_popState *popState_t;
typedef struct s_popState {
  container_t container;
  type_t type;
}s_popState_t;

typedef struct s_startLink *startLink_t;
typedef struct s_startLink {
  container_t container;
  type_t type;
  container_t sourceContainer;
  char *value;
  char *key;
}s_startLink_t;

typedef struct s_endLink *endLink_t;
typedef struct s_endLink {
  container_t container;
  type_t type;
  container_t destContainer;
  char *value;
  char *key;
}s_endLink_t;

typedef struct s_newEvent *newEvent_t;
typedef struct s_newEvent {
  container_t container;
  type_t type;
  char *value;
}s_newEvent_t;

static FILE *tracing_file = NULL;

static xbt_dynar_t buffer = NULL;

void TRACE_paje_start(void)
{
  char *filename = TRACE_get_filename();
  tracing_file = fopen(filename, "w");
  xbt_assert1 (tracing_file != NULL, "Tracefile %s could not be opened for writing.", filename);

  DEBUG1("Filename %s is open for writing", filename);

  /* output header */
  TRACE_paje_create_header();

  buffer = xbt_dynar_new (sizeof(paje_event_t), NULL);
}

void TRACE_paje_end(void)
{
  fclose(tracing_file);
  char *filename = TRACE_get_filename();
  DEBUG1("Filename %s is closed", filename);
}

double TRACE_last_timestamp_to_dump = 0;
//dumps the trace file until the timestamp TRACE_last_timestamp_to_dump
void TRACE_paje_dump_buffer (void)
{
  paje_event_t event;
  while (xbt_dynar_length (buffer) > 0){
    double head_timestamp = (*(paje_event_t*)xbt_dynar_get_ptr(buffer, 0))->timestamp;
    if (head_timestamp > TRACE_last_timestamp_to_dump){
      break;
    }
    xbt_dynar_remove_at (buffer, 0, &event);
    event->print (event);
    event->free (event);
  }
}

void TRACE_paje_create_header(void)
{
  DEBUG0 ("Define paje header");
  fprintf(tracing_file, "\
%%EventDef PajeDefineContainerType %d \n\
%%       Alias string \n\
%%       ContainerType string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeDefineVariableType %d \n\
%%       Alias string \n\
%%       ContainerType string \n\
%%       Name string \n\
%%       Color color \n\
%%EndEventDef \n\
%%EventDef PajeDefineStateType %d \n\
%%       Alias string \n\
%%       ContainerType string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeDefineEventType %d \n\
%%       Alias string \n\
%%       EntityType string \n\
%%       Name string \n\
%%       Color color \n\
%%EndEventDef \n\
%%EventDef PajeDefineLinkType %d \n\
%%       Alias string \n\
%%       ContainerType string \n\
%%       SourceContainerType string \n\
%%       DestContainerType string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeCreateContainer %d \n\
%%       Time date \n\
%%       Alias string \n\
%%       Type string \n\
%%       Container string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeDestroyContainer %d \n\
%%       Time date \n\
%%       Type string \n\
%%       Container string \n\
%%EndEventDef \n\
%%EventDef PajeSetVariable %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n\
%%EventDef PajeAddVariable %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n\
%%EventDef PajeSubVariable %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n\
%%EventDef PajeSetState %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n\
%%EventDef PajePushState %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n\
%%EventDef PajePopState %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%EndEventDef\n\
%%EventDef PajeStartLink %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%       SourceContainer string \n\
%%       Key string \n\
%%EndEventDef\n\
%%EventDef PajeEndLink %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%       DestContainer string \n\
%%       Key string \n\
%%EndEventDef\n\
%%EventDef PajeNewEvent %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n",
  PAJE_DefineContainerType,
  PAJE_DefineVariableType,
  PAJE_DefineStateType,
  PAJE_DefineEventType,
  PAJE_DefineLinkType,
  PAJE_CreateContainer,
  PAJE_DestroyContainer,
  PAJE_SetVariable,
  PAJE_AddVariable,
  PAJE_SubVariable,
  PAJE_SetState,
  PAJE_PushState,
  PAJE_PopState,
  PAJE_StartLink,
  PAJE_EndLink,
  PAJE_NewEvent);
}

/* internal do the instrumentation module */
static void insert_into_buffer (paje_event_t tbi)
{
  unsigned int i;
  if (xbt_dynar_length(buffer) == 0){
    xbt_dynar_push (buffer, &tbi);
  }else{
    int inserted = 0;
    for (i = 0; i < xbt_dynar_length(buffer); i++){
      paje_event_t e1 = *(paje_event_t*)xbt_dynar_get_ptr(buffer, i);
      if (e1->timestamp > tbi->timestamp){
        xbt_dynar_insert_at (buffer, i, &tbi);
        inserted = 1;
        break;
      }
    }
    if (!inserted){
      xbt_dynar_push (buffer, &tbi);
    }
  }
}

static void print_pajeDefineContainerType(paje_event_t event)
{
  fprintf(tracing_file, "%d %s %s %s\n",
      event->event_type,
      ((defineContainerType_t)event->data)->type->id,
      ((defineContainerType_t)event->data)->type->father->id,
      ((defineContainerType_t)event->data)->type->name);
}

static void print_pajeDefineVariableType(paje_event_t event)
{
  fprintf(tracing_file, "%d %s %s %s \"%s\"\n",
      event->event_type,
      ((defineVariableType_t)event->data)->type->id,
      ((defineVariableType_t)event->data)->type->father->id,
      ((defineVariableType_t)event->data)->type->name,
      ((defineVariableType_t)event->data)->type->color);
}

static void print_pajeDefineStateType(paje_event_t event)
{
  fprintf(tracing_file, "%d %s %s %s\n",
      event->event_type,
      ((defineStateType_t)event->data)->type->id,
      ((defineStateType_t)event->data)->type->father->id,
      ((defineStateType_t)event->data)->type->name);
}

static void print_pajeDefineEventType(paje_event_t event)
{
  fprintf(tracing_file, "%d %s %s %s \"%s\"\n",
      event->event_type,
      ((defineEventType_t)event->data)->type->id,
      ((defineEventType_t)event->data)->type->father->id,
      ((defineEventType_t)event->data)->type->name,
      ((defineEventType_t)event->data)->type->color);
}

static void print_pajeDefineLinkType(paje_event_t event)
{
  fprintf(tracing_file, "%d %s %s %s %s %s\n",
      event->event_type,
      ((defineLinkType_t)event->data)->type->id,
      ((defineLinkType_t)event->data)->type->father->id,
      ((defineLinkType_t)event->data)->source->id,
      ((defineLinkType_t)event->data)->dest->id,
      ((defineLinkType_t)event->data)->type->name);
}

static void print_pajeCreateContainer(paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s %s\n",
        event->event_type,
        ((createContainer_t)event->data)->container->id,
        ((createContainer_t)event->data)->container->type->id,
        ((createContainer_t)event->data)->container->father->id,
        ((createContainer_t)event->data)->container->name);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %s %s\n",
        event->event_type,
        event->timestamp,
        ((createContainer_t)event->data)->container->id,
        ((createContainer_t)event->data)->container->type->id,
        ((createContainer_t)event->data)->container->father->id,
        ((createContainer_t)event->data)->container->name);
  }
}

static void print_pajeDestroyContainer(paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s\n",
        event->event_type,
        ((destroyContainer_t)event->data)->container->type->id,
        ((destroyContainer_t)event->data)->container->id);
  }else{
    fprintf(tracing_file, "%d %lf %s %s\n",
        event->event_type,
        event->timestamp,
        ((destroyContainer_t)event->data)->container->type->id,
        ((destroyContainer_t)event->data)->container->id);
  }
}

static void print_pajeSetVariable(paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %f\n",
        event->event_type,
        ((setVariable_t)event->data)->type->id,
        ((setVariable_t)event->data)->container->id,
        ((setVariable_t)event->data)->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %f\n",
        event->event_type,
        event->timestamp,
        ((setVariable_t)event->data)->type->id,
        ((setVariable_t)event->data)->container->id,
        ((setVariable_t)event->data)->value);
  }
}

static void print_pajeAddVariable(paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %f\n",
        event->event_type,
        ((addVariable_t)event->data)->type->id,
        ((addVariable_t)event->data)->container->id,
        ((addVariable_t)event->data)->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %f\n",
        event->event_type,
        event->timestamp,
        ((addVariable_t)event->data)->type->id,
        ((addVariable_t)event->data)->container->id,
        ((addVariable_t)event->data)->value);
  }
}

static void print_pajeSubVariable(paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %f\n",
        event->event_type,
        ((subVariable_t)event->data)->type->id,
        ((subVariable_t)event->data)->container->id,
        ((subVariable_t)event->data)->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %f\n",
        event->event_type,
        event->timestamp,
        ((subVariable_t)event->data)->type->id,
        ((subVariable_t)event->data)->container->id,
        ((subVariable_t)event->data)->value);
  }
}

static void print_pajeSetState(paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s\n",
        event->event_type,
        ((setState_t)event->data)->type->id,
        ((setState_t)event->data)->container->id,
        ((setState_t)event->data)->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %s\n",
        event->event_type,
        event->timestamp,
        ((setState_t)event->data)->type->id,
        ((setState_t)event->data)->container->id,
        ((setState_t)event->data)->value);
  }
}

static void print_pajePushState(paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s\n",
        event->event_type,
        ((pushState_t)event->data)->type->id,
        ((pushState_t)event->data)->container->id,
        ((pushState_t)event->data)->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %s\n",
        event->event_type,
        event->timestamp,
        ((pushState_t)event->data)->type->id,
        ((pushState_t)event->data)->container->id,
        ((pushState_t)event->data)->value);
  }
}

static void print_pajePopState(paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s\n",
        event->event_type,
        ((popState_t)event->data)->type->id,
        ((popState_t)event->data)->container->id);
  }else{
    fprintf(tracing_file, "%d %lf %s %s\n",
        event->event_type,
        event->timestamp,
        ((popState_t)event->data)->type->id,
        ((popState_t)event->data)->container->id);
  }
}

static void print_pajeStartLink(paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s %s %s\n",
        event->event_type,
        ((startLink_t)event->data)->type->id,
        ((startLink_t)event->data)->container->id,
        ((startLink_t)event->data)->value,
        ((startLink_t)event->data)->sourceContainer->id,
        ((startLink_t)event->data)->key);
  }else {
    fprintf(tracing_file, "%d %lf %s %s %s %s %s\n",
        event->event_type,
        event->timestamp,
        ((startLink_t)event->data)->type->id,
        ((startLink_t)event->data)->container->id,
        ((startLink_t)event->data)->value,
        ((startLink_t)event->data)->sourceContainer->id,
        ((startLink_t)event->data)->key);
  }
}

static void print_pajeEndLink(paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s %s %s\n",
        event->event_type,
        ((endLink_t)event->data)->type->id,
        ((endLink_t)event->data)->container->id,
        ((endLink_t)event->data)->value,
        ((endLink_t)event->data)->destContainer->id,
        ((endLink_t)event->data)->key);
  }else {
    fprintf(tracing_file, "%d %lf %s %s %s %s %s\n",
        event->event_type,
        event->timestamp,
        ((endLink_t)event->data)->type->id,
        ((endLink_t)event->data)->container->id,
        ((endLink_t)event->data)->value,
        ((endLink_t)event->data)->destContainer->id,
        ((endLink_t)event->data)->key);
  }
}

static void print_pajeNewEvent (paje_event_t event)
{
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s\n",
        event->event_type,
        ((newEvent_t)event->data)->type->id,
        ((newEvent_t)event->data)->container->id,
        ((newEvent_t)event->data)->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %s\n",
        event->event_type,
        event->timestamp,
        ((newEvent_t)event->data)->type->id,
        ((newEvent_t)event->data)->container->id,
        ((newEvent_t)event->data)->value);
  }
}

static void free_paje_event (paje_event_t event)
{
  if (event->event_type == PAJE_SetState) {
    xbt_free (((setState_t)(event->data))->value);
  }else if (event->event_type == PAJE_PushState) {
    xbt_free (((pushState_t)(event->data))->value);
  }else if (event->event_type == PAJE_NewEvent){
    xbt_free (((newEvent_t)(event->data))->value);
  }else if (event->event_type == PAJE_StartLink){
    xbt_free (((startLink_t)(event->data))->value);
    xbt_free (((startLink_t)(event->data))->key);
  }else if (event->event_type == PAJE_EndLink){
    xbt_free (((endLink_t)(event->data))->value);
    xbt_free (((endLink_t)(event->data))->key);
  }
  xbt_free (event->data);
  xbt_free (event);
}

void new_pajeDefineContainerType(type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineContainerType;
  event->timestamp = 0;
  event->print = print_pajeDefineContainerType;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineContainerType_t, 1);
  ((defineContainerType_t)(event->data))->type = type;

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDefineVariableType(type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineVariableType;
  event->timestamp = 0;
  event->print = print_pajeDefineVariableType;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineVariableType_t, 1);
  ((defineVariableType_t)(event->data))->type = type;

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDefineStateType(type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineStateType;
  event->timestamp = 0;
  event->print = print_pajeDefineStateType;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineStateType_t, 1);
  ((defineStateType_t)(event->data))->type = type;

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDefineEventType(type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineEventType;
  event->timestamp = 0;
  event->print = print_pajeDefineEventType;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineEventType_t, 1);
  ((defineEventType_t)(event->data))->type = type;

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDefineLinkType(type_t type, type_t source, type_t dest)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineLinkType;
  event->timestamp = 0;
  event->print = print_pajeDefineLinkType;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineLinkType_t, 1);
  ((defineLinkType_t)(event->data))->type = type;
  ((defineLinkType_t)(event->data))->source = source;
  ((defineLinkType_t)(event->data))->dest = dest;

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeCreateContainer (container_t container)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_CreateContainer;
  event->timestamp = SIMIX_get_clock();
  event->print = print_pajeCreateContainer;
  event->free = free_paje_event;
  event->data = xbt_new0(s_createContainer_t, 1);
  ((createContainer_t)(event->data))->container = container;

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDestroyContainer (container_t container)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DestroyContainer;
  event->timestamp = SIMIX_get_clock();
  event->print = print_pajeDestroyContainer;
  event->free = free_paje_event;
  event->data = xbt_new0(s_destroyContainer_t, 1);
  ((destroyContainer_t)(event->data))->container = container;

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeSetVariable (double timestamp, container_t container, type_t type, double value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_SetVariable;
  event->timestamp = timestamp;
  event->print = print_pajeSetVariable;
  event->free = free_paje_event;
  event->data = xbt_new0(s_setVariable_t, 1);
  ((setVariable_t)(event->data))->type = type;
  ((setVariable_t)(event->data))->container = container;
  ((setVariable_t)(event->data))->value = value;

  insert_into_buffer (event);
}


void new_pajeAddVariable (double timestamp, container_t container, type_t type, double value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_AddVariable;
  event->timestamp = timestamp;
  event->print = print_pajeAddVariable;
  event->free = free_paje_event;
  event->data = xbt_new0(s_addVariable_t, 1);
  ((addVariable_t)(event->data))->type = type;
  ((addVariable_t)(event->data))->container = container;
  ((addVariable_t)(event->data))->value = value;

  insert_into_buffer (event);
}

void new_pajeSubVariable (double timestamp, container_t container, type_t type, double value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_SubVariable;
  event->timestamp = timestamp;
  event->print = print_pajeSubVariable;
  event->free = free_paje_event;
  event->data = xbt_new0(s_subVariable_t, 1);
  ((subVariable_t)(event->data))->type = type;
  ((subVariable_t)(event->data))->container = container;
  ((subVariable_t)(event->data))->value = value;

  insert_into_buffer (event);
}

void new_pajeSetState (double timestamp, container_t container, type_t type, const char *value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_SetState;
  event->timestamp = timestamp;
  event->print = print_pajeSetState;
  event->free = free_paje_event;
  event->data = xbt_new0(s_setState_t, 1);
  ((setState_t)(event->data))->type = type;
  ((setState_t)(event->data))->container = container;
  ((setState_t)(event->data))->value = xbt_strdup(value);

  insert_into_buffer (event);
}


void new_pajePushState (double timestamp, container_t container, type_t type, const char *value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_PushState;
  event->timestamp = timestamp;
  event->print = print_pajePushState;
  event->free = free_paje_event;
  event->data = xbt_new0(s_pushState_t, 1);
  ((pushState_t)(event->data))->type = type;
  ((pushState_t)(event->data))->container = container;
  ((pushState_t)(event->data))->value = xbt_strdup(value);

  insert_into_buffer (event);
}


void new_pajePopState (double timestamp, container_t container, type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_PopState;
  event->timestamp = timestamp;
  event->print = print_pajePopState;
  event->free = free_paje_event;
  event->data = xbt_new0(s_popState_t, 1);
  ((popState_t)(event->data))->type = type;
  ((popState_t)(event->data))->container = container;

  insert_into_buffer (event);
}

void new_pajeStartLink (double timestamp, container_t container, type_t type, container_t sourceContainer, const char *value, const char *key)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_StartLink;
  event->timestamp = timestamp;
  event->print = print_pajeStartLink;
  event->free = free_paje_event;
  event->data = xbt_new0(s_startLink_t, 1);
  ((startLink_t)(event->data))->type = type;
  ((startLink_t)(event->data))->container = container;
  ((startLink_t)(event->data))->sourceContainer = sourceContainer;
  ((startLink_t)(event->data))->value = xbt_strdup(value);
  ((startLink_t)(event->data))->key = xbt_strdup(key);

  insert_into_buffer (event);
}

void new_pajeEndLink (double timestamp, container_t container, type_t type, container_t destContainer, const char *value, const char *key)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_EndLink;
  event->timestamp = timestamp;
  event->print = print_pajeEndLink;
  event->free = free_paje_event;
  event->data = xbt_new0(s_endLink_t, 1);
  ((endLink_t)(event->data))->type = type;
  ((endLink_t)(event->data))->container = container;
  ((endLink_t)(event->data))->destContainer = destContainer;
  ((endLink_t)(event->data))->value = xbt_strdup(value);
  ((endLink_t)(event->data))->key = xbt_strdup(key);

  insert_into_buffer (event);
}

void new_pajeNewEvent (double timestamp, container_t container, type_t type, const char *value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_NewEvent;
  event->timestamp = timestamp;
  event->print = print_pajeNewEvent;
  event->free = free_paje_event;
  event->data = xbt_new0(s_newEvent_t, 1);
  ((newEvent_t)(event->data))->type = type;
  ((newEvent_t)(event->data))->container = container;
  ((newEvent_t)(event->data))->value = xbt_strdup(value);

  insert_into_buffer (event);
}

#endif /* HAVE_TRACING */
