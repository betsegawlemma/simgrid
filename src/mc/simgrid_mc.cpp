  /* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <exception>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <signal.h>
#include <poll.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include <xbt/log.h>

#include "simgrid/sg_config.h"
#include "xbt_modinter.h"

#include "mc_base.h"
#include "mc_private.h"
#include "mc_protocol.h"
#include "mc_server.h"
#include "mc_safety.h"
#include "mc_comm_pattern.h"
#include "mc_liveness.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_main, mc, "Entry point for simgrid-mc");

static int do_child(int socket, char** argv)
{
  XBT_DEBUG("Inside the child process PID=%i", (int) getpid());

#ifdef __linux__
  // Make sure we do not outlive our parent:
  if (prctl(PR_SET_PDEATHSIG, SIGHUP) != 0) {
    std::perror("simgrid-mc");
    return MC_SERVER_ERROR;
  }
#endif

  int res;

  // Remove CLOEXEC in order to pass the socket to the exec-ed program:
  int fdflags = fcntl(socket, F_GETFD, 0);
  if (fdflags == -1) {
    std::perror("simgrid-mc");
    return MC_SERVER_ERROR;
  }
  if (fcntl(socket, F_SETFD, fdflags & ~FD_CLOEXEC) == -1) {
    std::perror("simgrid-mc");
    return MC_SERVER_ERROR;
  }

  XBT_DEBUG("CLOEXEC removed on socket %i", socket);

  // Set environment:
  setenv(MC_ENV_VARIABLE, "1", 1);

  char buffer[64];
  res = std::snprintf(buffer, sizeof(buffer), "%i", socket);
  if ((size_t) res >= sizeof(buffer) || res == -1)
    return MC_SERVER_ERROR;
  setenv(MC_ENV_SOCKET_FD, buffer, 1);

  execvp(argv[1], argv+1);
  std::perror("simgrid-mc");
  return MC_SERVER_ERROR;
}

static int do_parent(int socket, pid_t child)
{
  XBT_DEBUG("Inside the parent process");
  if (mc_server)
    xbt_die("MC server already present");
  try {
    mc_mode = MC_MODE_SERVER;
    mc_server = new s_mc_server(child, socket);
    mc_server->start();
    MC_init_model_checker(child, socket);
    if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
      MC_modelcheck_comm_determinism();
    else if (!_sg_mc_property_file || _sg_mc_property_file[0] == '\0')
      MC_modelcheck_safety();
    else
      MC_modelcheck_liveness();
    mc_server->shutdown();
    mc_server->exit();
  }
  catch(std::exception& e) {
    XBT_ERROR("Exception: %s", e.what());
  }
  exit(MC_SERVER_ERROR);
}

static char** argvdup(int argc, char** argv)
{
  char** argv_copy = xbt_new(char*, argc+1);
  std::memcpy(argv_copy, argv, sizeof(char*) * argc);
  argv_copy[argc] = NULL;
  return argv_copy;
}

int main(int argc, char** argv)
{
  // We need to keep the original parameters in order to pass them to the
  // model-checked process:
  int argc_copy = argc;
  char** argv_copy = argvdup(argc, argv);
  xbt_log_init(&argc_copy, argv_copy);
  sg_config_init(&argc_copy, argv_copy);

  if (argc < 2)
    xbt_die("Missing arguments.\n");

  bool server_mode = true;
  char* env = std::getenv("SIMGRID_MC_MODE");
  if (env) {
    if (std::strcmp(env, "server") == 0)
      server_mode = true;
    else if (std::strcmp(env, "standalone") == 0)
      server_mode = false;
    else
      xbt_die("Unrecognised value for SIMGRID_MC_MODE (server/standalone)");
  }

  if (!server_mode) {
    setenv(MC_ENV_VARIABLE, "1", 1);
    execvp(argv[1], argv+1);

    std::perror("simgrid-mc");
    return 127;
  }

  // Create a AF_LOCAL socketpair:
  int res;

  int sockets[2];
  res = socketpair(AF_LOCAL, SOCK_DGRAM | SOCK_CLOEXEC, 0, sockets);
  if (res == -1) {
    perror("simgrid-mc");
    return MC_SERVER_ERROR;
  }

  XBT_DEBUG("Created socketpair");

  pid_t pid = fork();
  if (pid < 0) {
    perror("simgrid-mc");
    return MC_SERVER_ERROR;
  } else if (pid == 0) {
    close(sockets[1]);
    return do_child(sockets[0], argv);
  } else {
    close(sockets[0]);
    return do_parent(sockets[1], pid);
  }

  return 0;
}
