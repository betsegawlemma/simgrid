#include "storage.hpp"
#include "surf_private.h"

#define __STDC_FORMAT_MACROS

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_storage, surf,
                                "Logging specific to the SURF storage module");
}

xbt_lib_t storage_lib;
int ROUTING_STORAGE_LEVEL;      //Routing for storagelevel
int ROUTING_STORAGE_HOST_LEVEL;
int SURF_STORAGE_LEVEL;
xbt_lib_t storage_type_lib;
int ROUTING_STORAGE_TYPE_LEVEL; //Routing for storage_type level

static xbt_dynar_t storage_list;

xbt_dynar_t mount_list = NULL;  /* temporary store of current mount storage */
StorageModelPtr surf_storage_model = NULL;

static int storage_selective_update = 0;
static xbt_swag_t storage_running_action_set_that_does_not_need_being_checked = NULL;

/*************
 * CallBacks *
 *************/

static XBT_INLINE void routing_storage_type_free(void *r)
{
  storage_type_t stype = (storage_type_t) r;
  free(stype->model);
  free(stype->type_id);
  free(stype->content);
  xbt_dict_free(&(stype->properties));
  xbt_dict_free(&(stype->properties));
  free(stype);
}

static XBT_INLINE void surf_storage_resource_free(void *r)
{
  // specific to storage
  StoragePtr storage = dynamic_cast<StoragePtr>(static_cast<ResourcePtr>(r));
  xbt_dict_free(&storage->p_content);
  xbt_dynar_free(&storage->p_writeActions);
  free(storage->p_typeId);
  free(storage->p_contentType);
  // generic resource
  delete storage;
}

static XBT_INLINE void routing_storage_host_free(void *r)
{
  xbt_dynar_t dyn = (xbt_dynar_t) r;
  xbt_dynar_free(&dyn);
}

static void parse_storage_init(sg_platf_storage_cbarg_t storage)
{
  void* stype = xbt_lib_get_or_null(storage_type_lib,
                                    storage->type_id,
                                    ROUTING_STORAGE_TYPE_LEVEL);
  if(!stype) xbt_die("No storage type '%s'",storage->type_id);

  // if storage content is not specified use the content of storage_type if exist
  if(!strcmp(storage->content,"") && strcmp(((storage_type_t) stype)->content,"")){
    storage->content = ((storage_type_t) stype)->content;
    storage->content_type = ((storage_type_t) stype)->content_type;
    XBT_DEBUG("For disk '%s' content is empty, inherit the content (of type %s) from storage type '%s' ",
        storage->id,((storage_type_t) stype)->content_type,
        ((storage_type_t) stype)->type_id);
  }

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' "
      "\n\t\tmodel '%s' \n\t\tcontent '%s'\n\t\tcontent_type '%s' "
      "\n\t\tproperties '%p'\n",
      storage->id,
      ((storage_type_t) stype)->model,
      ((storage_type_t) stype)->type_id,
      storage->content,
      storage->content_type,
      ((storage_type_t) stype)->properties);

  surf_storage_model->createResource(storage->id, ((storage_type_t) stype)->model,
                                     ((storage_type_t) stype)->type_id,
                                     storage->content,
                                     storage->content_type,
                                     storage->properties);
}

static void parse_mstorage_init(sg_platf_mstorage_cbarg_t /*mstorage*/)
{
  XBT_DEBUG("parse_mstorage_init");
}

static void parse_storage_type_init(sg_platf_storage_type_cbarg_t /*storagetype_*/)
{
  XBT_DEBUG("parse_storage_type_init");
}

static void parse_mount_init(sg_platf_mount_cbarg_t /*mount*/)
{
  XBT_DEBUG("parse_mount_init");
}

static void storage_parse_storage(sg_platf_storage_cbarg_t storage)
{
  xbt_assert(!xbt_lib_get_or_null(storage_lib, storage->id,ROUTING_STORAGE_LEVEL),
               "Reading a storage, processing unit \"%s\" already exists", storage->id);

  // Verification of an existing type_id
#ifndef NDEBUG
  void* storage_type = xbt_lib_get_or_null(storage_type_lib, storage->type_id,ROUTING_STORAGE_TYPE_LEVEL);
#endif
  xbt_assert(storage_type,"Reading a storage, type id \"%s\" does not exists", storage->type_id);

  XBT_DEBUG("ROUTING Create a storage name '%s' with type_id '%s' and content '%s'",
      storage->id,
      storage->type_id,
      storage->content);

  xbt_lib_set(storage_lib,
      storage->id,
      ROUTING_STORAGE_LEVEL,
      (void *) xbt_strdup(storage->type_id));
}

static void storage_parse_storage_type(sg_platf_storage_type_cbarg_t storage_type)
{
  xbt_assert(!xbt_lib_get_or_null(storage_type_lib, storage_type->id,ROUTING_STORAGE_TYPE_LEVEL),
               "Reading a storage type, processing unit \"%s\" already exists", storage_type->id);

  storage_type_t stype = xbt_new0(s_storage_type_t, 1);
  stype->model = xbt_strdup(storage_type->model);
  stype->properties = storage_type->properties;
  stype->content = xbt_strdup(storage_type->content);
  stype->content_type = xbt_strdup(storage_type->content_type);
  stype->type_id = xbt_strdup(storage_type->id);
  stype->size = storage_type->size;

  XBT_DEBUG("ROUTING Create a storage type id '%s' with model '%s', "
      "content '%s', and content_type '%s'",
      stype->type_id,
      stype->model,
      storage_type->content,
      storage_type->content_type);

  xbt_lib_set(storage_type_lib,
      stype->type_id,
      ROUTING_STORAGE_TYPE_LEVEL,
      (void *) stype);
}
static void storage_parse_mstorage(sg_platf_mstorage_cbarg_t /*mstorage*/)
{
  THROW_UNIMPLEMENTED;
//  mount_t mnt = xbt_new0(s_mount_t, 1);
//  mnt->id = xbt_strdup(mstorage->type_id);
//  mnt->name = xbt_strdup(mstorage->name);
//
//  if(!mount_list){
//    XBT_DEBUG("Creata a Mount list for %s",A_surfxml_host_id);
//    mount_list = xbt_dynar_new(sizeof(char *), NULL);
//  }
//  xbt_dynar_push(mount_list,(void *) mnt);
//  free(mnt->id);
//  free(mnt->name);
//  xbt_free(mnt);
//  XBT_DEBUG("ROUTING Mount a storage name '%s' with type_id '%s'",mstorage->name, mstorage->id);
}

static void mount_free(void *p)
{
  mount_t mnt = (mount_t) p;
  xbt_free(mnt->name);
}

static void storage_parse_mount(sg_platf_mount_cbarg_t mount)
{
  // Verification of an existing storage
#ifndef NDEBUG
  void* storage = xbt_lib_get_or_null(storage_lib, mount->storageId, ROUTING_STORAGE_LEVEL);
#endif
  xbt_assert(storage,"Disk id \"%s\" does not exists", mount->storageId);

  XBT_DEBUG("ROUTING Mount '%s' on '%s'",mount->storageId, mount->name);

  s_mount_t mnt;
  mnt.storage = surf_storage_resource_priv(surf_storage_resource_by_name(mount->storageId));
  mnt.name = xbt_strdup(mount->name);

  if(!mount_list){
    XBT_DEBUG("Create a Mount list for %s",A_surfxml_host_id);
    mount_list = xbt_dynar_new(sizeof(s_mount_t), mount_free);
  }
  xbt_dynar_push(mount_list,&mnt);
}

static void storage_define_callbacks()
{
  sg_platf_storage_add_cb(parse_storage_init);
  sg_platf_storage_type_add_cb(parse_storage_type_init);
  sg_platf_mstorage_add_cb(parse_mstorage_init);
  sg_platf_mount_add_cb(parse_mount_init);
}

void storage_register_callbacks() {

  ROUTING_STORAGE_LEVEL = xbt_lib_add_level(storage_lib,xbt_free);
  ROUTING_STORAGE_HOST_LEVEL = xbt_lib_add_level(storage_lib, routing_storage_host_free);
  ROUTING_STORAGE_TYPE_LEVEL = xbt_lib_add_level(storage_type_lib, routing_storage_type_free);
  SURF_STORAGE_LEVEL = xbt_lib_add_level(storage_lib, surf_storage_resource_free);

  sg_platf_storage_add_cb(storage_parse_storage);
  sg_platf_mstorage_add_cb(storage_parse_mstorage);
  sg_platf_storage_type_add_cb(storage_parse_storage_type);
  sg_platf_mount_add_cb(storage_parse_mount);
}

/*********
 * Model *
 *********/

void surf_storage_model_init_default(void)
{
  surf_storage_model = new StorageModel();
  storage_define_callbacks();
  xbt_dynar_push(model_list, &surf_storage_model);
}

StorageModel::StorageModel() : Model("Storage") {
  StorageActionLmm action;

  XBT_DEBUG("surf_storage_model_init_internal");

  storage_running_action_set_that_does_not_need_being_checked =
      xbt_swag_new(xbt_swag_offset(action, p_stateHookup));

  if (!p_maxminSystem) {
    p_maxminSystem = lmm_system_new(storage_selective_update);
  }
}


StorageModel::~StorageModel(){
  lmm_system_free(p_maxminSystem);

  surf_storage_model = NULL;

  xbt_dynar_free(&storage_list);

  xbt_swag_free(storage_running_action_set_that_does_not_need_being_checked);
  storage_running_action_set_that_does_not_need_being_checked = NULL;
}

StoragePtr StorageModel::createResource(const char* id, const char* model, const char* type_id,
		const char* content_name, const char* content_type, xbt_dict_t properties)
{

  xbt_assert(!surf_storage_resource_priv(surf_storage_resource_by_name(id)),
              "Storage '%s' declared several times in the platform file",
              id);

  storage_type_t storage_type = (storage_type_t) xbt_lib_get_or_null(storage_type_lib, type_id,ROUTING_STORAGE_TYPE_LEVEL);

  double Bread  = surf_parse_get_bandwidth((char*)xbt_dict_get(storage_type->properties, "Bread"));
  double Bwrite = surf_parse_get_bandwidth((char*)xbt_dict_get(storage_type->properties, "Bwrite"));
  double Bconnection   = surf_parse_get_bandwidth((char*)xbt_dict_get(storage_type->properties, "Bconnection"));

  StoragePtr storage = new StorageLmm(this, id, properties, p_maxminSystem,
		  Bread, Bwrite, Bconnection,
		  type_id, (char *)content_name, xbt_strdup(content_type), storage_type->size);

  xbt_lib_set(storage_lib, id, SURF_STORAGE_LEVEL, static_cast<ResourcePtr>(storage));

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' \n\t\tmodel '%s' \n\t\tproperties '%p'\n\t\tBread '%f'\n",
      id,
      model,
      type_id,
      storage_type->properties,
      Bread);

  if(!storage_list)
	storage_list = xbt_dynar_new(sizeof(char *),NULL);
  xbt_dynar_push(storage_list, &storage);

  return storage;
}

double StorageModel::shareResources(double now)
{
  XBT_DEBUG("storage_share_resources %f", now);
  unsigned int i, j;
  StoragePtr storage;
  void *_write_action;
  StorageActionLmmPtr write_action;

  double min_completion = shareResourcesMaxMin(p_runningActionSet,
      p_maxminSystem, lmm_solve);

  double rate;
  // Foreach disk
  xbt_dynar_foreach(storage_list,i,storage)
  {
    rate = 0;
    // Foreach write action on disk
    xbt_dynar_foreach(storage->p_writeActions, j, _write_action)
    {
      write_action = dynamic_cast<StorageActionLmmPtr>(static_cast<ActionPtr>(_write_action));
      rate += lmm_variable_getvalue(write_action->p_variable);
    }
    if(rate > 0)
      min_completion = MIN(min_completion, (storage->m_size-storage->m_usedSize)/rate);
  }

  return min_completion;
}

void StorageModel::updateActionsState(double /*now*/, double delta)
{
  void *_action, *_next_action;
  StorageActionLmmPtr action = NULL;

  xbt_swag_foreach_safe(_action, _next_action, p_runningActionSet) {
	action = dynamic_cast<StorageActionLmmPtr>(static_cast<ActionPtr>(_action));
    if(action->m_type == WRITE)
    {
      // Update the disk usage
     // Update the file size
     // For each action of type write
      double rate = lmm_variable_getvalue(action->p_variable);
      /* Hack to avoid rounding differences between x86 and x86_64
       * (note that the next sizes are of type sg_size_t). */
      long incr = delta * rate + MAXMIN_PRECISION;
      action->p_storage->m_usedSize += incr; // disk usage
      action->p_file->size += incr; // file size

      sg_size_t *psize = xbt_new(sg_size_t,1);
      *psize = action->p_file->size;

      xbt_dict_t content_dict = action->p_storage->p_content;
      xbt_dict_set(content_dict, action->p_file->name, psize, NULL);
    }

    double_update(&action->m_remains,
                  lmm_variable_getvalue(action->p_variable) * delta);

    if (action->m_maxDuration != NO_MAX_DURATION)
      double_update(&action->m_maxDuration, delta);

    if(action->m_remains > 0 &&
        lmm_get_variable_weight(action->p_variable) > 0 &&
        action->p_storage->m_usedSize == action->p_storage->m_size)
    {
      action->m_finish = surf_get_clock();
      action->setState(SURF_ACTION_FAILED);
    } else if ((action->m_remains <= 0) &&
        (lmm_get_variable_weight(action->p_variable) > 0))
    {
      action->m_finish = surf_get_clock();
      action->setState(SURF_ACTION_DONE);
    } else if ((action->m_maxDuration != NO_MAX_DURATION) &&
               (action->m_maxDuration <= 0))
    {
      action->m_finish = surf_get_clock();
      action->setState(SURF_ACTION_DONE);
    }
  }

  return;
}

xbt_dict_t Storage::parseContent(char *filename)
{
  m_usedSize = 0;
  if ((!filename) || (strcmp(filename, "") == 0))
    return NULL;

  xbt_dict_t parse_content = xbt_dict_new_homogeneous(xbt_free);
  FILE *file = NULL;

  file = surf_fopen(filename, "r");
  xbt_assert(file != NULL, "Cannot open file '%s' (path=%s)", filename,
              xbt_str_join(surf_path, ":"));

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char path[1024];
  sg_size_t size;


  while ((read = xbt_getline(&line, &len, file)) != -1) {
    if (read){
    if(sscanf(line,"%s %llu", path, &size) == 2) {
        m_usedSize += size;
        sg_size_t *psize = xbt_new(sg_size_t, 1);
        *psize = size;
        xbt_dict_set(parse_content,path,psize,NULL);
      } else {
        xbt_die("Be sure of passing a good format for content file.\n");
      }
    }
  }
  free(line);
  fclose(file);
  return parse_content;
}

/************
 * Resource *
 ************/

Storage::Storage(StorageModelPtr model, const char* name, xbt_dict_t properties)
:  Resource(model, name, properties)
{
  p_writeActions = xbt_dynar_new(sizeof(ActionPtr),NULL);
}

StorageLmm::StorageLmm(StorageModelPtr model, const char* name, xbt_dict_t properties,
	     lmm_system_t maxminSystem, double bread, double bwrite, double bconnection,
	     const char* type_id, char *content_name, char *content_type, sg_size_t size)
 :  Resource(model, name, properties), ResourceLmm(), Storage(model, name, properties) {
  XBT_DEBUG("Create resource with Bconnection '%f' Bread '%f' Bwrite '%f' and Size '%llu'", bconnection, bread, bwrite, size);

  p_stateCurrent = SURF_RESOURCE_ON;
  m_usedSize = 0;
  m_size = 0;

  p_content = parseContent(content_name);
  p_contentType = content_type;
  p_constraint = lmm_constraint_new(maxminSystem, this, bconnection);
  p_constraintRead  = lmm_constraint_new(maxminSystem, this, bread);
  p_constraintWrite = lmm_constraint_new(maxminSystem, this, bwrite);
  m_size = size;
  p_typeId = xbt_strdup(type_id);
}

bool Storage::isUsed()
{
  THROW_UNIMPLEMENTED;
  return false;
}

void Storage::updateState(tmgr_trace_event_t /*event_type*/, double /*value*/, double /*date*/)
{
  THROW_UNIMPLEMENTED;
}

StorageActionPtr StorageLmm::ls(const char* path)
{
  StorageActionLmmPtr action = new StorageActionLmm(p_model, 0, p_stateCurrent != SURF_RESOURCE_ON, this, LS);

  action->p_lsDict = NULL;
  xbt_dict_t ls_dict = xbt_dict_new_homogeneous(xbt_free);

  char* key;
  sg_size_t size = 0;
  xbt_dict_cursor_t cursor = NULL;

  xbt_dynar_t dyn = NULL;
  char* file = NULL;

  // for each file in the storage content
  xbt_dict_foreach(p_content,cursor,key,size){
    // Search if file start with the prefix 'path'
    if(xbt_str_start_with(key,path)){
      file = &key[strlen(path)];

      // Split file with '/'
      dyn = xbt_str_split(file,"/");
      file = xbt_dynar_get_as(dyn,0,char*);

      // file
      if(xbt_dynar_length(dyn) == 1){
        sg_size_t *psize = xbt_new(sg_size_t, 1);
        *psize=size;
        xbt_dict_set(ls_dict, file, psize, NULL);
      }
      // Directory
      else
      {
        // if directory does not exist yet in the dictionary
        if(!xbt_dict_get_or_null(ls_dict,file))
          xbt_dict_set(ls_dict,file,NULL,NULL);
      }
      xbt_dynar_free(&dyn);
    }
  }

  action->p_lsDict = ls_dict;
  return action;
}

StorageActionPtr StorageLmm::open(const char* mount, const char* path)
{
  XBT_DEBUG("\tOpen file '%s'",path);
  sg_size_t size, *psize;
  psize = (sg_size_t*) xbt_dict_get_or_null(p_content, path);
  // if file does not exist create an empty file
  if(psize)
    size = *psize;
  else {
	psize = xbt_new(sg_size_t,1);
    size = 0;
    *psize = size;
    xbt_dict_set(p_content, path, psize, NULL);
    XBT_DEBUG("File '%s' was not found, file created.",path);
  }
  surf_file_t file = xbt_new0(s_surf_file_t,1);
  file->name = xbt_strdup(path);
  file->size = size;
  file->mount = xbt_strdup(mount);

  StorageActionLmmPtr action = new StorageActionLmm(p_model, 0, p_stateCurrent != SURF_RESOURCE_ON, this, OPEN);
  action->p_file = file;
  return action;
}

StorageActionPtr StorageLmm::close(surf_file_t fd)
{
  char *filename = fd->name;
  XBT_DEBUG("\tClose file '%s' size '%llu'", filename, fd->size);
  // unref write actions from storage
  void *_write_action;
  StorageActionLmmPtr write_action;
  unsigned int i;
  xbt_dynar_foreach(p_writeActions, i, _write_action) {
	write_action = dynamic_cast<StorageActionLmmPtr>(static_cast<ActionPtr>(_write_action));
    if ((write_action->p_file) == fd) {
      xbt_dynar_cursor_rm(p_writeActions, &i);
      write_action->unref();
    }
  }
  free(fd->name);
  free(fd->mount);
  xbt_free(fd);
  StorageActionLmmPtr action = new StorageActionLmm(p_model, 0, p_stateCurrent != SURF_RESOURCE_ON, this, CLOSE);
  return action;
}

StorageActionPtr StorageLmm::read(surf_file_t fd, sg_size_t size)
{
  if(size > fd->size)
    size = fd->size;
  StorageActionLmmPtr action = new StorageActionLmm(p_model, size, p_stateCurrent != SURF_RESOURCE_ON, this, READ);
  return action;
}

StorageActionPtr StorageLmm::write(surf_file_t fd, sg_size_t size)
{
  char *filename = fd->name;
  XBT_DEBUG("\tWrite file '%s' size '%llu/%llu'",filename,size,fd->size);

  StorageActionLmmPtr action = new StorageActionLmm(p_model, size, p_stateCurrent != SURF_RESOURCE_ON, this, WRITE);
  action->p_file = fd;

  // If the storage is full
  if(m_usedSize==m_size) {
    action->setState(SURF_ACTION_FAILED);
  }
  return action;
}

void StorageLmm::rename(const char *src, const char *dest)
{
  sg_size_t *psize, *new_psize;
  psize = (sg_size_t*) xbt_dict_get_or_null(p_content,src);
  new_psize = xbt_new(sg_size_t, 1);
  *new_psize = *psize;
  if (psize){// src file exists
    xbt_dict_remove(p_content, src);
    xbt_dict_set(p_content, dest, new_psize,NULL);
    XBT_DEBUG("Change file name from %s to %s, size '%llu'",src, dest, *psize);
  }
  else
    XBT_DEBUG("File %s doesn't exist",src);
}

xbt_dict_t StorageLmm::getContent()
{
  /* For the moment this action has no cost, but in the future we could take in account access latency of the disk */
  /*surf_action_t action = storage_action_execute(storage,0, LS);*/

  xbt_dict_t content_dict = xbt_dict_new_homogeneous(NULL);
  xbt_dict_cursor_t cursor = NULL;
  char *file;
  sg_size_t *psize;

  xbt_dict_foreach(p_content, cursor, file, psize){
    xbt_dict_set(content_dict,file,psize,NULL);
  }
  return content_dict;
}

sg_size_t StorageLmm::getSize(){
  return m_size;
}

/**********
 * Action *
 **********/

StorageActionLmm::StorageActionLmm(ModelPtr model, double cost, bool failed, StorageLmmPtr storage, e_surf_action_storage_type_t type)
  : Action(model, cost, failed),
    ActionLmm(model, cost, failed),
    StorageAction(model, cost, failed, storage, type) {
  XBT_IN("(%s,%g", storage->m_name, cost);
  p_variable = lmm_variable_new(p_model->p_maxminSystem, this, 1.0, -1.0 , 3);

  // Must be less than the max bandwidth for all actions
  lmm_expand(p_model->p_maxminSystem, storage->p_constraint, p_variable, 1.0);
  switch(type) {
  case OPEN:
  case CLOSE:
  case STAT:
  case LS:
    break;
  case READ:
    lmm_expand(p_model->p_maxminSystem, storage->p_constraintRead,
               p_variable, 1.0);
    break;
  case WRITE:
    lmm_expand(p_model->p_maxminSystem, storage->p_constraintWrite,
               p_variable, 1.0);
    ActionPtr action = this;
    xbt_dynar_push(storage->p_writeActions, &action);
    ref();
    break;
  }
  XBT_OUT();
}

int StorageActionLmm::unref()
{
  m_refcount--;
  if (!m_refcount) {
    xbt_swag_remove(static_cast<ActionPtr>(this), p_stateSet);
    if (p_variable)
      lmm_variable_free(p_model->p_maxminSystem, p_variable);
#ifdef HAVE_TRACING
    xbt_free(p_category);
#endif
    delete this;
    return 1;
  }
  return 0;
}

void StorageActionLmm::cancel()
{
  setState(SURF_ACTION_FAILED);
  return;
}

void StorageActionLmm::suspend()
{
  XBT_IN("(%p)", this);
  if (m_suspended != 2) {
    lmm_update_variable_weight(p_model->p_maxminSystem,
                               p_variable,
                               0.0);
    m_suspended = 1;
  }
  XBT_OUT();
}

void StorageActionLmm::resume()
{
  THROW_UNIMPLEMENTED;
}

bool StorageActionLmm::isSuspended()
{
  return m_suspended == 1;
}

void StorageActionLmm::setMaxDuration(double /*duration*/)
{
  THROW_UNIMPLEMENTED;
}

void StorageActionLmm::setPriority(double /*priority*/)
{
  THROW_UNIMPLEMENTED;
}

