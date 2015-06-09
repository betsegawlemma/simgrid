/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_PROCESS_H
#define MC_PROCESS_H

#include <type_traits>

#include <sys/types.h>

#include <vector>
#include <memory>

#include "simgrid_config.h"
#include <sys/types.h>

#include <xbt/mmalloc.h>

#ifdef HAVE_MC
#include "xbt/mmalloc/mmprivate.h"
#endif

#include <simgrid/simix.h>
#include "simix/popping_private.h"
#include "simix/smx_private.h"

#include "mc_forward.h"
#include "mc_base.h"
#include "mc_mmalloc.h" // std_heap
#include "mc_memory_map.h"
#include "AddressSpace.hpp"
#include "mc_protocol.h"

typedef int mc_process_flags_t;
#define MC_PROCESS_NO_FLAG 0
#define MC_PROCESS_SELF_FLAG 1

// Those flags are used to track down which cached information
// is still up to date and which information needs to be updated.
typedef int mc_process_cache_flags_t;
#define MC_PROCESS_CACHE_FLAG_NONE 0
#define MC_PROCESS_CACHE_FLAG_HEAP 1
#define MC_PROCESS_CACHE_FLAG_MALLOC_INFO 2
#define MC_PROCESS_CACHE_FLAG_SIMIX_PROCESSES 4

typedef struct s_mc_smx_process_info s_mc_smx_process_info_t, *mc_smx_process_info_t;

namespace simgrid {
namespace mc {

struct IgnoredRegion {
  std::uint64_t addr;
  size_t size;
};

/** Representation of a process
 */
class Process : public AddressSpace {
public:
  Process(pid_t pid, int sockfd);
  ~Process();

  bool is_self() const
  {
    return this->process_flags & MC_PROCESS_SELF_FLAG;
  }

  // Read memory:
  const void* read_bytes(void* buffer, std::size_t size,
    remote_ptr<void> address, int process_index = ProcessIndexAny,
    ReadMode mode = Normal) const MC_OVERRIDE;
  void read_variable(const char* name, void* target, size_t size) const;
  template<class T>
  T read_variable(const char *name) const
  {
    static_assert(std::is_trivial<T>::value, "Cannot read a non-trivial type");
    T res;
    read_variable(name, &res, sizeof(T));
    return res;
  }
  char* read_string(remote_ptr<void> address) const;

  // Write memory:
  void write_bytes(const void* buffer, size_t len, remote_ptr<void> address);
  void clear_bytes(remote_ptr<void> address, size_t len);

  // Debug information:
  std::shared_ptr<s_mc_object_info_t> find_object_info(remote_ptr<void> addr) const;
  std::shared_ptr<s_mc_object_info_t> find_object_info_exec(remote_ptr<void> addr) const;
  std::shared_ptr<s_mc_object_info_t> find_object_info_rw(remote_ptr<void> addr) const;
  dw_frame_t find_function(remote_ptr<void> ip) const;
  dw_variable_t find_variable(const char* name) const;

  // Heap access:
  xbt_mheap_t get_heap()
  {
    if (!(this->cache_flags & MC_PROCESS_CACHE_FLAG_HEAP))
      this->refresh_heap();
    return this->heap;
  }
  malloc_info* get_malloc_info()
  {
    if (!(this->cache_flags & MC_PROCESS_CACHE_FLAG_MALLOC_INFO))
      this->refresh_malloc_info();
    return this->heap_info;
  }

  std::vector<IgnoredRegion> const& ignored_regions() const
  {
    return ignored_regions_;
  }
  void ignore_region(std::uint64_t address, std::size_t size);

  pid_t pid() const { return pid_; }

  bool in_maestro_stack(remote_ptr<void> p) const
  {
    return p >= this->maestro_stack_start_ && p < this->maestro_stack_end_;
  }

  bool running() const
  {
    return running_;
  }

  void terminate(int status)
  {
    status_ = status;
    running_ = false;
  }

  int status() const
  {
    return status_;
  }

  template<class M>
  typename std::enable_if< std::is_class<M>::value && std::is_trivial<M>::value, int >::type
  send_message(M const& m)
  {
    return MC_protocol_send(this->socket_, &m, sizeof(M));
  }

  int send_message(e_mc_message_type message_id)
  {
    return MC_protocol_send_simple_message(this->socket_, message_id);
  }

  template<class M>
  typename std::enable_if< std::is_class<M>::value && std::is_trivial<M>::value, ssize_t >::type
  receive_message(M& m)
  {
    return MC_receive_message(this->socket_, &m, sizeof(M), 0);
  }

private:
  void init_memory_map_info();
  void refresh_heap();
  void refresh_malloc_info();
private:
  mc_process_flags_t process_flags;
  pid_t pid_;
  int socket_;
  int status_;
  bool running_;
  std::vector<VmMap> memory_map_;
  remote_ptr<void> maestro_stack_start_, maestro_stack_end_;
  int memory_file;
  std::vector<IgnoredRegion> ignored_regions_;

public: // object info
  // TODO, make private (first, objectify mc_object_info_t)
  std::vector<std::shared_ptr<s_mc_object_info_t>> object_infos;
  std::shared_ptr<s_mc_object_info_t> libsimgrid_info;
  std::shared_ptr<s_mc_object_info_t> binary_info;

public: // Copies of MCed SMX data structures
  /** Copy of `simix_global->process_list`
   *
   *  See mc_smx.c.
   */
  xbt_dynar_t smx_process_infos;

  /** Copy of `simix_global->process_to_destroy`
   *
   *  See mc_smx.c.
   */
  xbt_dynar_t smx_old_process_infos;

  /** State of the cache (which variables are up to date) */
  mc_process_cache_flags_t cache_flags;

  /** Address of the heap structure in the MCed process. */
  void* heap_address;

  /** Copy of the heap structure of the process
   *
   *  This is refreshed with the `MC_process_refresh` call.
   *  This is not used if the process is the current one:
   *  use `get_heap_info()` in order to use it.
   */
   xbt_mheap_t heap;

  /** Copy of the allocation info structure
   *
   *  This is refreshed with the `MC_process_refresh` call.
   *  This is not used if the process is the current one:
   *  use `get_malloc_info()` in order to use it.
   */
  malloc_info* heap_info;

public: // Libunwind-data

  /** Full-featured MC-aware libunwind address space for the process
   *
   *  This address space is using a mc_unw_context_t
   *  (with mc_process_t/mc_address_space_t and unw_context_t).
   */
  unw_addr_space_t unw_addr_space;

  /** Underlying libunwind addres-space
   *
   *  The `find_proc_info`, `put_unwind_info`, `get_dyn_info_list_addr`
   *  operations of the native MC address space is currently delegated
   *  to this address space (either the local or a ptrace unwinder).
   */
  unw_addr_space_t unw_underlying_addr_space;

  /** The corresponding context
   */
  void* unw_underlying_context;
};

/** Open a FD to a remote process memory (`/dev/$pid/mem`)
 */
int open_vm(pid_t pid, int flags);

}
}

SG_BEGIN_DECL()

XBT_INTERNAL void MC_invalidate_cache(void);

SG_END_DECL()

#endif
