/**********************************************************************/
/* File generated by src/simix/simcalls.py from src/simix/simcalls.in */
/*                                                                    */
/*                    DO NOT EVER CHANGE THIS FILE                    */
/*                                                                    */
/* change simcalls specification in src/simix/simcalls.in             */
/* Copyright (c) 2014-2017. The SimGrid Team. All rights reserved.    */
/**********************************************************************/

/*
 * Note that the name comes from http://en.wikipedia.org/wiki/Popping
 * Indeed, the control flow is doing a strange dance in there.
 *
 * That's not about http://en.wikipedia.org/wiki/Poop, despite the odor :)
 */

#include <functional>
#include "smx_private.h"
#include "src/mc/mc_forward.hpp"
#include "xbt/ex.h"
#include <simgrid/simix.hpp>
/** @cond */ // Please Doxygen, don't look at this

template<class R, class... T>
inline static R simcall(e_smx_simcall_t call, T const&... t)
{
  smx_actor_t self = SIMIX_process_self();
  simgrid::simix::marshal(&self->simcall, call, t...);
  if (self != simix_global->maestro_process) {
    XBT_DEBUG("Yield process '%s' on simcall %s (%d)", self->name.c_str(),
              SIMIX_simcall_name(self->simcall.call), (int)self->simcall.call);
    SIMIX_process_yield(self);
  } else {
    SIMIX_simcall_handle(&self->simcall, 0);
  }
  return simgrid::simix::unmarshal<R>(self->simcall.result);
}

inline static void simcall_BODY_process_kill(smx_actor_t process) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_process_kill(&SIMIX_process_self()->simcall, process);
    return simcall<void, smx_actor_t>(SIMCALL_PROCESS_KILL, process);
  }

inline static void simcall_BODY_process_killall(int reset_pid) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_process_killall(&SIMIX_process_self()->simcall, reset_pid);
    return simcall<void, int>(SIMCALL_PROCESS_KILLALL, reset_pid);
  }

inline static void simcall_BODY_process_cleanup(smx_actor_t process) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_process_cleanup(process);
    return simcall<void, smx_actor_t>(SIMCALL_PROCESS_CLEANUP, process);
  }

inline static void simcall_BODY_process_suspend(smx_actor_t process) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_process_suspend(&SIMIX_process_self()->simcall, process);
    return simcall<void, smx_actor_t>(SIMCALL_PROCESS_SUSPEND, process);
  }

inline static int simcall_BODY_process_join(smx_actor_t process, double timeout) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_process_join(&SIMIX_process_self()->simcall, process, timeout);
    return simcall<int, smx_actor_t, double>(SIMCALL_PROCESS_JOIN, process, timeout);
  }

inline static int simcall_BODY_process_sleep(double duration) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_process_sleep(&SIMIX_process_self()->simcall, duration);
    return simcall<int, double>(SIMCALL_PROCESS_SLEEP, duration);
  }

  inline static boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
  simcall_BODY_execution_start(const char* name, double flops_amount, double priority, double bound)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_execution_start(&SIMIX_process_self()->simcall, name, flops_amount, priority, bound);
    return simcall<boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>, const char*, double, double, double>(
        SIMCALL_EXECUTION_START, name, flops_amount, priority, bound);
  }

  inline static boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
  simcall_BODY_execution_parallel_start(const char* name, int host_nb, sg_host_t* host_list, double* flops_amount,
                                        double* bytes_amount, double amount, double rate, double timeout)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_execution_parallel_start(name, host_nb, host_list, flops_amount, bytes_amount, amount, rate, timeout);
    return simcall<boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>, const char*, int, sg_host_t*, double*,
                   double*, double, double, double>(SIMCALL_EXECUTION_PARALLEL_START, name, host_nb, host_list,
                                                    flops_amount, bytes_amount, amount, rate, timeout);
  }

  inline static void
  simcall_BODY_execution_cancel(boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl> execution)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_execution_cancel(execution);
    return simcall<void, boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>>(SIMCALL_EXECUTION_CANCEL,
                                                                                        execution);
  }

  inline static void
  simcall_BODY_execution_set_priority(boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl> execution,
                                      double priority)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_execution_set_priority(execution, priority);
    return simcall<void, boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>, double>(
        SIMCALL_EXECUTION_SET_PRIORITY, execution, priority);
  }

  inline static void
  simcall_BODY_execution_set_bound(boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl> execution,
                                   double bound)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_execution_set_bound(execution, bound);
    return simcall<void, boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>, double>(
        SIMCALL_EXECUTION_SET_BOUND, execution, bound);
  }

  inline static int simcall_BODY_execution_wait(boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl> execution)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_execution_wait(&SIMIX_process_self()->simcall, execution);
    return simcall<int, boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>>(SIMCALL_EXECUTION_WAIT,
                                                                                       execution);
  }

inline static void simcall_BODY_process_on_exit(smx_actor_t process, int_f_pvoid_pvoid_t fun, void* data) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_process_on_exit(process, fun, data);
    return simcall<void, smx_actor_t, int_f_pvoid_pvoid_t, void*>(SIMCALL_PROCESS_ON_EXIT, process, fun, data);
  }

inline static void simcall_BODY_process_auto_restart_set(smx_actor_t process, int auto_restart) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_process_auto_restart_set(process, auto_restart);
    return simcall<void, smx_actor_t, int>(SIMCALL_PROCESS_AUTO_RESTART_SET, process, auto_restart);
  }

inline static smx_actor_t simcall_BODY_process_restart(smx_actor_t process) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_process_restart(&SIMIX_process_self()->simcall, process);
    return simcall<smx_actor_t, smx_actor_t>(SIMCALL_PROCESS_RESTART, process);
  }

  inline static boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>
  simcall_BODY_comm_iprobe(smx_mailbox_t mbox, int type, simix_match_func_t match_fun, void* data)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0)
      simcall_HANDLER_comm_iprobe(&SIMIX_process_self()->simcall, mbox, type, match_fun, data);
    return simcall<boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>, smx_mailbox_t, int,
                   simix_match_func_t, void*>(SIMCALL_COMM_IPROBE, mbox, type, match_fun, data);
  }

inline static void simcall_BODY_comm_send(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff, size_t src_buff_size, simix_match_func_t match_fun, simix_copy_data_func_t copy_data_fun, void* data, double timeout) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_comm_send(&SIMIX_process_self()->simcall, sender, mbox, task_size, rate, src_buff, src_buff_size, match_fun, copy_data_fun, data, timeout);
    return simcall<void, smx_actor_t, smx_mailbox_t, double, double, void*, size_t, simix_match_func_t, simix_copy_data_func_t, void*, double>(SIMCALL_COMM_SEND, sender, mbox, task_size, rate, src_buff, src_buff_size, match_fun, copy_data_fun, data, timeout);
  }

  inline static boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>
  simcall_BODY_comm_isend(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                          size_t src_buff_size, simix_match_func_t match_fun, simix_clean_func_t clean_fun,
                          simix_copy_data_func_t copy_data_fun, void* data, int detached)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_comm_isend(&SIMIX_process_self()->simcall, sender, mbox, task_size, rate, src_buff, src_buff_size, match_fun, clean_fun, copy_data_fun, data, detached);
    return simcall<boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>, smx_actor_t, smx_mailbox_t, double,
                   double, void*, size_t, simix_match_func_t, simix_clean_func_t, simix_copy_data_func_t, void*, int>(
        SIMCALL_COMM_ISEND, sender, mbox, task_size, rate, src_buff, src_buff_size, match_fun, clean_fun, copy_data_fun,
        data, detached);
  }

inline static void simcall_BODY_comm_recv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size, simix_match_func_t match_fun, simix_copy_data_func_t copy_data_fun, void* data, double timeout, double rate) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_comm_recv(&SIMIX_process_self()->simcall, receiver, mbox, dst_buff, dst_buff_size, match_fun, copy_data_fun, data, timeout, rate);
    return simcall<void, smx_actor_t, smx_mailbox_t, void*, size_t*, simix_match_func_t, simix_copy_data_func_t, void*, double, double>(SIMCALL_COMM_RECV, receiver, mbox, dst_buff, dst_buff_size, match_fun, copy_data_fun, data, timeout, rate);
  }

  inline static boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>
  simcall_BODY_comm_irecv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                          simix_match_func_t match_fun, simix_copy_data_func_t copy_data_fun, void* data, double rate)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_comm_irecv(&SIMIX_process_self()->simcall, receiver, mbox, dst_buff, dst_buff_size, match_fun, copy_data_fun, data, rate);
    return simcall<boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>, smx_actor_t, smx_mailbox_t, void*,
                   size_t*, simix_match_func_t, simix_copy_data_func_t, void*, double>(
        SIMCALL_COMM_IRECV, receiver, mbox, dst_buff, dst_buff_size, match_fun, copy_data_fun, data, rate);
  }

inline static int simcall_BODY_comm_waitany(xbt_dynar_t comms, double timeout) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_comm_waitany(&SIMIX_process_self()->simcall, comms, timeout);
    return simcall<int, xbt_dynar_t, double>(SIMCALL_COMM_WAITANY, comms, timeout);
  }

  inline static void simcall_BODY_comm_wait(boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl> comm,
                                            double timeout)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_comm_wait(&SIMIX_process_self()->simcall, comm, timeout);
    return simcall<void, boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>, double>(SIMCALL_COMM_WAIT, comm,
                                                                                                timeout);
  }

  inline static int simcall_BODY_comm_test(boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl> comm)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_comm_test(&SIMIX_process_self()->simcall, comm);
    return simcall<int, boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>>(SIMCALL_COMM_TEST, comm);
  }

  inline static int simcall_BODY_comm_testany(boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>* comms,
                                              size_t count)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_comm_testany(&SIMIX_process_self()->simcall, comms, count);
    return simcall<int, boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>*, size_t>(SIMCALL_COMM_TESTANY,
                                                                                                comms, count);
  }

inline static smx_mutex_t simcall_BODY_mutex_init() {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_mutex_init(&SIMIX_process_self()->simcall);
    return simcall<smx_mutex_t>(SIMCALL_MUTEX_INIT);
  }

inline static void simcall_BODY_mutex_lock(smx_mutex_t mutex) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_mutex_lock(&SIMIX_process_self()->simcall, mutex);
    return simcall<void, smx_mutex_t>(SIMCALL_MUTEX_LOCK, mutex);
  }

inline static int simcall_BODY_mutex_trylock(smx_mutex_t mutex) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_mutex_trylock(&SIMIX_process_self()->simcall, mutex);
    return simcall<int, smx_mutex_t>(SIMCALL_MUTEX_TRYLOCK, mutex);
  }

inline static void simcall_BODY_mutex_unlock(smx_mutex_t mutex) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_mutex_unlock(&SIMIX_process_self()->simcall, mutex);
    return simcall<void, smx_mutex_t>(SIMCALL_MUTEX_UNLOCK, mutex);
  }

inline static smx_cond_t simcall_BODY_cond_init() {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_cond_init();
    return simcall<smx_cond_t>(SIMCALL_COND_INIT);
  }

inline static void simcall_BODY_cond_signal(smx_cond_t cond) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_cond_signal(cond);
    return simcall<void, smx_cond_t>(SIMCALL_COND_SIGNAL, cond);
  }

inline static void simcall_BODY_cond_wait(smx_cond_t cond, smx_mutex_t mutex) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_cond_wait(&SIMIX_process_self()->simcall, cond, mutex);
    return simcall<void, smx_cond_t, smx_mutex_t>(SIMCALL_COND_WAIT, cond, mutex);
  }

inline static void simcall_BODY_cond_wait_timeout(smx_cond_t cond, smx_mutex_t mutex, double timeout) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_cond_wait_timeout(&SIMIX_process_self()->simcall, cond, mutex, timeout);
    return simcall<void, smx_cond_t, smx_mutex_t, double>(SIMCALL_COND_WAIT_TIMEOUT, cond, mutex, timeout);
  }

inline static void simcall_BODY_cond_broadcast(smx_cond_t cond) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_cond_broadcast(cond);
    return simcall<void, smx_cond_t>(SIMCALL_COND_BROADCAST, cond);
  }

inline static smx_sem_t simcall_BODY_sem_init(unsigned int capacity) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_sem_init(capacity);
    return simcall<smx_sem_t, unsigned int>(SIMCALL_SEM_INIT, capacity);
  }

inline static void simcall_BODY_sem_release(smx_sem_t sem) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_sem_release(&SIMIX_process_self()->simcall, sem);
    return simcall<void, smx_sem_t>(SIMCALL_SEM_RELEASE, sem);
  }

inline static int simcall_BODY_sem_would_block(smx_sem_t sem) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_sem_would_block(&SIMIX_process_self()->simcall, sem);
    return simcall<int, smx_sem_t>(SIMCALL_SEM_WOULD_BLOCK, sem);
  }

inline static void simcall_BODY_sem_acquire(smx_sem_t sem) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_sem_acquire(&SIMIX_process_self()->simcall, sem);
    return simcall<void, smx_sem_t>(SIMCALL_SEM_ACQUIRE, sem);
  }

inline static void simcall_BODY_sem_acquire_timeout(smx_sem_t sem, double timeout) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_sem_acquire_timeout(&SIMIX_process_self()->simcall, sem, timeout);
    return simcall<void, smx_sem_t, double>(SIMCALL_SEM_ACQUIRE_TIMEOUT, sem, timeout);
  }

inline static int simcall_BODY_sem_get_capacity(smx_sem_t sem) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_sem_get_capacity(&SIMIX_process_self()->simcall, sem);
    return simcall<int, smx_sem_t>(SIMCALL_SEM_GET_CAPACITY, sem);
  }

  inline static sg_size_t simcall_BODY_file_read(surf_file_t fd, sg_size_t size, sg_host_t host)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_file_read(&SIMIX_process_self()->simcall, fd, size, host);
    return simcall<sg_size_t, surf_file_t, sg_size_t, sg_host_t>(SIMCALL_FILE_READ, fd, size, host);
  }

  inline static sg_size_t simcall_BODY_file_write(surf_file_t fd, sg_size_t size, sg_host_t host)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_file_write(&SIMIX_process_self()->simcall, fd, size, host);
    return simcall<sg_size_t, surf_file_t, sg_size_t, sg_host_t>(SIMCALL_FILE_WRITE, fd, size, host);
  }

  inline static surf_file_t simcall_BODY_file_open(const char* mount, const char* path, sg_storage_t st)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0)
      simcall_HANDLER_file_open(&SIMIX_process_self()->simcall, mount, path, st);
    return simcall<surf_file_t, const char*, const char*, sg_storage_t>(SIMCALL_FILE_OPEN, mount, path, st);
  }

  inline static int simcall_BODY_file_close(surf_file_t fd, sg_host_t host)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_file_close(&SIMIX_process_self()->simcall, fd, host);
    return simcall<int, surf_file_t, sg_host_t>(SIMCALL_FILE_CLOSE, fd, host);
  }

inline static int simcall_BODY_mc_random(int min, int max) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) simcall_HANDLER_mc_random(&SIMIX_process_self()->simcall, min, max);
    return simcall<int, int, int>(SIMCALL_MC_RANDOM, min, max);
  }

  inline static void simcall_BODY_set_category(boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl> synchro,
                                               const char* category)
  {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_set_category(synchro, category);
    return simcall<void, boost::intrusive_ptr<simgrid::kernel::activity::ActivityImpl>, const char*>(
        SIMCALL_SET_CATEGORY, synchro, category);
  }

inline static void simcall_BODY_run_kernel(std::function<void()> const* code) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_run_kernel(code);
    return simcall<void, std::function<void()> const*>(SIMCALL_RUN_KERNEL, code);
  }

inline static void simcall_BODY_run_blocking(std::function<void()> const* code) {
    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_run_blocking(code);
    return simcall<void, std::function<void()> const*>(SIMCALL_RUN_BLOCKING, code);
  }/** @endcond */
