
/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_chord,
                             "Messages specific for this msg example");

#define COMM_SIZE 10
#define COMP_SIZE 0

static int nb_bits = 24;
static int nb_keys = 0;
static int timeout = 50;
static int max_simulation_time = 1000;
static int periodic_stabilize_delay = 20;
static int periodic_fix_fingers_delay = 120;
static int periodic_check_predecessor_delay = 120;
static int periodic_lookup_delay = 10;

/**
 * Finger element.
 */
typedef struct finger {
  int id;
  char* mailbox;
} s_finger_t, *finger_t;

/**
 * Node data.
 */
typedef struct node {
  int id;                                 // my id
  char* mailbox;                          // my usual mailbox name
  s_finger_t *fingers;                    // finger table, of size nb_bits (fingers[0] is my successor)
  int pred_id;                            // predecessor id
  char* pred_mailbox;                     // predecessor's mailbox name
  int next_finger_to_fix;                 // index of the next finger to fix in fix_fingers()
  msg_comm_t comm_receive;                // current communication to receive
  xbt_dynar_t comms;                      // current communications being sent
  double last_change_date;                // last time I changed a finger or my predecessor
} s_node_t, *node_t;

/**
 * Types of tasks exchanged between nodes.
 */
typedef enum {
  TASK_FIND_SUCCESSOR,
  TASK_FIND_SUCCESSOR_ANSWER,
  TASK_GET_PREDECESSOR,
  TASK_GET_PREDECESSOR_ANSWER,
  TASK_NOTIFY,
  TASK_SUCCESSOR_LEAVING,
  TASK_PREDECESSOR_LEAVING
} e_task_type_t;

/**
 * Data attached with the tasks sent and received
 */
typedef struct task_data {
  e_task_type_t type;                     // type of task
  int request_id;                         // id paramater (used by some types of tasks)
  int request_finger;                     // finger parameter (used by some types of tasks)
  int answer_id;                          // answer (used by some types of tasks)
  char* answer_to;                        // mailbox to send an answer to (or NULL)
  const char* issuer_host_name;           // used for logging
} s_task_data_t, *task_data_t;

static int *powers2;

// utility functions
static void chord_initialize(void);
static int normalize(int id);
static int is_in_interval(int id, int start, int end);
static char* get_mailbox(int host_id);
static void task_data_destroy(task_data_t task_data);
static void print_finger_table(node_t node);
static void set_finger(node_t node, int finger_index, int id);
static void set_predecessor(node_t node, int predecessor_id);

// process functions
static int node(int argc, char *argv[]);
static void handle_task(node_t node, m_task_t task);

// Chord core
static void create(node_t node);
static int join(node_t node, int known_id);
static void leave(node_t node);
static int find_successor(node_t node, int id);
static int remote_find_successor(node_t node, int ask_to_id, int id);
static int remote_get_predecessor(node_t node, int ask_to_id);
static int closest_preceding_node(node_t node, int id);
static void stabilize(node_t node);
static void notify(node_t node, int predecessor_candidate_id);
static void remote_notify(node_t node, int notify_to, int predecessor_candidate_id);
static void fix_fingers(node_t node);
static void check_predecessor(node_t node);
static void random_lookup(node_t);
static void quit_notify(node_t node, int to);

/**
 * \brief Global initialization of the Chord simulation.
 */
static void chord_initialize(void)
{
  // compute the powers of 2 once for all
  powers2 = xbt_new(int, nb_bits);
  int pow = 1;
  int i;
  for (i = 0; i < nb_bits; i++) {
    powers2[i] = pow;
    pow = pow << 1;
  }
  nb_keys = pow;
  DEBUG1("Sets nb_keys to %d", nb_keys);
}

/**
 * \brief Turns an id into an equivalent id in [0, nb_keys).
 * \param id an id
 * \return the corresponding normalized id
 */
static int normalize(int id)
{
  // make sure id >= 0
  while (id < 0) {
    id += nb_keys;
  }
  // make sure id < nb_keys
  id = id % nb_keys;

  return id;
}

/**
 * \brief Returns whether a id belongs to the interval [start, end].
 *
 * The parameters are noramlized to make sure they are between 0 and nb_keys - 1).
 * 1 belongs to [62, 3]
 * 1 does not belong to [3, 62]
 * 63 belongs to [62, 3]
 * 63 does not belong to [3, 62]
 * 24 belongs to [21, 29]
 * 24 does not belong to [29, 21]
 *
 * \param id id to check
 * \param start lower bound
 * \param end upper bound
 * \return a non-zero value if id in in [start, end]
 */
static int is_in_interval(int id, int start, int end)
{
  id = normalize(id);
  start = normalize(start);
  end = normalize(end);

  // make sure end >= start and id >= start
  if (end < start) {
    end += nb_keys;
  }

  if (id < start) {
    id += nb_keys;
  }

  return id <= end;
}

/**
 * \brief Gets the mailbox name of a host given its chord id.
 * \param node_id id of a node
 * \return the name of its mailbox
 */
static char* get_mailbox(int node_id)
{
  return bprintf("mailbox%d", node_id);
}

/**
 * \brief Frees the memory used by some task data.
 * \param task_data the task data to destroy
 */
static void task_data_destroy(task_data_t task_data)
{
  xbt_free(task_data->answer_to);
  xbt_free(task_data);
}

/**
 * \brief Displays the finger table of a node.
 * \param node a node
 */
static void print_finger_table(node_t node)
{
  if (XBT_LOG_ISENABLED(msg_chord, xbt_log_priority_verbose)) {
    int i;
    int pow = 1;
    VERB0("My finger table:");
    VERB0("Start | Succ ");
    for (i = 0; i < nb_bits; i++) {
      VERB2(" %3d  | %3d ", (node->id + pow) % nb_keys, node->fingers[i].id);
      pow = pow << 1;
    }
    VERB1("Predecessor: %d", node->pred_id);
  }
}

/**
 * \brief Sets a finger of the current node.
 * \param node the current node
 * \param finger_index index of the finger to set (0 to nb_bits - 1)
 * \param id the id to set for this finger
 */
static void set_finger(node_t node, int finger_index, int id)
{
  if (id != node->fingers[finger_index].id) {
    node->fingers[finger_index].id = id;
    xbt_free(node->fingers[finger_index].mailbox);
    node->fingers[finger_index].mailbox = get_mailbox(id);
    node->last_change_date = MSG_get_clock();
    DEBUG2("My new finger #%d is %d", finger_index, id);
  }
}

/**
 * \brief Sets the predecessor of the current node.
 * \param node the current node
 * \param id the id to predecessor, or -1 to unset the predecessor
 */
static void set_predecessor(node_t node, int predecessor_id)
{
  if (predecessor_id != node->pred_id) {
    node->pred_id = predecessor_id;
    xbt_free(node->pred_mailbox);

    if (predecessor_id != -1) {
      node->pred_mailbox = get_mailbox(predecessor_id);
    }
    node->last_change_date = MSG_get_clock();

    DEBUG1("My new predecessor is %d", predecessor_id);
  }
}

/**
 * \brief Node Function
 * Arguments:
 * - my id
 * - the id of a guy I know in the system (except for the first node)
 * - the time to sleep before I join (except for the first node)
 */
int node(int argc, char *argv[])
{
  double init_time = MSG_get_clock();
  m_task_t task = NULL;
  m_task_t task_received = NULL;
  msg_comm_t comm_send = NULL;
  int i;
  int index;
  int join_success = 0;
  double deadline;
  double next_stabilize_date = init_time + periodic_stabilize_delay;
  double next_fix_fingers_date = init_time + periodic_fix_fingers_delay;
  double next_check_predecessor_date = init_time + periodic_check_predecessor_delay;
  double next_lookup_date = init_time + periodic_lookup_delay;

  xbt_assert0(argc == 3 || argc == 5, "Wrong number of arguments for this node");

  // initialize my node
  s_node_t node = {0};
  node.id = atoi(argv[1]);
  node.mailbox = get_mailbox(node.id);
  node.next_finger_to_fix = 0;
  node.comms = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  node.fingers = xbt_new0(s_finger_t, nb_bits);
  node.last_change_date = init_time;

  for (i = 0; i < nb_bits; i++) {
    set_finger(&node, i, node.id);
  }

  if (argc == 3) { // first ring
    deadline = atof(argv[2]);
    create(&node);
    join_success = 1;
  }
  else {
    int known_id = atoi(argv[2]);
    //double sleep_time = atof(argv[3]);
    deadline = atof(argv[4]);

    /*
    // sleep before starting
    DEBUG1("Let's sleep during %f", sleep_time);
    MSG_process_sleep(sleep_time);
    */
    DEBUG0("Hey! Let's join the system.");

    join_success = join(&node, known_id);
  }

  if (join_success) {
    while (MSG_get_clock() < init_time + deadline
//	&& MSG_get_clock() < node.last_change_date + 1000
	&& MSG_get_clock() < max_simulation_time) {

      if (node.comm_receive == NULL) {
        task_received = NULL;
        node.comm_receive = MSG_task_irecv(&task_received, node.mailbox);
        // FIXME: do not make MSG_task_irecv() calls from several functions
      }

      if (!MSG_comm_test(node.comm_receive)) {

        // no task was received: make some periodic calls
        if (MSG_get_clock() >= next_stabilize_date) {
          stabilize(&node);
          next_stabilize_date = MSG_get_clock() + periodic_stabilize_delay;
        }
        else if (MSG_get_clock() >= next_fix_fingers_date) {
          fix_fingers(&node);
          next_fix_fingers_date = MSG_get_clock() + periodic_fix_fingers_delay;
        }
        else if (MSG_get_clock() >= next_check_predecessor_date) {
          check_predecessor(&node);
          next_check_predecessor_date = MSG_get_clock() + periodic_check_predecessor_delay;
        }
	else if (MSG_get_clock() >= next_lookup_date) {
	  random_lookup(&node);
	  next_lookup_date = MSG_get_clock() + periodic_lookup_delay;
	}
        else {
          // nothing to do: sleep for a while
          MSG_process_sleep(5);
        }
      }
      else {
        // a transfer has occured

        MSG_error_t status = MSG_comm_get_status(node.comm_receive);

        if (status != MSG_OK) {
          DEBUG0("Failed to receive a task. Nevermind.");
          node.comm_receive = NULL;
        }
        else {
          // the task was successfully received
          MSG_comm_destroy(node.comm_receive);
          node.comm_receive = NULL;
          handle_task(&node, task_received);
        }
      }

      // see if some communications are finished
      while ((index = MSG_comm_testany(node.comms)) != -1) {
        comm_send = xbt_dynar_get_as(node.comms, index, msg_comm_t);
        MSG_error_t status = MSG_comm_get_status(comm_send);
        xbt_dynar_remove_at(node.comms, index, &comm_send);
        DEBUG3("Communication %p is finished with status %d, dynar size is now %lu",
            comm_send, status, xbt_dynar_length(node.comms));
        MSG_comm_destroy(comm_send);
      }
    }

    // clean unfinished comms sent
    unsigned int cursor;
    xbt_dynar_foreach(node.comms, cursor, comm_send) {
      task = MSG_comm_get_task(comm_send);
      MSG_task_cancel(task);
      task_data_destroy(MSG_task_get_data(task));
      MSG_task_destroy(task);
      MSG_comm_destroy(comm_send);
      // FIXME: the task is actually not destroyed because MSG thinks that the other side (whose process is dead) is still using it
    }

    // leave the ring
    leave(&node);
  }

  // stop the simulation
  xbt_dynar_free(&node.comms);
  xbt_free(node.mailbox);
  xbt_free(node.pred_mailbox);
  for (i = 0; i < nb_bits - 1; i++) {
    xbt_free(node.fingers[i].mailbox);
  }
  xbt_free(node.fingers);
  return 0;
}

/**
 * \brief This function is called when the current node receives a task.
 * \param node the current node
 * \param task the task to handle (don't touch it then:
 * it will be destroyed, reused or forwarded)
 */
static void handle_task(node_t node, m_task_t task) {

  DEBUG1("Handling task %p", task);
  msg_comm_t comm = NULL;
  char* mailbox = NULL;
  task_data_t task_data = (task_data_t) MSG_task_get_data(task);
  e_task_type_t type = task_data->type;

  switch (type) {

    case TASK_FIND_SUCCESSOR:
      DEBUG2("Receiving a 'Find Successor' request from %s for id %d",
          task_data->issuer_host_name, task_data->request_id);
      // is my successor the successor?
      if (is_in_interval(task_data->request_id, node->id + 1, node->fingers[0].id)) {
        task_data->type = TASK_FIND_SUCCESSOR_ANSWER;
        task_data->answer_id = node->fingers[0].id;
        DEBUG3("Sending back a 'Find Successor Answer' to %s: the successor of %d is %d",
            task_data->issuer_host_name,
            task_data->request_id, task_data->answer_id);
        comm = MSG_task_isend(task, task_data->answer_to);
        xbt_dynar_push(node->comms, &comm);
      }
      else {
        // otherwise, forward the request to the closest preceding finger in my table
        int closest = closest_preceding_node(node, task_data->request_id);
        DEBUG2("Forwarding the 'Find Successor' request for id %d to my closest preceding finger %d",
            task_data->request_id, closest);
        mailbox = get_mailbox(closest);
        comm = MSG_task_isend(task, mailbox);
        xbt_dynar_push(node->comms, &comm);
        xbt_free(mailbox);
      }
      break;

    case TASK_GET_PREDECESSOR:
      DEBUG1("Receiving a 'Get Predecessor' request from %s", task_data->issuer_host_name);
      task_data->type = TASK_GET_PREDECESSOR_ANSWER;
      task_data->answer_id = node->pred_id;
      DEBUG3("Sending back a 'Get Predecessor Answer' to %s via mailbox '%s': my predecessor is %d",
          task_data->issuer_host_name,
          task_data->answer_to, task_data->answer_id);
      comm = MSG_task_isend(task, task_data->answer_to);
      xbt_dynar_push(node->comms, &comm);
      break;

    case TASK_NOTIFY:
      // someone is telling me that he may be my new predecessor
      DEBUG1("Receiving a 'Notify' request from %s", task_data->issuer_host_name);
      notify(node, task_data->request_id);
      task_data_destroy(task_data);
      MSG_task_destroy(task);
      break;

    case TASK_PREDECESSOR_LEAVING:
      // my predecessor is about to quit
      DEBUG1("Receiving a 'Predecessor Leaving' message from %s", task_data->issuer_host_name);
      // modify my predecessor
      set_predecessor(node, task_data->request_id);
      task_data_destroy(task_data);
      MSG_task_destroy(task);
      /*TODO :
      >> notify my new predecessor
      >> send a notify_predecessors !!
       */
      break;

    case TASK_SUCCESSOR_LEAVING:
      // my successor is about to quit
      DEBUG1("Receiving a 'Successor Leaving' message from %s", task_data->issuer_host_name);
      // modify my successor FIXME : this should be implicit ?
      set_finger(node, 0, task_data->request_id);
      task_data_destroy(task_data);
      MSG_task_destroy(task);
      /* TODO
      >> notify my new successor
      >> update my table & predecessors table */
      break;

    case TASK_FIND_SUCCESSOR_ANSWER:
    case TASK_GET_PREDECESSOR_ANSWER:
      DEBUG2("Ignoring unexpected task of type %d (%p)", type, task);
      break;
  }
}

/**
 * \brief Initializes the current node as the first one of the system.
 * \param node the current node
 */
static void create(node_t node)
{
  DEBUG0("Create a new Chord ring...");
  set_predecessor(node, -1); // -1 means that I have no predecessor
  print_finger_table(node);
}

/**
 * \brief Makes the current node join the ring, knowing the id of a node
 * already in the ring
 * \param node the current node
 * \param known_id id of a node already in the ring
 * \return 1 if the join operation succeeded, 0 otherwise
 */
static int join(node_t node, int known_id)
{
  INFO2("Joining the ring with id %d, knowing node %d", node->id, known_id);
  set_predecessor(node, -1); // no predecessor (yet)

  int successor_id = remote_find_successor(node, known_id, node->id);
  if (successor_id == -1) {
    INFO0("Cannot join the ring.");
  }
  else {
    set_finger(node, 0, successor_id);
    print_finger_table(node);
  }

  return successor_id != -1;
}

/**
 * \brief Makes the current node quit the system
 * \param node the current node
 */
static void leave(node_t node)
{
  DEBUG0("Well Guys! I Think it's time for me to quit ;)");
  quit_notify(node, 1);  // notify to my successor ( >>> 1 );
  quit_notify(node, -1); // notify my predecessor  ( >>> -1);
  // TODO ...
}

/*
 * \brief Notifies the successor or the predecessor of the current node
 * of the departure
 * \param node the current node
 * \param to 1 to notify the successor, -1 to notify the predecessor
 * FIXME: notify both nodes with only one call
 */
static void quit_notify(node_t node, int to)
{
  /* TODO
  task_data_t req_data = xbt_new0(s_task_data_t, 1);
  req_data->request_id = node->id;
  req_data->successor_id = node->fingers[0].id;
  req_data->pred_id = node->pred_id;
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());
  req_data->answer_to = NULL;
  const char* task_name = NULL;
  const char* to_mailbox = NULL;
  if (to == 1) {    // notify my successor
    to_mailbox = node->fingers[0].mailbox;
    INFO2("Telling my Successor %d about my departure via mailbox %s",
	  node->fingers[0].id, to_mailbox);
    req_data->type = TASK_PREDECESSOR_LEAVING;
  }
  else if (to == -1) {    // notify my predecessor

    if (node->pred_id == -1) {
      return;
    }

    to_mailbox = node->pred_mailbox;
    INFO2("Telling my Predecessor %d about my departure via mailbox %s",
	  node->pred_id, to_mailbox);
    req_data->type = TASK_SUCCESSOR_LEAVING;
  }
  m_task_t task = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data);
  //char* mailbox = get_mailbox(to_mailbox);
  msg_comm_t comm = MSG_task_isend(task, to_mailbox);
  xbt_dynar_push(node->comms, &comm);
  */
}

/**
 * \brief Makes the current node find the successor node of an id.
 * \param node the current node
 * \param id the id to find
 * \return the id of the successor node, or -1 if the request failed
 */
static int find_successor(node_t node, int id)
{
  // is my successor the successor?
  if (is_in_interval(id, node->id + 1, node->fingers[0].id)) {
    return node->fingers[0].id;
  }

  // otherwise, ask the closest preceding finger in my table
  int closest = closest_preceding_node(node, id);
  return remote_find_successor(node, closest, id);
}

/**
 * \brief Asks another node the successor node of an id.
 * \param node the current node
 * \param ask_to the node to ask to
 * \param id the id to find
 * \return the id of the successor node, or -1 if the request failed
 */
static int remote_find_successor(node_t node, int ask_to, int id)
{
  int successor = -1;
  int stop = 0;
  char* mailbox = get_mailbox(ask_to);
  task_data_t req_data = xbt_new0(s_task_data_t, 1);
  req_data->type = TASK_FIND_SUCCESSOR;
  req_data->request_id = id;
  req_data->answer_to = xbt_strdup(node->mailbox);
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());

  // send a "Find Successor" request to ask_to_id
  m_task_t task_sent = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data);
  DEBUG3("Sending a 'Find Successor' request (task %p) to %d for id %d", task_sent, ask_to, id);
  MSG_error_t res = MSG_task_send_with_timeout(task_sent, mailbox, timeout);

  if (res != MSG_OK) {
    DEBUG3("Failed to send the 'Find Successor' request (task %p) to %d for id %d",
        task_sent, ask_to, id);
    MSG_task_destroy(task_sent);
    task_data_destroy(req_data);
  }
  else {

    // receive the answer
    DEBUG3("Sent a 'Find Successor' request (task %p) to %d for key %d, waiting for the answer",
        task_sent, ask_to, id);

    do {
      if (node->comm_receive == NULL) {
        m_task_t task_received = NULL;
        node->comm_receive = MSG_task_irecv(&task_received, node->mailbox);
      }

      res = MSG_comm_wait(node->comm_receive, timeout);

      if (res != MSG_OK) {
        DEBUG2("Failed to receive the answer to my 'Find Successor' request (task %p): %d",
            task_sent, res);
        stop = 1;
        //MSG_comm_destroy(node->comm_receive);
      }
      else {
        m_task_t task_received = MSG_comm_get_task(node->comm_receive);
        DEBUG1("Received a task (%p)", task_received);
        task_data_t ans_data = MSG_task_get_data(task_received);

        if (task_received != task_sent) {
          // this is not the expected answer
          handle_task(node, task_received);
        }
        else {
          // this is our answer
          DEBUG4("Received the answer to my 'Find Successor' request for id %d (task %p): the successor of key %d is %d",
              ans_data->request_id, task_received, id, ans_data->answer_id);
          successor = ans_data->answer_id;
          stop = 1;
          MSG_task_destroy(task_received);
          task_data_destroy(req_data);
        }
      }
      node->comm_receive = NULL;
    } while (!stop);
  }

  xbt_free(mailbox);
  return successor;
}

/**
 * \brief Asks another node its predecessor.
 * \param node the current node
 * \param ask_to the node to ask to
 * \return the id of its predecessor node, or -1 if the request failed
 * (or if the node does not know its predecessor)
 */
static int remote_get_predecessor(node_t node, int ask_to)
{
  int predecessor_id = -1;
  int stop = 0;
  char* mailbox = get_mailbox(ask_to);
  task_data_t req_data = xbt_new0(s_task_data_t, 1);
  req_data->type = TASK_GET_PREDECESSOR;
  req_data->answer_to = xbt_strdup(node->mailbox);
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());

  // send a "Get Predecessor" request to ask_to_id
  DEBUG1("Sending a 'Get Predecessor' request to %d", ask_to);
  m_task_t task_sent = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data);
  MSG_error_t res = MSG_task_send_with_timeout(task_sent, mailbox, timeout);

  if (res != MSG_OK) {
    DEBUG2("Failed to send the 'Get Predecessor' request (task %p) to %d",
        task_sent, ask_to);
    MSG_task_destroy(task_sent);
    task_data_destroy(req_data);
  }
  else {

    // receive the answer
    DEBUG3("Sent 'Get Predecessor' request (task %p) to %d, waiting for the answer on my mailbox '%s'",
        task_sent, ask_to, req_data->answer_to);

    do {
      if (node->comm_receive == NULL) { // FIXME simplify this
        m_task_t task_received = NULL;
        node->comm_receive = MSG_task_irecv(&task_received, node->mailbox);
      }

      res = MSG_comm_wait(node->comm_receive, timeout);

      if (res != MSG_OK) {
        DEBUG2("Failed to receive the answer to my 'Get Predecessor' request (task %p): %d",
            task_sent, res);
        stop = 1;
        //MSG_comm_destroy(node->comm_receive);
      }
      else {
        m_task_t task_received = MSG_comm_get_task(node->comm_receive);
        task_data_t ans_data = MSG_task_get_data(task_received);

        if (task_received != task_sent) {
          handle_task(node, task_received);
        }
        else {
          DEBUG3("Received the answer to my 'Get Predecessor' request (task %p): the predecessor of node %d is %d",
              task_received, ask_to, ans_data->answer_id);
          predecessor_id = ans_data->answer_id;
          stop = 1;
          MSG_task_destroy(task_received);
          task_data_destroy(req_data);
        }
      }
      node->comm_receive = NULL;
    } while (!stop);
  }

  xbt_free(mailbox);
  return predecessor_id;
}

/**
 * \brief Returns the closest preceding finger of an id
 * with respect to the finger table of the current node.
 * \param node the current node
 * \param id the id to find
 * \return the closest preceding finger of that id
 */
int closest_preceding_node(node_t node, int id)
{
  int i;
  for (i = nb_bits - 1; i >= 0; i--) {
    if (is_in_interval(node->fingers[i].id, node->id + 1, id - 1)) {
      return node->fingers[i].id;
    }
  }
  return node->id;
}

/**
 * \brief This function is called periodically. It checks the immediate
 * successor of the current node.
 * \param node the current node
 */
static void stabilize(node_t node)
{
  DEBUG0("Stabilizing node");

  // get the predecessor of my immediate successor
  int candidate_id;
  int successor_id = node->fingers[0].id;
  if (successor_id != node->id) {
    candidate_id = remote_get_predecessor(node, successor_id);
  }
  else {
    candidate_id = node->pred_id;
  }

  // this node is a candidate to become my new successor
  if (candidate_id != -1
      && is_in_interval(candidate_id, node->id + 1, successor_id - 1)) {
    set_finger(node, 0, candidate_id);
  }
  if (successor_id != node->id) {
    remote_notify(node, successor_id, node->id);
  }
}

/**
 * \brief Notifies the current node that its predecessor may have changed.
 * \param node the current node
 * \param candidate_id the possible new predecessor
 */
static void notify(node_t node, int predecessor_candidate_id) {

  if (node->pred_id == -1
    || is_in_interval(predecessor_candidate_id, node->pred_id + 1, node->id - 1)) {

    set_predecessor(node, predecessor_candidate_id);
    print_finger_table(node);
  }
  else {
    DEBUG1("I don't have to change my predecessor to %d", predecessor_candidate_id);
  }
}

/**
 * \brief Notifies a remote node that its predecessor may have changed.
 * \param node the current node
 * \param notify_id id of the node to notify
 * \param candidate_id the possible new predecessor
 */
static void remote_notify(node_t node, int notify_id, int predecessor_candidate_id) {

  task_data_t req_data = xbt_new0(s_task_data_t, 1);
  req_data->type = TASK_NOTIFY;
  req_data->request_id = predecessor_candidate_id;
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());
  req_data->answer_to = NULL;

  // send a "Notify" request to notify_id
  m_task_t task = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data);
  DEBUG2("Sending a 'Notify' request (task %p) to %d", task, notify_id);
  char* mailbox = get_mailbox(notify_id);
  msg_comm_t comm = MSG_task_isend(task, mailbox);
  xbt_dynar_push(node->comms, &comm);
  xbt_free(mailbox);
}

/**
 * \brief This function is called periodically.
 * It refreshes the finger table of the current node.
 * \param node the current node
 */
static void fix_fingers(node_t node) {

  DEBUG0("Fixing fingers");
  int i = node->next_finger_to_fix;
  int id = find_successor(node, node->id + powers2[i]);
  if (id != -1) {

    if (id != node->fingers[i].id) {
      set_finger(node, i, id);
      print_finger_table(node);
    }
    node->next_finger_to_fix = (i + 1) % nb_bits;
  }
}

/**
 * \brief This function is called periodically.
 * It checks whether the predecessor has failed
 * \param node the current node
 */
static void check_predecessor(node_t node)
{
  DEBUG0("Checking whether my predecessor is alive");
  // TODO
}

/**
 * \brief Performs a find successor request to a random id.
 * \param node the current node
 */
static void random_lookup(node_t node)
{
  int id = 1337; // TODO pick a pseudorandom id
  DEBUG1("Making a lookup request for id %d", id);
  find_successor(node, id);
}

/**
 * \brief Main function.
 */
int main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("Usage: %s [-nb_bits=n] [-timeout=t] platform_file deployment_file\n", argv[0]);
    printf("example: %s ../msg_platform.xml chord.xml\n", argv[0]);
    exit(1);
  }

  MSG_global_init(&argc, argv);

  char **options = &argv[1];
  while (!strncmp(options[0], "-", 1)) {

    int length = strlen("-nb_bits=");
    if (!strncmp(options[0], "-nb_bits=", length) && strlen(options[0]) > length) {
      nb_bits = atoi(options[0] + length);
      DEBUG1("Set nb_bits to %d", nb_bits);
    }
    else {

      length = strlen("-timeout=");
      if (!strncmp(options[0], "-timeout=", length) && strlen(options[0]) > length) {
	timeout = atoi(options[0] + length);
	DEBUG1("Set timeout to %d", timeout);
      }
      else {
	xbt_assert1(0, "Invalid chord option '%s'", options[0]);
      }
    }
    options++;
  }

  const char* platform_file = options[0];
  const char* application_file = options[1];

  chord_initialize();

  MSG_set_channel_number(0);
  MSG_create_environment(platform_file);

  MSG_function_register("node", node);
  MSG_launch_application(application_file);

  MSG_error_t res = MSG_main();
  INFO1("Simulation time: %g", MSG_get_clock());

  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
