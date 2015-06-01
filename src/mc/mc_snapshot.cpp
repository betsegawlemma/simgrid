/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdbool.h>

#include "internal_config.h"
#include "smpi/private.h"

#include "mc_snapshot.h"
#include "mc_private.h"
#include "mc_mmu.h"
#include "PageStore.hpp"

extern "C" {

/** @brief Find the snapshoted region from a pointer
 *
 *  @param addr     Pointer
 *  @param snapshot Snapshot
 *  @param Snapshot region in the snapshot this pointer belongs to
 *         (or NULL if it does not belong to any snapshot region)
 * */
mc_mem_region_t mc_get_snapshot_region(
  const void* addr, const s_mc_snapshot_t* snapshot, int process_index)
{
  size_t n = snapshot->snapshot_regions_count;
  for (size_t i = 0; i != n; ++i) {
    mc_mem_region_t region = snapshot->snapshot_regions[i];
    if (!(region && mc_region_contain(region, addr)))
      continue;

    if (region->storage_type == MC_REGION_STORAGE_TYPE_PRIVATIZED) {
#ifdef HAVE_SMPI
      // Use the current process index of the snapshot:
      if (process_index == simgrid::mc::ProcessIndexDisabled) {
        process_index = snapshot->privatization_index;
      }
      if (process_index < 0) {
        xbt_die("Missing process index");
      }
      if (process_index >= (int) region->privatized_regions_.size()) {
        xbt_die("Invalid process index");
      }
      mc_mem_region_t priv_region = region->privatized_regions_[process_index].get();
      xbt_assert(mc_region_contain(priv_region, addr));
      return priv_region;
#else
      xbt_die("Privatized region in a non SMPI build (this should not happen)");
#endif
    }

    return region;
  }

  return NULL;
}

/** @brief Read memory from a snapshot region broken across fragmented pages
 *
 *  @param addr    Process (non-snapshot) address of the data
 *  @param region  Snapshot memory region where the data is located
 *  @param target  Buffer to store the value
 *  @param size    Size of the data to read in bytes
 *  @return Pointer where the data is located (target buffer of original location)
 */
const void* MC_region_read_fragmented(mc_mem_region_t region, void* target, const void* addr, size_t size)
{
  // Last byte of the memory area:
  void* end = (char*) addr + size - 1;

  // Page of the last byte of the memory area:
  size_t page_end = mc_page_number(NULL, end);

  void* dest = target;

  if (dest==NULL) {
    xbt_die("Missing destination buffer for fragmented memory access");
  }

  // Read each page:
  while (mc_page_number(NULL, addr) != page_end) {
    void* snapshot_addr = mc_translate_address_region_chunked((uintptr_t) addr, region);
    void* next_page = mc_page_from_number(NULL, mc_page_number(NULL, addr) + 1);
    size_t readable = (char*) next_page - (char*) addr;
    memcpy(dest, snapshot_addr, readable);
    addr = (char*) addr + readable;
    dest = (char*) dest + readable;
    size -= readable;
  }

  // Read the end:
  void* snapshot_addr = mc_translate_address_region_chunked((uintptr_t)addr, region);
  memcpy(dest, snapshot_addr, size);

  return target;
}

/** Compare memory between snapshots (with known regions)
 *
 * @param addr1 Address in the first snapshot
 * @param snapshot2 Region of the address in the first snapshot
 * @param addr2 Address in the second snapshot
 * @param snapshot2 Region of the address in the second snapshot
 * @return same as memcmp
 * */
int MC_snapshot_region_memcmp(
  const void* addr1, mc_mem_region_t region1,
  const void* addr2, mc_mem_region_t region2,
  size_t size)
{
  // Using alloca() for large allocations may trigger stack overflow:
  // use malloc if the buffer is too big.
  bool stack_alloc = size < 64;
  const bool region1_need_buffer = region1==NULL || region1->storage_type==MC_REGION_STORAGE_TYPE_FLAT;
  const bool region2_need_buffer = region2==NULL || region2->storage_type==MC_REGION_STORAGE_TYPE_FLAT;
  void* buffer1a = region1_need_buffer ? NULL : stack_alloc ? alloca(size) : malloc(size);
  void* buffer2a = region2_need_buffer ? NULL : stack_alloc ? alloca(size) : malloc(size);
  const void* buffer1 = MC_region_read(region1, buffer1a, addr1, size);
  const void* buffer2 = MC_region_read(region2, buffer2a, addr2, size);
  int res;
  if (buffer1 == buffer2) {
    res = 0;
  } else {
    res = memcmp(buffer1, buffer2, size);
  }
  if (!stack_alloc) {
    free(buffer1a);
    free(buffer2a);
  }
  return res;
}

/** Compare memory between snapshots
 *
 * @param addr1 Address in the first snapshot
 * @param snapshot1 First snapshot
 * @param addr2 Address in the second snapshot
 * @param snapshot2 Second snapshot
 * @return same as memcmp
 * */
int MC_snapshot_memcmp(
  const void* addr1, mc_snapshot_t snapshot1,
  const void* addr2, mc_snapshot_t snapshot2, int process_index, size_t size)
{
  mc_mem_region_t region1 = mc_get_snapshot_region(addr1, snapshot1, process_index);
  mc_mem_region_t region2 = mc_get_snapshot_region(addr2, snapshot2, process_index);
  return MC_snapshot_region_memcmp(addr1, region1, addr2, region2, size);
}

namespace simgrid {
namespace mc {

Snapshot::Snapshot() :
  process(nullptr),
  num_state(0),
  heap_bytes_used(0),
  snapshot_regions(nullptr),
  snapshot_regions_count(0),
  enabled_processes(0),
  privatization_index(0),
  stack_sizes(nullptr),
  stacks(nullptr),
  to_ignore(nullptr),
  hash(0),
  ignored_data(nullptr),
  total_fd(0),
  current_fd(nullptr)
{

}
Snapshot::~Snapshot()
{
  for (size_t i = 0; i < this->snapshot_regions_count; i++) {
    delete this->snapshot_regions[i];
  }
  xbt_free(this->snapshot_regions);
  xbt_free(this->stack_sizes);
  xbt_dynar_free(&(this->stacks));
  xbt_dynar_free(&(this->to_ignore));
  xbt_dynar_free(&this->ignored_data);
}

const void* Snapshot::read_bytes(void* buffer, std::size_t size,
  remote_ptr<void> address, int process_index,
  AddressSpace::ReadMode mode) const
{
  mc_mem_region_t region = mc_get_snapshot_region((void*)address.address(), this, process_index);
  if (region) {
    const void* res = MC_region_read(region, buffer, (void*)address.address(), size);
    if (buffer == res || mode == AddressSpace::Lazy)
      return res;
    else {
      memcpy(buffer, res, size);
      return buffer;
    }
  }
  else
    return this->read_bytes(buffer, size, address, process_index, mode);
}

}
}

#ifdef SIMGRID_TEST

#include <string.h>
#include <stdlib.h>

#include <sys/mman.h>

#include "mc/mc_private.h"
#include "mc/mc_snapshot.h"
#include "mc/mc_mmu.h"

extern "C" {

XBT_TEST_SUITE("mc_snapshot", "Snapshots");

static inline void init_memory(void* mem, size_t size)
{
  char* dest = (char*) mem;
  for (size_t i = 0; i < size; ++i) {
    dest[i] = rand() & 255;
  }
}

static void test_snapshot(bool sparse_checkpoint);

XBT_TEST_UNIT("flat_snapshot", test_flat_snapshots, "Test flat snapshots")
{
  test_snapshot(0);
}

XBT_TEST_UNIT("page_snapshots", test_per_snpashots, "Test per-page snapshots")
{
  test_snapshot(1);
}

static void test_snapshot(bool sparse_checkpoint) {

  xbt_test_add("Initialisation");
  _sg_mc_sparse_checkpoint = sparse_checkpoint;
  xbt_assert(xbt_pagesize == getpagesize());
  xbt_assert(1 << xbt_pagebits == xbt_pagesize);
  mc_model_checker = new ::simgrid::mc::ModelChecker(getpid(), -1);

  for(int n=1; n!=256; ++n) {

    // Store region page(s):
    size_t byte_size = n * xbt_pagesize;
    void* source = mmap(NULL, byte_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    xbt_assert(source!=MAP_FAILED, "Could not allocate source memory");

    // Init memory and take snapshots:
    init_memory(source, byte_size);
    mc_mem_region_t region0 = mc_region_new_sparse(
      MC_REGION_TYPE_UNKNOWN, source, source, byte_size);
    for(int i=0; i<n; i+=2) {
      init_memory((char*) source + i*xbt_pagesize, xbt_pagesize);
    }
    mc_mem_region_t region = mc_region_new_sparse(
      MC_REGION_TYPE_UNKNOWN, source, source, byte_size);

    void* destination = mmap(NULL, byte_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    xbt_assert(source!=MAP_FAILED, "Could not allocate destination memory");

    xbt_test_add("Reading whole region data for %i page(s)", n);
    const void* read = MC_region_read(region, destination, source, byte_size);
    xbt_test_assert(!memcmp(source, read, byte_size), "Mismatch in MC_region_read()");

    xbt_test_add("Reading parts of region data for %i page(s)", n);
    for(int j=0; j!=100; ++j) {
      size_t offset = rand() % byte_size;
      size_t size = rand() % (byte_size - offset);
      const void* read = MC_region_read(region, destination, (const char*) source+offset, size);
      xbt_test_assert(!memcmp((char*) source+offset, read, size),
        "Mismatch in MC_region_read()");
    }

    xbt_test_add("Compare whole region data for %i page(s)", n);

    xbt_test_assert(MC_snapshot_region_memcmp(source, region0, source, region, byte_size),
      "Unexpected match in MC_snapshot_region_memcmp() with previous snapshot");

    xbt_test_add("Compare parts of region data for %i page(s) with itself", n);
    for(int j=0; j!=100; ++j) {
      size_t offset = rand() % byte_size;
      size_t size = rand() % (byte_size - offset);
      xbt_test_assert(!MC_snapshot_region_memcmp((char*) source+offset, region, (char*) source+offset, region, size),
        "Mismatch in MC_snapshot_region_memcmp()");
    }

    if (n==1) {
      xbt_test_add("Read pointer for %i page(s)", n);
      memcpy(source, &mc_model_checker, sizeof(void*));
      mc_mem_region_t region2 = mc_region_new_sparse(
        MC_REGION_TYPE_UNKNOWN, source, source, byte_size);
      xbt_test_assert(MC_region_read_pointer(region2, source) == mc_model_checker,
        "Mismtach in MC_region_read_pointer()");
      delete region2;
    }

    delete region;
    delete region0;
    munmap(destination, byte_size);
    munmap(source, byte_size);
  }

  delete mc_model_checker;
  mc_model_checker = NULL;
}

}

#endif /* SIMGRID_TEST */

}
