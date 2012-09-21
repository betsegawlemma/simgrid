/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "smpi_mpi_dt_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_pmpi, smpi,
                                "Logging specific to SMPI (pmpi)");

#ifdef HAVE_TRACING
//this function need to be here because of the calls to smpi_bench
void TRACE_smpi_set_category(const char *category)
{
  //need to end bench otherwise categories for execution tasks are wrong
  smpi_bench_end();
  TRACE_internal_smpi_set_category (category);
  //begin bench after changing process's category
  smpi_bench_begin();
}
#endif

/* PMPI User level calls */

int PMPI_Init(int *argc, char ***argv)
{
  smpi_process_init(argc, argv);
#ifdef HAVE_TRACING
  int rank = smpi_process_index();
  TRACE_smpi_init(rank);

  TRACE_smpi_computing_init(rank);
#endif
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Finalize(void)
{
  smpi_process_finalize();
  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = smpi_process_index();
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_finalize(smpi_process_index());
#endif
  smpi_process_destroy();
  return MPI_SUCCESS;
}

int PMPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  if (provided != NULL) {
    *provided = MPI_THREAD_MULTIPLE;
  }
  return MPI_Init(argc, argv);
}

int PMPI_Query_thread(int *provided)
{
  int retval;

  smpi_bench_end();
  if (provided == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *provided = MPI_THREAD_MULTIPLE;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Is_thread_main(int *flag)
{
  int retval;

  smpi_bench_end();
  if (flag == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = smpi_process_index() == 0;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Abort(MPI_Comm comm, int errorcode)
{
  smpi_bench_end();
  smpi_process_destroy();
#ifdef HAVE_TRACING
  int rank = smpi_process_index();
  TRACE_smpi_computing_out(rank);
#endif
  // FIXME: should kill all processes in comm instead
  simcall_process_kill(SIMIX_process_self());
  return MPI_SUCCESS;
}

double PMPI_Wtime(void)
{
  double time;

  smpi_bench_end();
  time = SIMIX_get_clock();
  smpi_bench_begin();
  return time;
}
extern double sg_maxmin_precision;
double PMPI_Wtick(void)
{
  return sg_maxmin_precision;
}

int PMPI_Address(void *location, MPI_Aint * address)
{
  int retval;

  smpi_bench_end();
  if (!address) {
    retval = MPI_ERR_ARG;
  } else {
    *address = (MPI_Aint) location;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Type_free(MPI_Datatype * datatype)
{
  int retval;

  smpi_bench_end();
  if (!datatype) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_datatype_free(datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Type_size(MPI_Datatype datatype, int *size)
{
  int retval;

  smpi_bench_end();
  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (size == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *size = (int) smpi_datatype_size(datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  int retval;

  smpi_bench_end();
  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (lb == NULL || extent == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    retval = smpi_datatype_extent(datatype, lb, extent);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent)
{
  int retval;
  MPI_Aint dummy;

  smpi_bench_end();
  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (extent == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    retval = smpi_datatype_extent(datatype, &dummy, extent);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint * disp)
{
  int retval;

  smpi_bench_end();
  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (disp == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *disp = smpi_datatype_lb(datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint * disp)
{
  int retval;

  smpi_bench_end();
  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (disp == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *disp = smpi_datatype_ub(datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Op_create(MPI_User_function * function, int commute, MPI_Op * op)
{
  int retval;

  smpi_bench_end();
  if (function == NULL || op == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *op = smpi_op_new(function, commute);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Op_free(MPI_Op * op)
{
  int retval;

  smpi_bench_end();
  if (op == NULL) {
    retval = MPI_ERR_ARG;
  } else if (*op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    smpi_op_destroy(*op);
    *op = MPI_OP_NULL;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_free(MPI_Group * group)
{
  int retval;

  smpi_bench_end();
  if (group == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_group_destroy(*group);
    *group = MPI_GROUP_NULL;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_size(MPI_Group group, int *size)
{
  int retval;

  smpi_bench_end();
  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (size == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *size = smpi_group_size(group);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_rank(MPI_Group group, int *rank)
{
  int retval;

  smpi_bench_end();
  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (rank == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *rank = smpi_group_rank(group, smpi_process_index());
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1,
                              MPI_Group group2, int *ranks2)
{
  int retval, i, index;

  smpi_bench_end();
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else {
    for (i = 0; i < n; i++) {
      index = smpi_group_index(group1, ranks1[i]);
      ranks2[i] = smpi_group_rank(group2, index);
    }
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
  int retval;

  smpi_bench_end();
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (result == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *result = smpi_group_compare(group1, group2);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_union(MPI_Group group1, MPI_Group group2,
                    MPI_Group * newgroup)
{
  int retval, i, proc1, proc2, size, size2;

  smpi_bench_end();
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    size = smpi_group_size(group1);
    size2 = smpi_group_size(group2);
    for (i = 0; i < size2; i++) {
      proc2 = smpi_group_index(group2, i);
      proc1 = smpi_group_rank(group1, proc2);
      if (proc1 == MPI_UNDEFINED) {
        size++;
      }
    }
    if (size == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      *newgroup = smpi_group_new(size);
      size2 = smpi_group_size(group1);
      for (i = 0; i < size2; i++) {
        proc1 = smpi_group_index(group1, i);
        smpi_group_set_mapping(*newgroup, proc1, i);
      }
      for (i = size2; i < size; i++) {
        proc2 = smpi_group_index(group2, i - size2);
        smpi_group_set_mapping(*newgroup, proc2, i);
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2,
                           MPI_Group * newgroup)
{
  int retval, i, proc1, proc2, size, size2;

  smpi_bench_end();
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    size = smpi_group_size(group1);
    size2 = smpi_group_size(group2);
    for (i = 0; i < size2; i++) {
      proc2 = smpi_group_index(group2, i);
      proc1 = smpi_group_rank(group1, proc2);
      if (proc1 == MPI_UNDEFINED) {
        size--;
      }
    }
    if (size == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      *newgroup = smpi_group_new(size);
      size2 = smpi_group_size(group1);
      for (i = 0; i < size2; i++) {
        proc1 = smpi_group_index(group1, i);
        proc2 = smpi_group_rank(group2, proc1);
        if (proc2 != MPI_UNDEFINED) {
          smpi_group_set_mapping(*newgroup, proc1, i);
        }
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  int retval, i, proc1, proc2, size, size2;

  smpi_bench_end();
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    size = size2 = smpi_group_size(group1);
    for (i = 0; i < size2; i++) {
      proc1 = smpi_group_index(group1, i);
      proc2 = smpi_group_rank(group2, proc1);
      if (proc2 != MPI_UNDEFINED) {
        size--;
      }
    }
    if (size == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      *newgroup = smpi_group_new(size);
      for (i = 0; i < size2; i++) {
        proc1 = smpi_group_index(group1, i);
        proc2 = smpi_group_rank(group2, proc1);
        if (proc2 == MPI_UNDEFINED) {
          smpi_group_set_mapping(*newgroup, proc1, i);
        }
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  int retval, i, index;

  smpi_bench_end();
  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else if (n == smpi_group_size(group)) {
      *newgroup = group;
    } else {
      *newgroup = smpi_group_new(n);
      for (i = 0; i < n; i++) {
        index = smpi_group_index(group, ranks[i]);
        smpi_group_set_mapping(*newgroup, index, i);
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  int retval, i, size, rank, index;

  smpi_bench_end();
  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = group;
    } else if (n == smpi_group_size(group)) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      size = smpi_group_size(group) - n;
      *newgroup = smpi_group_new(size);
      rank = 0;
      while (rank < size) {
        for (i = 0; i < n; i++) {
          if (ranks[i] == rank) {
            break;
          }
        }
        if (i >= n) {
          index = smpi_group_index(group, rank);
          smpi_group_set_mapping(*newgroup, index, rank);
          rank++;
        }
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3],
                         MPI_Group * newgroup)
{
  int retval, i, j, rank, size, index;

  smpi_bench_end();
  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      size = 0;
      for (i = 0; i < n; i++) {
        for (rank = ranges[i][0];       /* First */
             rank >= 0 && rank <= ranges[i][1]; /* Last */
             rank += ranges[i][2] /* Stride */ ) {
          size++;
        }
      }
      if (size == smpi_group_size(group)) {
        *newgroup = group;
      } else {
        *newgroup = smpi_group_new(size);
        j = 0;
        for (i = 0; i < n; i++) {
          for (rank = ranges[i][0];     /* First */
               rank >= 0 && rank <= ranges[i][1];       /* Last */
               rank += ranges[i][2] /* Stride */ ) {
            index = smpi_group_index(group, rank);
            smpi_group_set_mapping(*newgroup, index, j);
            j++;
          }
        }
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3],
                         MPI_Group * newgroup)
{
  int retval, i, newrank, rank, size, index, add;

  smpi_bench_end();
  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = group;
    } else {
      size = smpi_group_size(group);
      for (i = 0; i < n; i++) {
        for (rank = ranges[i][0];       /* First */
             rank >= 0 && rank <= ranges[i][1]; /* Last */
             rank += ranges[i][2] /* Stride */ ) {
          size--;
        }
      }
      if (size == 0) {
        *newgroup = MPI_GROUP_EMPTY;
      } else {
        *newgroup = smpi_group_new(size);
        newrank = 0;
        while (newrank < size) {
          for (i = 0; i < n; i++) {
            add = 1;
            for (rank = ranges[i][0];   /* First */
                 rank >= 0 && rank <= ranges[i][1];     /* Last */
                 rank += ranges[i][2] /* Stride */ ) {
              if (rank == newrank) {
                add = 0;
                break;
              }
            }
            if (add == 1) {
              index = smpi_group_index(group, newrank);
              smpi_group_set_mapping(*newgroup, index, newrank);
            }
          }
        }
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Comm_rank(MPI_Comm comm, int *rank)
{
  int retval;

  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (rank == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *rank = smpi_comm_rank(comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Comm_size(MPI_Comm comm, int *size)
{
  int retval;

  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (size == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *size = smpi_comm_size(comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Comm_get_name (MPI_Comm comm, char* name, int* len)
{
  int retval;

  smpi_bench_end();
  if (comm == MPI_COMM_NULL)  {
    retval = MPI_ERR_COMM;
  } else if (name == NULL || len == NULL)  {
    retval = MPI_ERR_ARG;
  } else {
    smpi_comm_get_name(comm, name, len);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Comm_group(MPI_Comm comm, MPI_Group * group)
{
  int retval;

  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (group == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *group = smpi_comm_group(comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  int retval;

  smpi_bench_end();
  if (comm1 == MPI_COMM_NULL || comm2 == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (result == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if (comm1 == comm2) {       /* Same communicators means same groups */
      *result = MPI_IDENT;
    } else {
      *result =
          smpi_group_compare(smpi_comm_group(comm1),
                             smpi_comm_group(comm2));
      if (*result == MPI_IDENT) {
        *result = MPI_CONGRUENT;
      }
    }
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm * newcomm)
{
  int retval;

  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (newcomm == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *newcomm = smpi_comm_new(smpi_comm_group(comm));
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm)
{
  int retval;

  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newcomm == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *newcomm = smpi_comm_new(group);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Comm_free(MPI_Comm * comm)
{
  int retval;

  smpi_bench_end();
  if (comm == NULL) {
    retval = MPI_ERR_ARG;
  } else if (*comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_comm_destroy(*comm);
    *comm = MPI_COMM_NULL;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Comm_disconnect(MPI_Comm * comm)
{
  /* TODO: wait until all communication in comm are done */
  int retval;

  smpi_bench_end();
  if (comm == NULL) {
    retval = MPI_ERR_ARG;
  } else if (*comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_comm_destroy(*comm);
    *comm = MPI_COMM_NULL;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm* comm_out)
{
  int retval;

  smpi_bench_end();
  if (comm_out == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *comm_out = smpi_comm_split(comm, color, key);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Send_init(void *buf, int count, MPI_Datatype datatype, int dst,
                  int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval;

  smpi_bench_end();
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *request = smpi_mpi_send_init(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int src,
                  int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval;

  smpi_bench_end();
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *request = smpi_mpi_recv_init(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Start(MPI_Request * request)
{
  int retval;

  smpi_bench_end();
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_start(*request);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Startall(int count, MPI_Request * requests)
{
  int retval;

  smpi_bench_end();
  if (requests == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_startall(count, requests);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Request_free(MPI_Request * request)
{
  int retval;

  smpi_bench_end();
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_request_free(request);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src,
              int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  int src_traced = smpi_group_rank(smpi_comm_group(comm), src);
  TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__);
#endif
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *request = smpi_mpi_irecv(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  (*request)->recv = 1;
#endif
  smpi_bench_begin();
  return retval;
}


int PMPI_Isend(void *buf, int count, MPI_Datatype datatype, int dst,
              int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  int dst_traced = smpi_group_rank(smpi_comm_group(comm), dst);
  TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__);
  TRACE_smpi_send(rank, rank, dst_traced);
#endif
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *request = smpi_mpi_isend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  (*request)->send = 1;
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}



int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag,
             MPI_Comm comm, MPI_Status * status)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  int src_traced = smpi_group_rank(smpi_comm_group(comm), src);
  TRACE_smpi_computing_out(rank);

  TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_mpi_recv(buf, count, datatype, src, tag, comm, status);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
  if(status!=MPI_STATUS_IGNORE)src_traced = smpi_group_rank(smpi_comm_group(comm), status->MPI_SOURCE);
  TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  TRACE_smpi_recv(rank, src_traced, rank);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Send(void *buf, int count, MPI_Datatype datatype, int dst, int tag,
             MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  int dst_traced = smpi_group_rank(smpi_comm_group(comm), dst);
  TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__);
  TRACE_smpi_send(rank, rank, dst_traced);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_mpi_send(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 int dst, int sendtag, void *recvbuf, int recvcount,
                 MPI_Datatype recvtype, int src, int recvtag,
                 MPI_Comm comm, MPI_Status * status)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  int dst_traced = smpi_group_rank(smpi_comm_group(comm), dst);
  int src_traced = smpi_group_rank(smpi_comm_group(comm), src);
  TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__);
  TRACE_smpi_send(rank, rank, dst_traced);
  TRACE_smpi_send(rank, src_traced, rank);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL
             || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_mpi_sendrecv(sendbuf, sendcount, sendtype, dst, sendtag, recvbuf,
                      recvcount, recvtype, src, recvtag, comm, status);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
  TRACE_smpi_recv(rank, rank, dst_traced);
  TRACE_smpi_recv(rank, src_traced, rank);
  TRACE_smpi_computing_in(rank);

#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype,
                         int dst, int sendtag, int src, int recvtag,
                         MPI_Comm comm, MPI_Status * status)
{
  //TODO: suboptimal implementation
  void *recvbuf;
  int retval, size;

  size = smpi_datatype_size(datatype) * count;
  recvbuf = xbt_new(char, size);
  retval =
      MPI_Sendrecv(buf, count, datatype, dst, sendtag, recvbuf, count,
                   datatype, src, recvtag, comm, status);
  memcpy(buf, recvbuf, size * sizeof(char));
  xbt_free(recvbuf);
  return retval;
}

int PMPI_Test(MPI_Request * request, int *flag, MPI_Status * status)
{
  int retval;

  smpi_bench_end();
  if (request == NULL || flag == NULL) {
    retval = MPI_ERR_ARG;
  } else if (*request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_REQUEST;
  } else {
    *flag = smpi_mpi_test(request, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testany(int count, MPI_Request requests[], int *index, int *flag,
                MPI_Status * status)
{
  int retval;

  smpi_bench_end();
  if (index == NULL || flag == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = smpi_mpi_testany(count, requests, index, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testall(int count, MPI_Request* requests, int* flag, MPI_Status* statuses)
{
  int retval;

  smpi_bench_end();
  if (flag == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = smpi_mpi_testall(count, requests, statuses);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status* status) {
  int retval;
  smpi_bench_end();

  if (status == NULL) {
     retval = MPI_ERR_ARG;
  }else if (comm == MPI_COMM_NULL) {
	retval = MPI_ERR_COMM;
  } else {
	smpi_mpi_probe(source, tag, comm, status);
	retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}


int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status) {
  int retval;
  smpi_bench_end();

  if (flag == NULL) {
       retval = MPI_ERR_ARG;
    }else if (status == NULL) {
     retval = MPI_ERR_ARG;
  }else if (comm == MPI_COMM_NULL) {
	retval = MPI_ERR_COMM;
  } else {
	smpi_mpi_iprobe(source, tag, comm, flag, status);
	retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Wait(MPI_Request * request, MPI_Status * status)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = request && (*request)->comm != MPI_COMM_NULL
      ? smpi_comm_rank((*request)->comm)
      : -1;
  TRACE_smpi_computing_out(rank);

  MPI_Group group = smpi_comm_group((*request)->comm);
  int src_traced = smpi_group_rank(group, (*request)->src);
  int dst_traced = smpi_group_rank(group, (*request)->dst);
  int is_wait_for_receive = (*request)->recv;
  TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__);
#endif
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (*request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_REQUEST;
  } else {
    smpi_mpi_wait(request, status);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
  if (is_wait_for_receive) {
    TRACE_smpi_recv(rank, src_traced, dst_traced);
  }
  TRACE_smpi_computing_in(rank);

#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Waitany(int count, MPI_Request requests[], int *index, MPI_Status * status)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  //save requests information for tracing
  int i;
  xbt_dynar_t srcs = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_t dsts = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_t recvs = xbt_dynar_new(sizeof(int), NULL);
  for (i = 0; i < count; i++) {
    MPI_Request req = requests[i];      //already received requests are no longer valid
    if (req) {
      int *asrc = xbt_new(int, 1);
      int *adst = xbt_new(int, 1);
      int *arecv = xbt_new(int, 1);
      *asrc = req->src;
      *adst = req->dst;
      *arecv = req->recv;
      xbt_dynar_insert_at(srcs, i, asrc);
      xbt_dynar_insert_at(dsts, i, adst);
      xbt_dynar_insert_at(recvs, i, arecv);
      xbt_free(asrc);
      xbt_free(adst);
      xbt_free(arecv);
    } else {
      int *t = xbt_new(int, 1);
      xbt_dynar_insert_at(srcs, i, t);
      xbt_dynar_insert_at(dsts, i, t);
      xbt_dynar_insert_at(recvs, i, t);
      xbt_free(t);
    }
  }
  int rank_traced = smpi_comm_rank(MPI_COMM_WORLD);
  TRACE_smpi_computing_out(rank_traced);

  TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__);

#endif
  if (index == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *index = smpi_mpi_waitany(count, requests, status);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  int src_traced, dst_traced, is_wait_for_receive;
  xbt_dynar_get_cpy(srcs, *index, &src_traced);
  xbt_dynar_get_cpy(dsts, *index, &dst_traced);
  xbt_dynar_get_cpy(recvs, *index, &is_wait_for_receive);
  if (is_wait_for_receive) {
    TRACE_smpi_recv(rank_traced, src_traced, dst_traced);
  }
  TRACE_smpi_ptp_out(rank_traced, src_traced, dst_traced, __FUNCTION__);
  //clean-up of dynars
  xbt_dynar_free(&srcs);
  xbt_dynar_free(&dsts);
  xbt_dynar_free(&recvs);
  TRACE_smpi_computing_in(rank_traced);

#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Waitall(int count, MPI_Request requests[], MPI_Status status[])
{

  smpi_bench_end();
#ifdef HAVE_TRACING
  //save information from requests
  int i;
  xbt_dynar_t srcs = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_t dsts = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_t recvs = xbt_dynar_new(sizeof(int), NULL);
  for (i = 0; i < count; i++) {
    MPI_Request req = requests[i];      //all req should be valid in Waitall
    int *asrc = xbt_new(int, 1);
    int *adst = xbt_new(int, 1);
    int *arecv = xbt_new(int, 1);
    *asrc = req->src;
    *adst = req->dst;
    *arecv = req->recv;
    xbt_dynar_insert_at(srcs, i, asrc);
    xbt_dynar_insert_at(dsts, i, adst);
    xbt_dynar_insert_at(recvs, i, arecv);
    xbt_free(asrc);
    xbt_free(adst);
    xbt_free(arecv);
  }
  int rank_traced = smpi_comm_rank (MPI_COMM_WORLD);
  TRACE_smpi_computing_out(rank_traced);

  TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__);
#endif
  smpi_mpi_waitall(count, requests, status);
#ifdef HAVE_TRACING
  for (i = 0; i < count; i++) {
    int src_traced, dst_traced, is_wait_for_receive;
    xbt_dynar_get_cpy(srcs, i, &src_traced);
    xbt_dynar_get_cpy(dsts, i, &dst_traced);
    xbt_dynar_get_cpy(recvs, i, &is_wait_for_receive);
    if (is_wait_for_receive) {
      TRACE_smpi_recv(rank_traced, src_traced, dst_traced);
    }
  }
  TRACE_smpi_ptp_out(rank_traced, -1, -1, __FUNCTION__);
  //clean-up of dynars
  xbt_dynar_free(&srcs);
  xbt_dynar_free(&dsts);
  xbt_dynar_free(&recvs);
  TRACE_smpi_computing_in(rank_traced);
#endif
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Waitsome(int incount, MPI_Request requests[], int *outcount,
                 int *indices, MPI_Status status[])
{
  int retval;

  smpi_bench_end();
  if (outcount == NULL || indices == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *outcount = smpi_mpi_waitsome(incount, requests, indices, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  int root_traced = smpi_group_rank(smpi_comm_group(comm), root);
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_mpi_bcast(buf, count, datatype, root, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Barrier(MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_mpi_barrier(comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype,
               int root, MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  int root_traced = smpi_group_rank(smpi_comm_group(comm), root);
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL
             || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                    recvtype, root, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int *recvcounts, int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  int root_traced = smpi_group_rank(smpi_comm_group(comm), root);
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL
             || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (recvcounts == NULL || displs == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                     displs, recvtype, root, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL
             || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_mpi_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                       recvtype, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int *recvcounts, int *displs,
                   MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL
             || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (recvcounts == NULL || displs == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                        displs, recvtype, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  int root_traced = smpi_group_rank(smpi_comm_group(comm), root);
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL
             || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                     recvtype, root, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Scatterv(void *sendbuf, int *sendcounts, int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount,
                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  int root_traced = smpi_group_rank(smpi_comm_group(comm), root);
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL
             || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (sendcounts == NULL || displs == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                      recvcount, recvtype, root, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce(void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  int root_traced = smpi_group_rank(smpi_comm_group(comm), root);
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (datatype == MPI_DATATYPE_NULL || op == MPI_OP_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Allreduce(void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    smpi_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Scan(void *sendbuf, void *recvbuf, int count,
             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    smpi_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts,
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval, i, size, count;
  int *displs;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end();
#ifdef HAVE_TRACING
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (recvcounts == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    /* arbitrarily choose root as rank 0 */
    /* TODO: faster direct implementation ? */
    size = smpi_comm_size(comm);
    count = 0;
    displs = xbt_new(int, size);
    for (i = 0; i < size; i++) {
      count += recvcounts[i];
      displs[i] = 0;
    }
    smpi_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
    smpi_mpi_scatterv(recvbuf, recvcounts, displs, datatype, recvbuf,
                      recvcounts[rank], datatype, 0, comm);
    xbt_free(displs);
    retval = MPI_SUCCESS;
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 MPI_Comm comm)
{
  int retval, size, sendsize;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL
             || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    size = smpi_comm_size(comm);
    sendsize = smpi_datatype_size(sendtype) * sendcount;
    if (sendsize < 200 && size > 12) {
      retval =
          smpi_coll_tuned_alltoall_bruck(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype,
                                         comm);
    } else if (sendsize < 3000) {
      retval =
          smpi_coll_tuned_alltoall_basic_linear(sendbuf, sendcount,
                                                sendtype, recvbuf,
                                                recvcount, recvtype, comm);
    } else {
      retval =
          smpi_coll_tuned_alltoall_pairwise(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype,
                                            comm);
    }
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoallv(void *sendbuf, int *sendcounts, int *senddisps,
                  MPI_Datatype sendtype, void *recvbuf, int *recvcounts,
                  int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval;

  smpi_bench_end();
#ifdef HAVE_TRACING
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;
  TRACE_smpi_computing_out(rank);
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__);
#endif
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL
             || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (sendcounts == NULL || senddisps == NULL || recvcounts == NULL
             || recvdisps == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    retval =
        smpi_coll_basic_alltoallv(sendbuf, sendcounts, senddisps, sendtype,
                                  recvbuf, recvcounts, recvdisps, recvtype,
                                  comm);
  }
#ifdef HAVE_TRACING
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_computing_in(rank);
#endif
  smpi_bench_begin();
  return retval;
}


int PMPI_Get_processor_name(char *name, int *resultlen)
{
  int retval = MPI_SUCCESS;

  smpi_bench_end();
  strncpy(name, SIMIX_host_get_name(SIMIX_host_self()),
          MPI_MAX_PROCESSOR_NAME - 1);
  *resultlen =
      strlen(name) >
      MPI_MAX_PROCESSOR_NAME ? MPI_MAX_PROCESSOR_NAME : strlen(name);

  smpi_bench_begin();
  return retval;
}

int PMPI_Get_count(MPI_Status * status, MPI_Datatype datatype, int *count)
{
  int retval = MPI_SUCCESS;
  size_t size;

  smpi_bench_end();
  if (status == NULL || count == NULL) {
    retval = MPI_ERR_ARG;
  } else if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    size = smpi_datatype_size(datatype);
    if (size == 0) {
      *count = 0;
    } else if (status->count % size != 0) {
      retval = MPI_UNDEFINED;
    } else {
      *count = smpi_mpi_get_count(status, datatype);
    }
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Type_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval;

  smpi_bench_end();
  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<=0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_contiguous(count, old_type, new_type);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Type_commit(MPI_Datatype* datatype) {
  int retval;

  smpi_bench_end();
  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_datatype_commit(datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}


int PMPI_Type_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval;

  smpi_bench_end();
  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<=0 || blocklen<=0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_vector(count, blocklen, stride, old_type, new_type);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Type_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval;

  smpi_bench_end();
  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<=0 || blocklen<=0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_hvector(count, blocklen, stride, old_type, new_type);
  }
  smpi_bench_begin();
  return retval;
}


int PMPI_Type_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval;

  smpi_bench_end();
  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<=0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_indexed(count, blocklens, indices, old_type, new_type);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Type_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval;

  smpi_bench_end();
  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<=0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_hindexed(count, blocklens, indices, old_type, new_type);
  }
  smpi_bench_begin();
  return retval;
}


int PMPI_Type_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* new_type) {
  int retval;

  smpi_bench_end();
  if (count<=0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_struct(count, blocklens, indices, old_types, new_type);
  }
  smpi_bench_begin();
  return retval;}


/* The following calls are not yet implemented and will fail at runtime. */
/* Once implemented, please move them above this notice. */

static int not_yet_implemented(void) {
	  XBT_WARN("Not yet implemented");
   return MPI_SUCCESS;
}

int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int* size) {
   return not_yet_implemented();
}

int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int* coords) {
   return not_yet_implemented();
}

int PMPI_Cart_create(MPI_Comm comm_old, int ndims, int* dims, int* periods, int reorder, MPI_Comm* comm_cart) {
   return not_yet_implemented();
}

int PMPI_Cart_get(MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords) {
   return not_yet_implemented();
}

int PMPI_Cart_map(MPI_Comm comm_old, int ndims, int* dims, int* periods, int* newrank) {
   return not_yet_implemented();
}

int PMPI_Cart_rank(MPI_Comm comm, int* coords, int* rank) {
   return not_yet_implemented();
}

int PMPI_Cart_shift(MPI_Comm comm, int direction, int displ, int* source, int* dest) {
   return not_yet_implemented();
}

int PMPI_Cart_sub(MPI_Comm comm, int* remain_dims, MPI_Comm* comm_new) {
   return not_yet_implemented();
}

int PMPI_Cartdim_get(MPI_Comm comm, int* ndims) {
   return not_yet_implemented();
}

int PMPI_Graph_create(MPI_Comm comm_old, int nnodes, int* index, int* edges, int reorder, MPI_Comm* comm_graph) {
   return not_yet_implemented();
}

int PMPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int* index, int* edges) {
   return not_yet_implemented();
}

int PMPI_Graph_map(MPI_Comm comm_old, int nnodes, int* index, int* edges, int* newrank) {
   return not_yet_implemented();
}

int PMPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int* neighbors) {
   return not_yet_implemented();
}

int PMPI_Graph_neighbors_count(MPI_Comm comm, int rank, int* nneighbors) {
   return not_yet_implemented();
}

int PMPI_Graphdims_get(MPI_Comm comm, int* nnodes, int* nedges) {
   return not_yet_implemented();
}

int PMPI_Topo_test(MPI_Comm comm, int* top_type) {
   return not_yet_implemented();
}

int PMPI_Error_class(int errorcode, int* errorclass) {
   return not_yet_implemented();
}

int PMPI_Errhandler_create(MPI_Handler_function* function, MPI_Errhandler* errhandler) {
   return not_yet_implemented();
}

int PMPI_Errhandler_free(MPI_Errhandler* errhandler) {
   return not_yet_implemented();
}

int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler* errhandler) {
   return not_yet_implemented();
}

int PMPI_Error_string(int errorcode, char* string, int* resultlen) {
   return not_yet_implemented();
}

int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler) {
   return not_yet_implemented();
}


int PMPI_Cancel(MPI_Request* request) {
   return not_yet_implemented();
}

int PMPI_Buffer_attach(void* buffer, int size) {
   return not_yet_implemented();
}

int PMPI_Buffer_detach(void* buffer, int* size) {
   return not_yet_implemented();
}

int PMPI_Testsome(int incount, MPI_Request* requests, int* outcount, int* indices, MPI_Status* statuses) {
   return not_yet_implemented();
}

int PMPI_Comm_test_inter(MPI_Comm comm, int* flag) {
   return not_yet_implemented();
}

int PMPI_Unpack(void* inbuf, int insize, int* position, void* outbuf, int outcount, MPI_Datatype type, MPI_Comm comm) {
   return not_yet_implemented();
}

int PMPI_Ssend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
   return not_yet_implemented();
}

int PMPI_Ssend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
   return not_yet_implemented();
}

int PMPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm* comm_out) {
   return not_yet_implemented();
}

int PMPI_Intercomm_merge(MPI_Comm comm, int high, MPI_Comm* comm_out) {
   return not_yet_implemented();
}

int PMPI_Bsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
   return not_yet_implemented();
}

int PMPI_Bsend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
   return not_yet_implemented();
}

int PMPI_Ibsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
   return not_yet_implemented();
}

int PMPI_Comm_remote_group(MPI_Comm comm, MPI_Group* group) {
   return not_yet_implemented();
}

int PMPI_Comm_remote_size(MPI_Comm comm, int* size) {
   return not_yet_implemented();
}

int PMPI_Issend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
   return not_yet_implemented();
}


int PMPI_Attr_delete(MPI_Comm comm, int keyval) {
   return not_yet_implemented();
}

int PMPI_Attr_get(MPI_Comm comm, int keyval, void* attr_value, int* flag) {
   return not_yet_implemented();
}

int PMPI_Attr_put(MPI_Comm comm, int keyval, void* attr_value) {
   return not_yet_implemented();
}

int PMPI_Rsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
   return not_yet_implemented();
}

int PMPI_Rsend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
   return not_yet_implemented();
}

int PMPI_Irsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
   return not_yet_implemented();
}

int PMPI_Keyval_create(MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state) {
   return not_yet_implemented();
}

int PMPI_Keyval_free(int* keyval) {
   return not_yet_implemented();
}

int PMPI_Test_cancelled(MPI_Status* status, int* flag) {
   return not_yet_implemented();
}

int PMPI_Pack(void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position, MPI_Comm comm) {
   return not_yet_implemented();
}

int PMPI_Get_elements(MPI_Status* status, MPI_Datatype datatype, int* elements) {
   return not_yet_implemented();
}

int PMPI_Dims_create(int nnodes, int ndims, int* dims) {
   return not_yet_implemented();
}

int PMPI_Initialized(int* flag) {
   return not_yet_implemented();
}
