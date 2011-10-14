/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "mc/mc.h"
#include "xbt/log.h"
#include "mailbox.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_gos, msg,
                                "Logging specific to MSG (gos)");

/** \ingroup msg_gos_functions
 *
 * \brief Return the last value returned by a MSG function (except
 * MSG_get_errno...).
 */
MSG_error_t MSG_get_errno(void)
{
  return PROCESS_GET_ERRNO();
}

/** \ingroup msg_gos_functions
 * \brief Executes a task and waits for its termination.
 *
 * This function is used for describing the behavior of an agent. It
 * takes only one parameter.
 * \param task a #m_task_t to execute on the location on which the
 agent is running.
 * \return #MSG_FATAL if \a task is not properly initialized and
 * #MSG_OK otherwise.
 */
MSG_error_t MSG_task_execute(m_task_t task)
{
  simdata_task_t simdata = NULL;
  simdata_process_t p_simdata;
  e_smx_state_t comp_state;
  CHECK_HOST();

  simdata = task->simdata;

  xbt_assert(simdata->host_nb == 0,
              "This is a parallel task. Go to hell.");

#ifdef HAVE_TRACING
  TRACE_msg_task_execute_start(task);
#endif

  xbt_assert((!simdata->compute) && (task->simdata->isused == 0),
              "This task is executed somewhere else. Go fix your code! %d",
              task->simdata->isused);

  XBT_DEBUG("Computing on %s", MSG_process_get_name(MSG_process_self()));

  if (simdata->computation_amount == 0) {
#ifdef HAVE_TRACING
    TRACE_msg_task_execute_end(task);
#endif
    return MSG_OK;
  }
  simdata->isused=1;
  simdata->compute =
      SIMIX_req_host_execute(task->name, SIMIX_host_self(),
                           simdata->computation_amount,
                           simdata->priority);
#ifdef HAVE_TRACING
  SIMIX_req_set_category(simdata->compute, task->category);
#endif

  p_simdata = SIMIX_process_self_get_data();
  p_simdata->waiting_action = simdata->compute;
  comp_state = SIMIX_req_host_execution_wait(simdata->compute);
  p_simdata->waiting_action = NULL;

  simdata->isused=0;

  XBT_DEBUG("Execution task '%s' finished in state %d", task->name, comp_state);
  if (comp_state == SIMIX_DONE) {
    /* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
    simdata->computation_amount = 0.0;
    simdata->comm = NULL;
    simdata->compute = NULL;
#ifdef HAVE_TRACING
    TRACE_msg_task_execute_end(task);
#endif
    MSG_RETURN(MSG_OK);
  } else if (SIMIX_req_host_get_state(SIMIX_host_self()) == 0) {
    /* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
    simdata->comm = NULL;
    simdata->compute = NULL;
#ifdef HAVE_TRACING
    TRACE_msg_task_execute_end(task);
#endif
    MSG_RETURN(MSG_HOST_FAILURE);
  } else {
    /* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
    simdata->comm = NULL;
    simdata->compute = NULL;
#ifdef HAVE_TRACING
    TRACE_msg_task_execute_end(task);
#endif
    MSG_RETURN(MSG_TASK_CANCELLED);
  }
}

/** \ingroup m_task_management
 * \brief Creates a new #m_task_t (a parallel one....).
 *
 * A constructor for #m_task_t taking six arguments and returning the
 corresponding object.
 * \param name a name for the object. It is for user-level information
 and can be NULL.
 * \param host_nb the number of hosts implied in the parallel task.
 * \param host_list an array of \p host_nb m_host_t.
 * \param computation_amount an array of \p host_nb
 doubles. computation_amount[i] is the total number of operations
 that have to be performed on host_list[i].
 * \param communication_amount an array of \p host_nb* \p host_nb doubles.
 * \param data a pointer to any data may want to attach to the new
 object.  It is for user-level information and can be NULL. It can
 be retrieved with the function \ref MSG_task_get_data.
 * \see m_task_t
 * \return The new corresponding object.
 */
m_task_t
MSG_parallel_task_create(const char *name, int host_nb,
                         const m_host_t * host_list,
                         double *computation_amount,
                         double *communication_amount, void *data)
{
  int i;
  simdata_task_t simdata = xbt_new0(s_simdata_task_t, 1);
  m_task_t task = xbt_new0(s_m_task_t, 1);
  task->simdata = simdata;

  /* Task structure */
  task->name = xbt_strdup(name);
  task->data = data;

  /* Simulator Data */
  simdata->computation_amount = 0;
  simdata->message_size = 0;
  simdata->compute = NULL;
  simdata->comm = NULL;
  simdata->rate = -1.0;
  simdata->isused = 0;
  simdata->sender = NULL;
  simdata->receiver = NULL;
  simdata->source = NULL;

  simdata->host_nb = host_nb;
  simdata->host_list = xbt_new0(smx_host_t, host_nb);
  simdata->comp_amount = computation_amount;
  simdata->comm_amount = communication_amount;

  for (i = 0; i < host_nb; i++)
    simdata->host_list[i] = host_list[i]->simdata->smx_host;

  return task;
}

MSG_error_t MSG_parallel_task_execute(m_task_t task)
{
  simdata_task_t simdata = NULL;
  e_smx_state_t comp_state;
  simdata_process_t p_simdata;
  CHECK_HOST();

  simdata = task->simdata;
  p_simdata = SIMIX_process_self_get_data();

  xbt_assert((!simdata->compute)
              && (task->simdata->isused == 0),
              "This task is executed somewhere else. Go fix your code!");

  xbt_assert(simdata->host_nb,
              "This is not a parallel task. Go to hell.");

  XBT_DEBUG("Parallel computing on %s", p_simdata->m_host->name);

  simdata->isused=1;

  simdata->compute =
      SIMIX_req_host_parallel_execute(task->name, simdata->host_nb,
                                  simdata->host_list,
                                  simdata->comp_amount,
                                  simdata->comm_amount, 1.0, -1.0);
  XBT_DEBUG("Parallel execution action created: %p", simdata->compute);

  p_simdata->waiting_action = simdata->compute;
  comp_state = SIMIX_req_host_execution_wait(simdata->compute);
  p_simdata->waiting_action = NULL;

  XBT_DEBUG("Finished waiting for execution of action %p, state = %d", simdata->compute, comp_state);

  simdata->isused=0;

  if (comp_state == SIMIX_DONE) {
    /* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
    simdata->computation_amount = 0.0;
    simdata->comm = NULL;
    simdata->compute = NULL;
    MSG_RETURN(MSG_OK);
  } else if (SIMIX_req_host_get_state(SIMIX_host_self()) == 0) {
    /* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
    simdata->comm = NULL;
    simdata->compute = NULL;
    MSG_RETURN(MSG_HOST_FAILURE);
  } else {
    /* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
    simdata->comm = NULL;
    simdata->compute = NULL;
    MSG_RETURN(MSG_TASK_CANCELLED);
  }
}


/** \ingroup msg_gos_functions
 * \brief Sleep for the specified number of seconds
 *
 * Makes the current process sleep until \a time seconds have elapsed.
 *
 * \param nb_sec a number of second
 */
MSG_error_t MSG_process_sleep(double nb_sec)
{
  e_smx_state_t state;
  /*m_process_t proc = MSG_process_self();*/

#ifdef HAVE_TRACING
  TRACE_msg_process_sleep_in(MSG_process_self());
#endif

  /* create action to sleep */
  state = SIMIX_req_process_sleep(nb_sec);

  /*proc->simdata->waiting_action = act_sleep;

  FIXME: check if not setting the waiting_action breaks something on msg
  
  proc->simdata->waiting_action = NULL;*/
  
  if (state == SIMIX_DONE) {
#ifdef HAVE_TRACING
  TRACE_msg_process_sleep_out(MSG_process_self());
#endif
    MSG_RETURN(MSG_OK);
  } else {
#ifdef HAVE_TRACING
    TRACE_msg_process_sleep_out(MSG_process_self());
#endif
    MSG_RETURN(MSG_HOST_FAILURE);
  }
}

/** \ingroup msg_gos_functions
 * \brief Listen on \a channel and waits for receiving a task from \a host.
 *
 * It takes three parameters.
 * \param task a memory location for storing a #m_task_t. It will
 hold a task when this function will return. Thus \a task should not
 be equal to \c NULL and \a *task should be equal to \c NULL. If one of
 those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the agent should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \param host the host that is to be watched.
 * \return #MSG_FATAL if \a task is equal to \c NULL, #MSG_WARNING
 if \a *task is not equal to \c NULL, and #MSG_OK otherwise.
 */
MSG_error_t
MSG_task_get_from_host(m_task_t * task, m_channel_t channel, m_host_t host)
{
  return MSG_task_get_ext(task, channel, -1, host);
}

/** \ingroup msg_gos_functions
 * \brief Listen on a channel and wait for receiving a task.
 *
 * It takes two parameters.
 * \param task a memory location for storing a #m_task_t. It will
 hold a task when this function will return. Thus \a task should not
 be equal to \c NULL and \a *task should be equal to \c NULL. If one of
 those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the agent should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \return #MSG_FATAL if \a task is equal to \c NULL, #MSG_WARNING
 * if \a *task is not equal to \c NULL, and #MSG_OK otherwise.
 */
MSG_error_t MSG_task_get(m_task_t * task, m_channel_t channel)
{
  return MSG_task_get_with_timeout(task, channel, -1);
}

/** \ingroup msg_gos_functions
 * \brief Listen on a channel and wait for receiving a task with a timeout.
 *
 * It takes three parameters.
 * \param task a memory location for storing a #m_task_t. It will
 hold a task when this function will return. Thus \a task should not
 be equal to \c NULL and \a *task should be equal to \c NULL. If one of
 those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the agent should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \param max_duration the maximum time to wait for a task before giving
 up. In such a case, #MSG_TRANSFER_FAILURE will be returned, \a task
 will not be modified and will still be
 equal to \c NULL when returning.
 * \return #MSG_FATAL if \a task is equal to \c NULL, #MSG_WARNING
 if \a *task is not equal to \c NULL, and #MSG_OK otherwise.
 */
MSG_error_t
MSG_task_get_with_timeout(m_task_t * task, m_channel_t channel,
                          double max_duration)
{
  return MSG_task_get_ext(task, channel, max_duration, NULL);
}

/** \defgroup msg_gos_functions MSG Operating System Functions
 *  \brief This section describes the functions that can be used
 *  by an agent for handling some task.
 */

MSG_error_t
MSG_task_get_ext(m_task_t * task, m_channel_t channel, double timeout,
                 m_host_t host)
{
  xbt_assert((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  return
      MSG_mailbox_get_task_ext(MSG_mailbox_get_by_channel
                               (MSG_host_self(), channel), task, host,
                               timeout);
}

MSG_error_t
MSG_task_receive_from_host(m_task_t * task, const char *alias,
                           m_host_t host)
{
  return MSG_task_receive_ext(task, alias, -1, host);
}

MSG_error_t MSG_task_receive(m_task_t * task, const char *alias)
{
  return MSG_task_receive_with_timeout(task, alias, -1);
}

MSG_error_t
MSG_task_receive_with_timeout(m_task_t * task, const char *alias,
                              double timeout)
{
  return MSG_task_receive_ext(task, alias, timeout, NULL);
}

MSG_error_t
MSG_task_receive_ext(m_task_t * task, const char *alias, double timeout,
                     m_host_t host)
{
  XBT_DEBUG
      ("MSG_task_receive_ext: Trying to receive a message on mailbox '%s'",
       alias);
  return MSG_mailbox_get_task_ext(MSG_mailbox_get_by_alias(alias), task,
                                  host, timeout);
}

/** \ingroup msg_gos_functions
 * \brief Sends a task on a mailbox.
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test()
 * to end the communication.
 *
 * \param task a #m_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_isend(m_task_t task, const char *alias)
{
  return MSG_task_isend_with_matching(task,alias,NULL,NULL);
}
/** \ingroup msg_gos_functions
 * \brief Sends a task on a mailbox, with support for matching requests
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test()
 * to end the communication.
 *
 * \param task a #m_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \param match_fun boolean function taking the #match_data provided by sender (here), and the one of the receiver (if any) and returning whether they match
 * \param match_data user provided data passed to match_fun
 * \return the msg_comm_t communication created
 */
XBT_INLINE msg_comm_t MSG_task_isend_with_matching(m_task_t task, const char *alias,
    int (*match_fun)(void*,void*),
    void *match_data)
{
  simdata_task_t t_simdata = NULL;
  m_process_t process = MSG_process_self();
  msg_mailbox_t mailbox = MSG_mailbox_get_by_alias(alias);

  CHECK_HOST();

  /* FIXME: these functions are not traceable */

  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = MSG_host_self();

  xbt_assert(t_simdata->isused == 0,
              "This task is still being used somewhere else. You cannot send it now. Go fix your code!");

  t_simdata->isused = 1;
  msg_global->sent_msg++;

  /* Send it by calling SIMIX network layer */
  msg_comm_t comm = xbt_new0(s_msg_comm_t, 1);
  comm->task_sent = task;
  comm->task_received = NULL;
  comm->status = MSG_OK;
  comm->s_comm =
    SIMIX_req_comm_isend(mailbox, t_simdata->message_size,
                         t_simdata->rate, task, sizeof(void *), match_fun, match_data, 0);
  t_simdata->comm = comm->s_comm; /* FIXME: is the field t_simdata->comm still useful? */

  return comm;
}

/** \ingroup msg_gos_functions
 * \brief Sends a task on a mailbox.
 *
 * This is a non blocking detached send function.
 * Think of it as a best effort send. The task should
 * be destroyed by the receiver.
 *
 * \param task a #m_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \param cleanup a function to destroy the task if the
 * communication fails (if NULL, MSG_task_destroy() will
 * be used by default)
 */
void MSG_task_dsend(m_task_t task, const char *alias, void_f_pvoid_t cleanup)
{
  simdata_task_t t_simdata = NULL;
  m_process_t process = MSG_process_self();
  msg_mailbox_t mailbox = MSG_mailbox_get_by_alias(alias);

  CHECK_HOST();

  if (cleanup == NULL) {
    cleanup = (void_f_pvoid_t) MSG_task_destroy;
  }

  /* FIXME: these functions are not traceable */

  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = MSG_host_self();

  xbt_assert(t_simdata->isused == 0,
              "This task is still being used somewhere else. You cannot send it now. Go fix your code!");

  t_simdata->isused = 1;
  msg_global->sent_msg++;

  /* Send it by calling SIMIX network layer */
  SIMIX_req_comm_isend(mailbox, t_simdata->message_size,
                       t_simdata->rate, task, sizeof(void *), NULL, cleanup, 1);
}

/** \ingroup msg_gos_functions
 * \brief Starts listening for receiving a task from an asynchronous communication.
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test()
 * to end the communication.
 *
 * \param task a memory location for storing a #m_task_t.
 * \param name of the mailbox to receive the task on
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_irecv(m_task_t *task, const char *name)
{
  smx_rdv_t rdv = MSG_mailbox_get_by_alias(name);

  CHECK_HOST();

  /* FIXME: these functions are not tracable */

  /* Sanity check */
  xbt_assert(task, "Null pointer for the task storage");

  if (*task)
    XBT_CRITICAL
        ("MSG_task_get() was asked to write in a non empty task struct.");

  /* Try to receive it by calling SIMIX network layer */
  msg_comm_t comm = xbt_new0(s_msg_comm_t, 1);
  comm->task_sent = NULL;
  comm->task_received = task;
  comm->status = MSG_OK;
  comm->s_comm = SIMIX_req_comm_irecv(rdv, task, NULL, NULL, NULL);

  return comm;
}

/** \ingroup msg_gos_functions
 * \brief Checks whether a communication is done, and if yes, finalizes it.
 * \param comm the communication to test
 * \return TRUE if the communication is finished
 * (but it may have failed, use MSG_comm_get_status() to know its status)
 * or FALSE if the communication is not finished yet
 * If the status is FALSE, don't forget to use MSG_process_sleep() after the test.
 */
int MSG_comm_test(msg_comm_t comm)
{
  xbt_ex_t e;
  int finished = 0;
  TRY {
    finished = SIMIX_req_comm_test(comm->s_comm);
  }
  CATCH(e) {
    switch (e.category) {

      case host_error:
        comm->status = MSG_HOST_FAILURE;
        finished = 1;
        break;

      case network_error:
        comm->status = MSG_TRANSFER_FAILURE;
        finished = 1;
        break;

      case timeout_error:
        comm->status = MSG_TIMEOUT;
        finished = 1;
        break;

      default:
        RETHROW;
    }
    xbt_ex_free(e);
  }

  return finished;
}

/** \ingroup msg_gos_functions
 * \brief This function checks if a communication is finished.
 * \param comms a vector of communications
 * \return the position of the finished communication if any
 * (but it may have failed, use MSG_comm_get_status() to know its status),
 * or -1 if none is finished
 */
int MSG_comm_testany(xbt_dynar_t comms)
{
  xbt_ex_t e;
  int finished_index = -1;

  /* create the equivalent dynar with SIMIX objects */
  xbt_dynar_t s_comms = xbt_dynar_new(sizeof(smx_action_t), NULL);
  msg_comm_t comm;
  unsigned int cursor;
  xbt_dynar_foreach(comms, cursor, comm) {
    xbt_dynar_push(s_comms, &comm->s_comm);
  }

  MSG_error_t status = MSG_OK;
  TRY {
    finished_index = SIMIX_req_comm_testany(s_comms);
  }
  CATCH(e) {
    switch (e.category) {

      case host_error:
        finished_index = e.value;
        status = MSG_HOST_FAILURE;
        break;

      case network_error:
        finished_index = e.value;
        status = MSG_TRANSFER_FAILURE;
        break;

      case timeout_error:
        finished_index = e.value;
        status = MSG_TIMEOUT;
        break;

      default:
        RETHROW;
    }
    xbt_ex_free(e);
  }
  xbt_dynar_free(&s_comms);

  if (finished_index != -1) {
    comm = xbt_dynar_get_as(comms, finished_index, msg_comm_t);
    /* the communication is finished */
    comm->status = status;
  }

  return finished_index;
}

/** \ingroup msg_gos_functions
 * \brief Destroys a communication.
 * \param comm the communication to destroy.
 */
void MSG_comm_destroy(msg_comm_t comm)
{
  if (comm->task_received != NULL
      && *comm->task_received != NULL
      && MSG_comm_get_status(comm) == MSG_OK) {
    (*comm->task_received)->simdata->isused = 0;
  }

  xbt_free(comm);
}

/** \ingroup msg_gos_functions
 * \brief Wait for the completion of a communication.
 *
 * It takes two parameters.
 * \param comm the communication to wait.
 * \param timeout Wait until the communication terminates or the timeout occurs
 * \return MSG_error_t
 */
MSG_error_t MSG_comm_wait(msg_comm_t comm, double timeout)
{
  xbt_ex_t e;
  TRY {
    SIMIX_req_comm_wait(comm->s_comm, timeout);

    if (comm->task_received != NULL) {
      /* I am the receiver */
      (*comm->task_received)->simdata->isused = 0;
    }

    /* FIXME: these functions are not traceable */
  }
  CATCH(e) {
    switch (e.category) {
    case host_error:
      comm->status = MSG_HOST_FAILURE;
      break;
    case network_error:
      comm->status = MSG_TRANSFER_FAILURE;
      break;
    case timeout_error:
      comm->status = MSG_TIMEOUT;
      break;
    default:
      RETHROW;
    }
    xbt_ex_free(e);
  }

  return comm->status;
}

/** \ingroup msg_gos_functions
* \brief This function is called by a sender and permit to wait for each communication
*
* \param comm a vector of communication
* \param nb_elem is the size of the comm vector
* \param timeout for each call of MSG_comm_wait
*/
void MSG_comm_waitall(msg_comm_t * comm, int nb_elem, double timeout)
{
  int i = 0;
  for (i = 0; i < nb_elem; i++) {
    MSG_comm_wait(comm[i], timeout);
  }
}

/** \ingroup msg_gos_functions
 * \brief This function waits for the first communication finished in a list.
 * \param comms a vector of communications
 * \return the position of the first finished communication
 * (but it may have failed, use MSG_comm_get_status() to know its status)
 */
int MSG_comm_waitany(xbt_dynar_t comms)
{
  xbt_ex_t e;
  int finished_index = -1;

  /* create the equivalent dynar with SIMIX objects */
  xbt_dynar_t s_comms = xbt_dynar_new(sizeof(smx_action_t), NULL);
  msg_comm_t comm;
  unsigned int cursor;
  xbt_dynar_foreach(comms, cursor, comm) {
    xbt_dynar_push(s_comms, &comm->s_comm);
  }

  MSG_error_t status = MSG_OK;
  TRY {
    finished_index = SIMIX_req_comm_waitany(s_comms);
  }
  CATCH(e) {
    switch (e.category) {

      case host_error:
        finished_index = e.value;
        status = MSG_HOST_FAILURE;
        break;

      case network_error:
        finished_index = e.value;
        status = MSG_TRANSFER_FAILURE;
        break;

      case timeout_error:
        finished_index = e.value;
        status = MSG_TIMEOUT;
        break;

      default:
        RETHROW;
    }
    xbt_ex_free(e);
  }

  xbt_assert(finished_index != -1, "WaitAny returned -1");
  xbt_dynar_free(&s_comms);

  comm = xbt_dynar_get_as(comms, finished_index, msg_comm_t);
  /* the communication is finished */
  comm->status = status;

  return finished_index;
}

/**
 * \ingroup msg_gos_functions
 * \brief Returns the error (if any) that occured during a finished communication.
 * \param comm a finished communication
 * \return the status of the communication, or MSG_OK if no error occured
 * during the communication
 */
MSG_error_t MSG_comm_get_status(msg_comm_t comm) {

  return comm->status;
}

m_task_t MSG_comm_get_task(msg_comm_t comm)
{
  xbt_assert(comm, "Invalid parameter");

  return comm->task_received ? *comm->task_received : comm->task_sent;
}

/** \ingroup msg_gos_functions
 * \brief Put a task on a channel of an host and waits for the end of the
 * transmission.
 *
 * This function is used for describing the behavior of an agent. It
 * takes three parameter.
 * \param task a #m_task_t to send on another location. This task
 will not be usable anymore when the function will return. There is
 no automatic task duplication and you have to save your parameters
 before calling this function. Tasks are unique and once it has been
 sent to another location, you should not access it anymore. You do
 not need to call MSG_task_destroy() but to avoid using, as an
 effect of inattention, this task anymore, you definitely should
 renitialize it with #MSG_TASK_UNINITIALIZED. Note that this task
 can be transfered iff it has been correctly created with
 MSG_task_create().
 * \param dest the destination of the message
 * \param channel the channel on which the agent should put this
 task. This value has to be >=0 and < than the maximal number of
 channels fixed with MSG_set_channel_number().
 * \return #MSG_FATAL if \a task is not properly initialized and
 * #MSG_OK otherwise. Returns #MSG_HOST_FAILURE if the host on which
 * this function was called was shut down. Returns
 * #MSG_TRANSFER_FAILURE if the transfer could not be properly done
 * (network failure, dest failure)
 */
MSG_error_t MSG_task_put(m_task_t task, m_host_t dest, m_channel_t channel)
{
  return MSG_task_put_with_timeout(task, dest, channel, -1.0);
}

/** \ingroup msg_gos_functions
 * \brief Does exactly the same as MSG_task_put but with a bounded transmition
 * rate.
 *
 * \sa MSG_task_put
 */
MSG_error_t
MSG_task_put_bounded(m_task_t task, m_host_t dest, m_channel_t channel,
                     double maxrate)
{
  task->simdata->rate = maxrate;
  return MSG_task_put(task, dest, channel);
}

/** \ingroup msg_gos_functions \brief Put a task on a channel of an
 * host (with a timeout on the waiting of the destination host) and
 * waits for the end of the transmission.
 *
 * This function is used for describing the behavior of an agent. It
 * takes four parameter.
 * \param task a #m_task_t to send on another location. This task
 will not be usable anymore when the function will return. There is
 no automatic task duplication and you have to save your parameters
 before calling this function. Tasks are unique and once it has been
 sent to another location, you should not access it anymore. You do
 not need to call MSG_task_destroy() but to avoid using, as an
 effect of inattention, this task anymore, you definitely should
 renitialize it with #MSG_TASK_UNINITIALIZED. Note that this task
 can be transfered iff it has been correctly created with
 MSG_task_create().
 * \param dest the destination of the message
 * \param channel the channel on which the agent should put this
 task. This value has to be >=0 and < than the maximal number of
 channels fixed with MSG_set_channel_number().
 * \param timeout the maximum time to wait for a task before giving
 up. In such a case, #MSG_TRANSFER_FAILURE will be returned, \a task
 will not be modified
 * \return #MSG_FATAL if \a task is not properly initialized and
#MSG_OK otherwise. Returns #MSG_HOST_FAILURE if the host on which
this function was called was shut down. Returns
#MSG_TRANSFER_FAILURE if the transfer could not be properly done
(network failure, dest failure, timeout...)
 */
MSG_error_t
MSG_task_put_with_timeout(m_task_t task, m_host_t dest,
                          m_channel_t channel, double timeout)
{
  xbt_assert((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  XBT_DEBUG("MSG_task_put_with_timout: Trying to send a task to '%s'", dest->name);
  return
      MSG_mailbox_put_with_timeout(MSG_mailbox_get_by_channel
                                   (dest, channel), task, timeout);
}

MSG_error_t MSG_task_send(m_task_t task, const char *alias)
{
  XBT_DEBUG("MSG_task_send: Trying to send a message on mailbox '%s'", alias);
  return MSG_task_send_with_timeout(task, alias, -1);
}


MSG_error_t
MSG_task_send_bounded(m_task_t task, const char *alias, double maxrate)
{
  task->simdata->rate = maxrate;
  return MSG_task_send(task, alias);
}


MSG_error_t
MSG_task_send_with_timeout(m_task_t task, const char *alias,
                           double timeout)
{
  return MSG_mailbox_put_with_timeout(MSG_mailbox_get_by_alias(alias),
                                      task, timeout);
}

int MSG_task_listen(const char *alias)
{
  CHECK_HOST();

  return !MSG_mailbox_is_empty(MSG_mailbox_get_by_alias(alias));
}

/** \ingroup msg_gos_functions
 * \brief Test whether there is a pending communication on a channel.
 *
 * It takes one parameter.
 * \param channel the channel on which the agent should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \return 1 if there is a pending communication and 0 otherwise
 */
int MSG_task_Iprobe(m_channel_t channel)
{
  xbt_assert((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  CHECK_HOST();

  return
      !MSG_mailbox_is_empty(MSG_mailbox_get_by_channel
                            (MSG_host_self(), channel));
}

/** \ingroup msg_gos_functions

 * \brief Return the number of tasks waiting to be received on a \a
 channel and sent by \a host.
 *
 * It takes two parameters.
 * \param channel the channel on which the agent should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \param host the host that is to be watched.
 * \return the number of tasks waiting to be received on \a channel
 and sent by \a host.
 */
int MSG_task_probe_from_host(int channel, m_host_t host)
{
  xbt_assert((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  CHECK_HOST();

  return
      MSG_mailbox_get_count_host_waiting_tasks(MSG_mailbox_get_by_channel
                                               (MSG_host_self(), channel),
                                               host);

}

int MSG_task_listen_from_host(const char *alias, m_host_t host)
{
  CHECK_HOST();

  return
      MSG_mailbox_get_count_host_waiting_tasks(MSG_mailbox_get_by_alias
                                               (alias), host);
}

/** \ingroup msg_gos_functions
 * \brief Test whether there is a pending communication on a channel, and who sent it.
 *
 * It takes one parameter.
 * \param channel the channel on which the agent should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \return -1 if there is no pending communication and the PID of the process who sent it otherwise
 */
int MSG_task_probe_from(m_channel_t channel)
{
  m_task_t task;

  CHECK_HOST();

  xbt_assert((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  if (NULL ==
      (task =
       MSG_mailbox_get_head(MSG_mailbox_get_by_channel
                            (MSG_host_self(), channel))))
    return -1;

  return MSG_process_get_PID(task->simdata->sender);
}

int MSG_task_listen_from(const char *alias)
{
  m_task_t task;

  CHECK_HOST();

  if (NULL ==
      (task = MSG_mailbox_get_head(MSG_mailbox_get_by_alias(alias))))
    return -1;

  return MSG_process_get_PID(task->simdata->sender);
}
