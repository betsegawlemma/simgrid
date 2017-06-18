/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../surf/StorageImpl.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "src/msg/msg_private.h"
#include <numeric>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_io, msg, "Logging specific to MSG (io)");

SG_BEGIN_DECL()

/** @addtogroup msg_file
 * (#msg_file_t) and the functions for managing it.
 *
 *  \see #msg_file_t
 */

/********************************* File **************************************/
void __MSG_file_get_info(msg_file_t fd){

  xbt_dynar_t info = simcall_file_get_info(fd->simdata->smx_file);
  sg_size_t *psize;

  fd->storage_type = xbt_dynar_pop_as(info, char*);
  fd->storageId    = xbt_dynar_pop_as(info, char*);
  fd->mount_point  = xbt_dynar_pop_as(info, char*);
  psize            = xbt_dynar_pop_as(info, sg_size_t*);
  fd->size         = *psize;
  xbt_free(psize);
  xbt_dynar_free_container(&info);
}

static int MSG_host_get_file_descriptor_id(msg_host_t host)
{
  simgrid::MsgHostExt* priv = host->extension<simgrid::MsgHostExt>();
  if (priv->file_descriptor_table == nullptr) {
    priv->file_descriptor_table = new std::vector<int>(sg_storage_max_file_descriptors);
    std::iota(priv->file_descriptor_table->rbegin(), priv->file_descriptor_table->rend(), 0); // Fill with ..., 1, 0.
  }
  xbt_assert(not priv->file_descriptor_table->empty(), "Too much files are opened! Some have to be closed.");
  int desc = priv->file_descriptor_table->back();
  priv->file_descriptor_table->pop_back();
  return desc;
}

static void MSG_host_release_file_descriptor_id(msg_host_t host, int id)
{
  host->extension<simgrid::MsgHostExt>()->file_descriptor_table->push_back(id);
}

/** \ingroup msg_file
 *
 * \brief Set the user data of a #msg_file_t.
 *
 * This functions attach \a data to \a file.
 */
msg_error_t MSG_file_set_data(msg_file_t fd, void *data)
{
  fd->data = data;
  return MSG_OK;
}

/** \ingroup msg_file
 *
 * \brief Return the user data of a #msg_file_t.
 *
 * This functions checks whether \a file is a valid pointer and return the user data associated to \a file if possible.
 */
void *MSG_file_get_data(msg_file_t fd)
{
  return fd->data;
}

/** \ingroup msg_file
 * \brief Display information related to a file descriptor
 *
 * \param fd is a the file descriptor
 */
void MSG_file_dump (msg_file_t fd){
  /* Update the cached information first */
  __MSG_file_get_info(fd);

  XBT_INFO("File Descriptor information:\n"
           "\t\tFull path: '%s'\n"
           "\t\tSize: %llu\n"
           "\t\tMount point: '%s'\n"
           "\t\tStorage Id: '%s'\n"
           "\t\tStorage Type: '%s'\n"
           "\t\tFile Descriptor Id: %d",
           fd->fullpath, fd->size, fd->mount_point, fd->storageId, fd->storage_type, fd->desc_id);
}

/** \ingroup msg_file
 * \brief Read a file (local or remote)
 *
 * \param size of the file to read
 * \param fd is a the file descriptor
 * \return the number of bytes successfully read or -1 if an error occurred
 */
sg_size_t MSG_file_read(msg_file_t fd, sg_size_t size)
{
  sg_size_t read_size;

  if (fd->size == 0) /* Nothing to read, return */
    return 0;

  /* Find the host where the file is physically located and read it */
  msg_storage_t storage_src           = simgrid::s4u::Storage::byName(fd->storageId);
  msg_host_t attached_host            = MSG_host_by_name(storage_src->host());
  read_size                           = simcall_file_read(fd->simdata->smx_file, size, attached_host);

  if (strcmp(storage_src->host(), MSG_host_self()->cname())) {
    /* the file is hosted on a remote host, initiate a communication between src and dest hosts for data transfer */
    XBT_DEBUG("File is on %s remote host, initiate data transfer of %llu bytes.", storage_src->host(), read_size);
    msg_host_t m_host_list[] = {MSG_host_self(), attached_host};
    double flops_amount[]    = {0, 0};
    double bytes_amount[]    = {0, 0, static_cast<double>(read_size), 0};

    msg_task_t task = MSG_parallel_task_create("file transfer for read", 2, m_host_list, flops_amount, bytes_amount,
                      nullptr);
    msg_error_t transfer = MSG_parallel_task_execute(task);
    MSG_task_destroy(task);

    if(transfer != MSG_OK){
      if (transfer == MSG_HOST_FAILURE)
        XBT_WARN("Transfer error, %s remote host just turned off!", attached_host->cname());
      if (transfer == MSG_TASK_CANCELED)
        XBT_WARN("Transfer error, task has been canceled!");

      return -1;
    }
  }
  return read_size;
}

/** \ingroup msg_file
 * \brief Write into a file (local or remote)
 *
 * \param size of the file to write
 * \param fd is a the file descriptor
 * \return the number of bytes successfully write or -1 if an error occurred
 */
sg_size_t MSG_file_write(msg_file_t fd, sg_size_t size)
{
  if (size == 0) /* Nothing to write, return */
    return 0;

  /* Find the host where the file is physically located (remote or local)*/
  msg_storage_t storage_src = simgrid::s4u::Storage::byName(fd->storageId);
  msg_host_t attached_host  = MSG_host_by_name(storage_src->host());

  if (strcmp(storage_src->host(), MSG_host_self()->cname())) {
    /* the file is hosted on a remote host, initiate a communication between src and dest hosts for data transfer */
    XBT_DEBUG("File is on %s remote host, initiate data transfer of %llu bytes.", storage_src->host(), size);
    msg_host_t m_host_list[] = {MSG_host_self(), attached_host};
    double flops_amount[]    = {0, 0};
    double bytes_amount[]    = {0, static_cast<double>(size), 0, 0};

    msg_task_t task = MSG_parallel_task_create("file transfer for write", 2, m_host_list, flops_amount, bytes_amount,
                                               nullptr);
    msg_error_t transfer = MSG_parallel_task_execute(task);
    MSG_task_destroy(task);

    if(transfer != MSG_OK){
      if (transfer == MSG_HOST_FAILURE)
        XBT_WARN("Transfer error, %s remote host just turned off!", attached_host->cname());
      if (transfer == MSG_TASK_CANCELED)
        XBT_WARN("Transfer error, task has been canceled!");

      return -1;
    }
  }
  /* Write file on local or remote host */
  sg_size_t offset     = simcall_file_tell(fd->simdata->smx_file);
  sg_size_t write_size = simcall_file_write(fd->simdata->smx_file, size, attached_host);
  fd->size             = offset + write_size;

  return write_size;
}

/** \ingroup msg_file
 * \brief Opens the file whose name is the string pointed to by path
 *
 * \param fullpath is the file location on the storage
 * \param data user data to attach to the file
 *
 * \return An #msg_file_t associated to the file
 */
msg_file_t MSG_file_open(const char* fullpath, void* data)
{
  msg_file_t fd         = xbt_new(s_msg_file_priv_t, 1);
  fd->data              = data;
  fd->fullpath          = xbt_strdup(fullpath);
  fd->simdata           = xbt_new0(s_simdata_file_t, 1);
  fd->simdata->smx_file = simcall_file_open(fullpath, MSG_host_self());
  fd->desc_id           = MSG_host_get_file_descriptor_id(MSG_host_self());

  __MSG_file_get_info(fd);

  return fd;
}

/** \ingroup msg_file
 * \brief Close the file
 *
 * \param fd is the file to close
 * \return 0 on success or 1 on error
 */
int MSG_file_close(msg_file_t fd)
{
  if (fd->data)
    xbt_free(fd->data);

  int res = simcall_file_close(fd->simdata->smx_file, MSG_host_self());
  MSG_host_release_file_descriptor_id(MSG_host_self(), fd->desc_id);
  __MSG_file_destroy(fd);

  return res;
}

/** \ingroup msg_file
 * \brief Unlink the file pointed by fd
 *
 * \param fd is the file descriptor (#msg_file_t)
 * \return 0 on success or 1 on error
 */
msg_error_t MSG_file_unlink(msg_file_t fd)
{
  /* Find the host where the file is physically located (remote or local)*/
  msg_storage_t storage_src           = simgrid::s4u::Storage::byName(fd->storageId);
  msg_host_t attached_host            = MSG_host_by_name(storage_src->host());
  int res                             = simcall_file_unlink(fd->simdata->smx_file, attached_host);
  __MSG_file_destroy(fd);
  return static_cast<msg_error_t>(res);
}

/** \ingroup msg_file
 * \brief Return the size of a file
 *
 * \param fd is the file descriptor (#msg_file_t)
 * \return the size of the file (as a #sg_size_t)
 */
sg_size_t MSG_file_get_size(msg_file_t fd){
  return simcall_file_get_size(fd->simdata->smx_file);
}

/**
 * \ingroup msg_file
 * \brief Set the file position indicator in the msg_file_t by adding offset bytes
 * to the position specified by origin (either SEEK_SET, SEEK_CUR, or SEEK_END).
 *
 * \param fd : file object that identifies the stream
 * \param offset : number of bytes to offset from origin
 * \param origin : Position used as reference for the offset. It is specified by one of the following constants defined
 *                 in \<stdio.h\> exclusively to be used as arguments for this function (SEEK_SET = beginning of file,
 *                 SEEK_CUR = current position of the file pointer, SEEK_END = end of file)
 * \return If successful, the function returns MSG_OK (=0). Otherwise, it returns MSG_TASK_CANCELED (=8).
 */
msg_error_t MSG_file_seek(msg_file_t fd, sg_offset_t offset, int origin)
{
  return static_cast<msg_error_t>(simcall_file_seek(fd->simdata->smx_file, offset, origin));
}

/**
 * \ingroup msg_file
 * \brief Returns the current value of the position indicator of the file
 *
 * \param fd : file object that identifies the stream
 * \return On success, the current value of the position indicator is returned.
 *
 */
sg_size_t MSG_file_tell(msg_file_t fd)
{
  return simcall_file_tell(fd->simdata->smx_file);
}

const char *MSG_file_get_name(msg_file_t fd) {
  xbt_assert((fd != nullptr), "Invalid parameters");
  return fd->fullpath;
}

/**
 * \ingroup msg_file
 * \brief Move a file to another location on the *same mount point*.
 *
 */
msg_error_t MSG_file_move (msg_file_t fd, const char* fullpath)
{
  return static_cast<msg_error_t>(simcall_file_move(fd->simdata->smx_file, fullpath));
}

/**
 * \ingroup msg_file
 * \brief Copy a file to another location on a remote host.
 * \param file : the file to move
 * \param host : the remote host where the file has to be copied
 * \param fullpath : the complete path destination on the remote host
 * \return If successful, the function returns MSG_OK. Otherwise, it returns MSG_TASK_CANCELED.
 */
msg_error_t MSG_file_rcopy (msg_file_t file, msg_host_t host, const char* fullpath)
{
  /* Find the host where the file is physically located and read it */
  msg_storage_t storage_src = simgrid::s4u::Storage::byName(file->storageId);
  msg_host_t attached_host  = MSG_host_by_name(storage_src->host());
  MSG_file_seek(file, 0, SEEK_SET);
  sg_size_t read_size = simcall_file_read(file->simdata->smx_file, file->size, attached_host);

  /* Find the real host destination where the file will be physically stored */
  xbt_dict_cursor_t cursor   = nullptr;
  msg_storage_t storage_dest = nullptr;
  msg_host_t host_dest;
  size_t longest_prefix_length = 0;

  xbt_dict_t storage_list = host->mountedStoragesAsDict();
  char *mount_name;
  char *storage_name;
  xbt_dict_foreach(storage_list,cursor,mount_name,storage_name){
    char* file_mount_name = static_cast<char*>(xbt_malloc(strlen(mount_name) + 1));
    strncpy(file_mount_name, fullpath, strlen(mount_name) + 1);
    file_mount_name[strlen(mount_name)] = '\0';

    if (not strcmp(file_mount_name, mount_name) && strlen(mount_name) > longest_prefix_length) {
      /* The current mount name is found in the full path and is bigger than the previous*/
      longest_prefix_length = strlen(mount_name);
      storage_dest          = simgrid::s4u::Storage::byName(storage_name);
    }
    xbt_free(file_mount_name);
  }
  xbt_dict_free(&storage_list);

  if(longest_prefix_length>0){
    /* Mount point found, retrieve the host the storage is attached to */
    host_dest = MSG_host_by_name(storage_dest->host());
  }else{
    XBT_WARN("Can't find mount point for '%s' on destination host '%s'", fullpath, host->cname());
    return MSG_TASK_CANCELED;
  }

  XBT_DEBUG("Initiate data transfer of %llu bytes between %s and %s.", read_size, storage_src->host(),
            storage_dest->host());
  msg_host_t m_host_list[] = {attached_host, host_dest};
  double flops_amount[]    = {0, 0};
  double bytes_amount[]    = {0, static_cast<double>(read_size), 0, 0};

  msg_task_t task =
      MSG_parallel_task_create("file transfer for write", 2, m_host_list, flops_amount, bytes_amount, nullptr);
  msg_error_t transfer = MSG_parallel_task_execute(task);
  MSG_task_destroy(task);

  if(transfer != MSG_OK){
    if (transfer == MSG_HOST_FAILURE)
      XBT_WARN("Transfer error, %s remote host just turned off!", storage_dest->host());
    if (transfer == MSG_TASK_CANCELED)
      XBT_WARN("Transfer error, task has been canceled!");

    return transfer;
  }

  /* Create file on remote host, write it and close it */
  smx_file_t smx_file = simcall_file_open(fullpath, host_dest);
  simcall_file_write(smx_file, read_size, host_dest);
  simcall_file_close(smx_file, host_dest);
  return MSG_OK;
}

/**
 * \ingroup msg_file
 * \brief Move a file to another location on a remote host.
 * \param file : the file to move
 * \param host : the remote host where the file has to be moved
 * \param fullpath : the complete path destination on the remote host
 * \return If successful, the function returns MSG_OK. Otherwise, it returns MSG_TASK_CANCELED.
 */
msg_error_t MSG_file_rmove (msg_file_t file, msg_host_t host, const char* fullpath)
{
  msg_error_t res = MSG_file_rcopy(file, host, fullpath);
  MSG_file_unlink(file);
  return res;
}

/**
 * \brief Destroys a file (internal call only)
 */
void __MSG_file_destroy(msg_file_t file)
{
  xbt_free(file->fullpath);
  xbt_free(file->simdata);
  xbt_free(file);
}

/********************************* Storage **************************************/
/** @addtogroup msg_storage_management
 * (#msg_storage_t) and the functions for managing it.
 */

/** \ingroup msg_storage_management
 *
 * \brief Returns the name of the #msg_storage_t.
 *
 * This functions checks whether a storage is a valid pointer or not and return its name.
 */
const char *MSG_storage_get_name(msg_storage_t storage) {
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->name();
}

/** \ingroup msg_storage_management
 * \brief Returns the free space size of a storage element
 * \param storage a storage
 * \return the free space size of the storage element (as a #sg_size_t)
 */
sg_size_t MSG_storage_get_free_size(msg_storage_t storage){
  return storage->sizeFree();
}

/** \ingroup msg_storage_management
 * \brief Returns the used space size of a storage element
 * \param storage a storage
 * \return the used space size of the storage element (as a #sg_size_t)
 */
sg_size_t MSG_storage_get_used_size(msg_storage_t storage){
  return storage->sizeUsed();
}

/** \ingroup msg_storage_management
 * \brief Returns a xbt_dict_t consisting of the list of properties assigned to this storage
 * \param storage a storage
 * \return a dict containing the properties
 */
xbt_dict_t MSG_storage_get_properties(msg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters (storage is nullptr)");
  return storage->properties();
}

/** \ingroup msg_storage_management
 * \brief Change the value of a given storage property
 *
 * \param storage a storage
 * \param name a property name
 * \param value what to change the property to
 */
void MSG_storage_set_property_value(msg_storage_t storage, const char* name, char* value)
{
  storage->setProperty(name, value);
}

/** \ingroup m_storage_management
 * \brief Returns the value of a given storage property
 *
 * \param storage a storage
 * \param name a property name
 * \return value of a property (or nullptr if property not set)
 */
const char *MSG_storage_get_property_value(msg_storage_t storage, const char *name)
{
  return storage->property(name);
}

/** \ingroup msg_storage_management
 * \brief Finds a msg_storage_t using its name.
 * \param name the name of a storage
 * \return the corresponding storage
 */
msg_storage_t MSG_storage_get_by_name(const char *name)
{
  return simgrid::s4u::Storage::byName(name);
}

/** \ingroup msg_storage_management
 * \brief Returns a dynar containing all the storage elements declared at a given point of time
 */
xbt_dynar_t MSG_storages_as_dynar() {
  xbt_dynar_t res = xbt_dynar_new(sizeof(msg_storage_t),nullptr);
  for (auto s : *simgrid::s4u::allStorages()) {
    xbt_dynar_push(res, &(s.second));
  }
  return res;
}

/** \ingroup msg_storage_management
 *
 * \brief Set the user data of a #msg_storage_t.
 * This functions attach \a data to \a storage if possible.
 */
msg_error_t MSG_storage_set_data(msg_storage_t storage, void *data)
{
  storage->setUserdata(data);
  return MSG_OK;
}

/** \ingroup m_host_management
 *
 * \brief Returns the user data of a #msg_storage_t.
 *
 * This functions checks whether \a storage is a valid pointer and returns its associate user data if possible.
 */
void *MSG_storage_get_data(msg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->userdata();
}

/** \ingroup msg_storage_management
 *
 * \brief Returns the content (file list) of a #msg_storage_t.
 * \param storage a storage
 * \return The content of this storage element as a dict (full path file => size)
 */
xbt_dict_t MSG_storage_get_content(msg_storage_t storage)
{
  std::map<std::string, sg_size_t*>* content = storage->content();
  xbt_dict_t content_dict = xbt_dict_new_homogeneous(nullptr);

  for (auto entry : *content) {
    xbt_dict_set(content_dict, entry.first.c_str(), entry.second, nullptr);
  }
  return content_dict;
}

/** \ingroup msg_storage_management
 *
 * \brief Returns the size of a #msg_storage_t.
 * \param storage a storage
 * \return The size of the storage
 */
sg_size_t MSG_storage_get_size(msg_storage_t storage)
{
  return storage->size();
}

/** \ingroup msg_storage_management
 *
 * \brief Returns the host name the storage is attached to
 *
 * This functions checks whether a storage is a valid pointer or not and return its name.
 */
const char *MSG_storage_get_host(msg_storage_t storage) {
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->host();
}

SG_END_DECL()
