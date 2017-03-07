/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/host.hpp>
#include <xbt/ex.hpp>

#include "private.h"
#include "smpi_mpi_dt_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_pmpi, smpi, "Logging specific to SMPI (pmpi)");

//this function need to be here because of the calls to smpi_bench
void TRACE_smpi_set_category(const char *category)
{
  //need to end bench otherwise categories for execution tasks are wrong
  smpi_bench_end();
  TRACE_internal_smpi_set_category (category);
  //begin bench after changing process's category
  smpi_bench_begin();
}

/* PMPI User level calls */
extern "C" { // Obviously, the C MPI interface should use the C linkage

int PMPI_Init(int *argc, char ***argv)
{
  // PMPI_Init is call only one time by only by SMPI process
  int already_init;
  MPI_Initialized(&already_init);
  if(already_init == 0){
    smpi_process_init(argc, argv);
    smpi_process_mark_as_initialized();
    int rank = smpi_process_index();
    TRACE_smpi_init(rank);
    TRACE_smpi_computing_init(rank);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_INIT;
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
    smpi_bench_begin();
  }

  smpi_mpi_init();

  return MPI_SUCCESS;
}

int PMPI_Finalize()
{
  smpi_bench_end();
  int rank = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_FINALIZE;
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

  smpi_process_finalize();

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_finalize(smpi_process_index());
  smpi_process_destroy();
  return MPI_SUCCESS;
}

int PMPI_Finalized(int* flag)
{
  *flag=smpi_process_finalized();
  return MPI_SUCCESS;
}

int PMPI_Get_version (int *version,int *subversion){
  *version = MPI_VERSION;
  *subversion= MPI_SUBVERSION;
  return MPI_SUCCESS;
}

int PMPI_Get_library_version (char *version,int *len){
  smpi_bench_end();
  snprintf(version,MPI_MAX_LIBRARY_VERSION_STRING,"SMPI Version %d.%d. Copyright The Simgrid Team 2007-2015",
           SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR);
  *len = strlen(version) > MPI_MAX_LIBRARY_VERSION_STRING ? MPI_MAX_LIBRARY_VERSION_STRING : strlen(version);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  if (provided != nullptr) {
    *provided = MPI_THREAD_SINGLE;
  }
  return MPI_Init(argc, argv);
}

int PMPI_Query_thread(int *provided)
{
  if (provided == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *provided = MPI_THREAD_SINGLE;
    return MPI_SUCCESS;
  }
}

int PMPI_Is_thread_main(int *flag)
{
  if (flag == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *flag = smpi_process_index() == 0;
    return MPI_SUCCESS;
  }
}

int PMPI_Abort(MPI_Comm comm, int errorcode)
{
  smpi_bench_end();
  smpi_process_destroy();
  // FIXME: should kill all processes in comm instead
  simcall_process_kill(SIMIX_process_self());
  return MPI_SUCCESS;
}

double PMPI_Wtime()
{
  return smpi_mpi_wtime();
}

extern double sg_maxmin_precision;
double PMPI_Wtick()
{
  return sg_maxmin_precision;
}

int PMPI_Address(void *location, MPI_Aint * address)
{
  if (address==nullptr) {
    return MPI_ERR_ARG;
  } else {
    *address = reinterpret_cast<MPI_Aint>(location);
    return MPI_SUCCESS;
  }
}

int PMPI_Get_address(void *location, MPI_Aint * address)
{
  return PMPI_Address(location, address);
}

int PMPI_Type_free(MPI_Datatype * datatype)
{
  /* Free a predefined datatype is an error according to the standard, and should be checked for */
  if (*datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_ARG;
  } else {
    smpi_datatype_unuse(*datatype);
    return MPI_SUCCESS;
  }
}

int PMPI_Type_size(MPI_Datatype datatype, int *size)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (size == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *size = static_cast<int>(smpi_datatype_size(datatype));
    return MPI_SUCCESS;
  }
}

int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (size == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *size = static_cast<MPI_Count>(smpi_datatype_size(datatype));
    return MPI_SUCCESS;
  }
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (lb == nullptr || extent == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return smpi_datatype_extent(datatype, lb, extent);
  }
}

int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  return PMPI_Type_get_extent(datatype, lb, extent);
}

int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (extent == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *extent = smpi_datatype_get_extent(datatype);
    return MPI_SUCCESS;
  }
}

int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint * disp)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (disp == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *disp = smpi_datatype_lb(datatype);
    return MPI_SUCCESS;
  }
}

int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint * disp)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (disp == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *disp = smpi_datatype_ub(datatype);
    return MPI_SUCCESS;
  }
}

int PMPI_Type_dup(MPI_Datatype datatype, MPI_Datatype *newtype){
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else {
    return smpi_datatype_dup(datatype, newtype);
  }
}

int PMPI_Op_create(MPI_User_function * function, int commute, MPI_Op * op)
{
  if (function == nullptr || op == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *op = smpi_op_new(function, (commute!=0));
    return MPI_SUCCESS;
  }
}

int PMPI_Op_free(MPI_Op * op)
{
  if (op == nullptr) {
    return MPI_ERR_ARG;
  } else if (*op == MPI_OP_NULL) {
    return MPI_ERR_OP;
  } else {
    smpi_op_destroy(*op);
    *op = MPI_OP_NULL;
    return MPI_SUCCESS;
  }
}

int PMPI_Group_free(MPI_Group * group)
{
  if (group == nullptr) {
    return MPI_ERR_ARG;
  } else {
    (*group)->destroy();
    *group = MPI_GROUP_NULL;
    return MPI_SUCCESS;
  }
}

int PMPI_Group_size(MPI_Group group, int *size)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (size == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *size = group->size();
    return MPI_SUCCESS;
  }
}

int PMPI_Group_rank(MPI_Group group, int *rank)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (rank == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *rank = group->rank(smpi_process_index());
    return MPI_SUCCESS;
  }
}

int PMPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2)
{
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else {
    for (int i = 0; i < n; i++) {
      if(ranks1[i]==MPI_PROC_NULL){
        ranks2[i]=MPI_PROC_NULL;
      }else{
        int index = group1->index(ranks1[i]);
        ranks2[i] = group2->rank(index);
      }
    }
    return MPI_SUCCESS;
  }
}

int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (result == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *result = group1->compare(group2);
    return MPI_SUCCESS;
  }
}

int PMPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{

  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return group1->group_union(group2, newgroup);
  }
}

int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{

  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return group1->intersection(group2,newgroup);
  }
}

int PMPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return group1->difference(group2,newgroup);
  }
}

int PMPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return group->incl(n, ranks, newgroup);
  }
}

int PMPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = group;
      if (group != MPI_COMM_WORLD->group()
                && group != MPI_COMM_SELF->group() && group != MPI_GROUP_EMPTY)
      group->use();
      return MPI_SUCCESS;
    } else if (n == group->size()) {
      *newgroup = MPI_GROUP_EMPTY;
      return MPI_SUCCESS;
    } else {
      return group->excl(n,ranks,newgroup);
    }
  }
}

int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = MPI_GROUP_EMPTY;
      return MPI_SUCCESS;
    } else {
      return group->range_incl(n,ranges,newgroup);
    }
  }
}

int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup)
{
  if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newgroup == nullptr) {
    return MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = group;
      if (group != MPI_COMM_WORLD->group() && group != MPI_COMM_SELF->group() &&
          group != MPI_GROUP_EMPTY)
        group->use();
      return MPI_SUCCESS;
    } else {
      return group->range_excl(n,ranges,newgroup);
    }
  }
}

int PMPI_Comm_rank(MPI_Comm comm, int *rank)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (rank == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *rank = comm->rank();
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_size(MPI_Comm comm, int *size)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (size == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *size = comm->size();
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_get_name (MPI_Comm comm, char* name, int* len)
{
  if (comm == MPI_COMM_NULL)  {
    return MPI_ERR_COMM;
  } else if (name == nullptr || len == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    comm->get_name(name, len);
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_group(MPI_Comm comm, MPI_Group * group)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (group == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *group = comm->group();
    if (*group != MPI_COMM_WORLD->group() && *group != MPI_GROUP_NULL && *group != MPI_GROUP_EMPTY)
      (*group)->use();
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  if (comm1 == MPI_COMM_NULL || comm2 == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (result == nullptr) {
    return MPI_ERR_ARG;
  } else {
    if (comm1 == comm2) {       /* Same communicators means same groups */
      *result = MPI_IDENT;
    } else {
      *result = comm1->group()->compare(comm2->group());
      if (*result == MPI_IDENT) {
        *result = MPI_CONGRUENT;
      }
    }
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm * newcomm)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (newcomm == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return comm->dup(newcomm);
  }
}

int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (group == MPI_GROUP_NULL) {
    return MPI_ERR_GROUP;
  } else if (newcomm == nullptr) {
    return MPI_ERR_ARG;
  } else if(group->rank(smpi_process_index())==MPI_UNDEFINED){
    *newcomm= MPI_COMM_NULL;
    return MPI_SUCCESS;
  }else{
    group->use();
    *newcomm = new simgrid::smpi::Comm(group, nullptr);
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_free(MPI_Comm * comm)
{
  if (comm == nullptr) {
    return MPI_ERR_ARG;
  } else if (*comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else {
    (*comm)->destroy();
    *comm = MPI_COMM_NULL;
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_disconnect(MPI_Comm * comm)
{
  /* TODO: wait until all communication in comm are done */
  if (comm == nullptr) {
    return MPI_ERR_ARG;
  } else if (*comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else {
    (*comm)->destroy();
    *comm = MPI_COMM_NULL;
    return MPI_SUCCESS;
  }
}

int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm* comm_out)
{
  int retval = 0;
  smpi_bench_end();

  if (comm_out == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *comm_out = comm->split(color, key);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();

  return retval;
}

int PMPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int, MPI_Comm* comm_out)
{
  int retval = 0;
  smpi_bench_end();

  if (comm_out == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    retval = MPI_Comm_create(comm, group, comm_out);
  }
  smpi_bench_begin();

  return retval;
}

int PMPI_Send_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
      retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
      retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if (dst == MPI_PROC_NULL) {
      retval = MPI_SUCCESS;
  } else {
      *request = smpi_mpi_send_init(buf, count, datatype, dst, tag, comm);
      retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if (src == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else {
    *request = smpi_mpi_recv_init(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Ssend_init(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else {
    *request = smpi_mpi_ssend_init(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Start(MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr || *request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_REQUEST;
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
  if (requests == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    retval = MPI_SUCCESS;
    for (int i = 0; i < count; i++) {
      if(requests[i] == MPI_REQUEST_NULL) {
        retval = MPI_ERR_REQUEST;
      }
    }
    if(retval != MPI_ERR_REQUEST) {
      smpi_mpi_startall(count, requests);
    }
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Request_free(MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (*request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_request_free(request);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();

  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (src == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0)){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {

    int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int src_traced = comm->group()->index(src);

    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_IRECV;
    extra->src = src_traced;
    extra->dst = rank;
    int known=0;
    extra->datatype1 = encode_datatype(datatype, &known);
    int dt_size_send = 1;
    if(known==0)
      dt_size_send = smpi_datatype_size(datatype);
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);

    *request = smpi_mpi_irecv(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
    (*request)->recv = 1;
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}


int PMPI_Isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int dst_traced = comm->group()->index(dst);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_ISEND;
    extra->src = rank;
    extra->dst = dst_traced;
    int known=0;
    extra->datatype1 = encode_datatype(datatype, &known);
    int dt_size_send = 1;
    if(known==0)
      dt_size_send = smpi_datatype_size(datatype);
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
    TRACE_smpi_send(rank, rank, dst_traced, tag, count*smpi_datatype_size(datatype));

    *request = smpi_mpi_isend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
    (*request)->send = 1;
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request!=nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Issend(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0)|| (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int dst_traced = comm->group()->index(dst);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_ISSEND;
    extra->src = rank;
    extra->dst = dst_traced;
    int known=0;
    extra->datatype1 = encode_datatype(datatype, &known);
    int dt_size_send = 1;
    if(known==0)
      dt_size_send = smpi_datatype_size(datatype);
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
    TRACE_smpi_send(rank, rank, dst_traced, tag, count*smpi_datatype_size(datatype));

    *request = smpi_mpi_issend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
    (*request)->send = 1;
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request!=nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (src == MPI_PROC_NULL) {
    smpi_empty_status(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else if (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0)){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int src_traced         = comm->group()->index(src);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_RECV;
    extra->src             = src_traced;
    extra->dst             = rank;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = smpi_datatype_size(datatype);
    extra->send_size = count * dt_size_send;
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);

    smpi_mpi_recv(buf, count, datatype, src, tag, comm, status);
    retval = MPI_SUCCESS;

    // the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    if (status != MPI_STATUS_IGNORE) {
      src_traced = comm->group()->index(status->MPI_SOURCE);
      if (!TRACE_smpi_view_internals()) {
        TRACE_smpi_recv(rank, src_traced, rank, tag);
      }
    }
    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf == nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if(tag < 0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int dst_traced         = comm->group()->index(dst);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type            = TRACING_SEND;
    extra->src             = rank;
    extra->dst             = dst_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0) {
      dt_size_send = smpi_datatype_size(datatype);
    }
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
    if (!TRACE_smpi_view_internals()) {
      TRACE_smpi_send(rank, rank, dst_traced, tag,count*smpi_datatype_size(datatype));
    }

    smpi_mpi_send(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Ssend(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm) {
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)){
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int dst_traced         = comm->group()->index(dst);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type            = TRACING_SSEND;
    extra->src             = rank;
    extra->dst             = dst_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if(known == 0) {
      dt_size_send = smpi_datatype_size(datatype);
    }
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
    TRACE_smpi_send(rank, rank, dst_traced, tag,count*smpi_datatype_size(datatype));
  
    smpi_mpi_ssend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  
    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(sendtype) || !is_datatype_valid(recvtype)) {
    retval = MPI_ERR_TYPE;
  } else if (src == MPI_PROC_NULL || dst == MPI_PROC_NULL) {
    smpi_empty_status(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval             = MPI_SUCCESS;
  }else if (dst >= comm->group()->size() || dst <0 ||
      (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0))){
    retval = MPI_ERR_RANK;
  } else if ((sendcount < 0 || recvcount<0) || 
      (sendbuf==nullptr && sendcount > 0) || (recvbuf==nullptr && recvcount>0)) {
    retval = MPI_ERR_COUNT;
  } else if((sendtag<0 && sendtag !=  MPI_ANY_TAG)||(recvtag<0 && recvtag != MPI_ANY_TAG)){
    retval = MPI_ERR_TAG;
  } else {

  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int dst_traced = comm->group()->index(dst);
  int src_traced = comm->group()->index(src);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_SENDRECV;
  extra->src = src_traced;
  extra->dst = dst_traced;
  int known=0;
  extra->datatype1 = encode_datatype(sendtype, &known);
  int dt_size_send = 1;
  if(known==0)
    dt_size_send = smpi_datatype_size(sendtype);
  extra->send_size = sendcount*dt_size_send;
  extra->datatype2 = encode_datatype(recvtype, &known);
  int dt_size_recv = 1;
  if(known==0)
    dt_size_recv = smpi_datatype_size(recvtype);
  extra->recv_size = recvcount*dt_size_recv;

  TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__, extra);
  TRACE_smpi_send(rank, rank, dst_traced, sendtag,sendcount*smpi_datatype_size(sendtype));

  smpi_mpi_sendrecv(sendbuf, sendcount, sendtype, dst, sendtag, recvbuf, recvcount, recvtype, src, recvtag, comm,
                    status);
  retval = MPI_SUCCESS;

  TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
  TRACE_smpi_recv(rank, src_traced, rank, recvtag);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Sendrecv_replace(void* buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src, int recvtag,
                          MPI_Comm comm, MPI_Status* status)
{
  int retval = 0;
  if (!is_datatype_valid(datatype)) {
    return MPI_ERR_TYPE;
  } else if (count < 0) {
    return MPI_ERR_COUNT;
  } else {
    int size = smpi_datatype_get_extent(datatype) * count;
    void* recvbuf = xbt_new0(char, size);
    retval = MPI_Sendrecv(buf, count, datatype, dst, sendtag, recvbuf, count, datatype, src, recvtag, comm, status);
    if(retval==MPI_SUCCESS){
        smpi_datatype_copy(recvbuf, count, datatype, buf, count, datatype);
    }
    xbt_free(recvbuf);

  }
  return retval;
}

int PMPI_Test(MPI_Request * request, int *flag, MPI_Status * status)
{
  int retval = 0;
  smpi_bench_end();
  if (request == nullptr || flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (*request == MPI_REQUEST_NULL) {
    *flag= true;
    smpi_empty_status(status);
    retval = MPI_SUCCESS;
  } else {
    int rank = ((*request)->comm != MPI_COMM_NULL) ? smpi_process_index() : -1;

    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_TEST;
    TRACE_smpi_testing_in(rank, extra);

    *flag = smpi_mpi_test(request, status);

    TRACE_smpi_testing_out(rank);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testany(int count, MPI_Request requests[], int *index, int *flag, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();
  if (index == nullptr || flag == nullptr) {
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
  int retval = 0;

  smpi_bench_end();
  if (flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = smpi_mpi_testall(count, requests, statuses);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status* status) {
  int retval = 0;
  smpi_bench_end();

  if (status == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (source == MPI_PROC_NULL) {
    smpi_empty_status(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else {
    smpi_mpi_probe(source, tag, comm, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status) {
  int retval = 0;
  smpi_bench_end();

  if ((flag == nullptr) || (status == nullptr)) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (source == MPI_PROC_NULL) {
    *flag=true;
    smpi_empty_status(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else {
    smpi_mpi_iprobe(source, tag, comm, flag, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Wait(MPI_Request * request, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();

  smpi_empty_status(status);

  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (*request == MPI_REQUEST_NULL) {
    retval = MPI_SUCCESS;
  } else {

    int rank = (request!=nullptr && (*request)->comm != MPI_COMM_NULL) ? smpi_process_index() : -1;

    int src_traced = (*request)->src;
    int dst_traced = (*request)->dst;
    int tag_traced= (*request)->tag;
    MPI_Comm comm = (*request)->comm;
    int is_wait_for_receive = (*request)->recv;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_WAIT;
    TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__, extra);

    smpi_mpi_wait(request, status);
    retval = MPI_SUCCESS;

    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
    if (is_wait_for_receive) {
      if(src_traced==MPI_ANY_SOURCE)
        src_traced = (status!=MPI_STATUS_IGNORE) ?
          comm->group()->rank(status->MPI_SOURCE) :
          src_traced;
      TRACE_smpi_recv(rank, src_traced, dst_traced, tag_traced);
    }
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Waitany(int count, MPI_Request requests[], int *index, MPI_Status * status)
{
  if (index == nullptr)
    return MPI_ERR_ARG;

  smpi_bench_end();
  //save requests information for tracing
  typedef struct {
    int src;
    int dst;
    int recv;
    int tag;
    MPI_Comm comm;
  } savedvalstype;
  savedvalstype* savedvals=nullptr;
  if(count>0){
    savedvals = xbt_new0(savedvalstype, count);
  }
  for (int i = 0; i < count; i++) {
    MPI_Request req = requests[i];      //already received requests are no longer valid
    if (req) {
      savedvals[i]=(savedvalstype){req->src, req->dst, req->recv, req->tag, req->comm};
    }
  }
  int rank_traced = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_WAITANY;
  extra->send_size=count;
  TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__,extra);

  *index = smpi_mpi_waitany(count, requests, status);

  if(*index!=MPI_UNDEFINED){
    int src_traced = savedvals[*index].src;
    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    int dst_traced = savedvals[*index].dst;
    int is_wait_for_receive = savedvals[*index].recv;
    if (is_wait_for_receive) {
      if(savedvals[*index].src==MPI_ANY_SOURCE)
        src_traced = (status != MPI_STATUSES_IGNORE)
                         ? savedvals[*index].comm->group()->rank(status->MPI_SOURCE)
                         : savedvals[*index].src;
      TRACE_smpi_recv(rank_traced, src_traced, dst_traced, savedvals[*index].tag);
    }
    TRACE_smpi_ptp_out(rank_traced, src_traced, dst_traced, __FUNCTION__);
  }
  xbt_free(savedvals);

  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Waitall(int count, MPI_Request requests[], MPI_Status status[])
{
  smpi_bench_end();
  //save information from requests
  typedef struct {
    int src;
    int dst;
    int recv;
    int tag;
    int valid;
    MPI_Comm comm;
  } savedvalstype;
  savedvalstype* savedvals=xbt_new0(savedvalstype, count);

  for (int i = 0; i < count; i++) {
    MPI_Request req = requests[i];
    if(req!=MPI_REQUEST_NULL){
      savedvals[i]=(savedvalstype){req->src, req->dst, req->recv, req->tag, 1, req->comm};
    }else{
      savedvals[i].valid=0;
    }
  }
  int rank_traced = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_WAITALL;
  extra->send_size=count;
  TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__,extra);

  int retval = smpi_mpi_waitall(count, requests, status);

  for (int i = 0; i < count; i++) {
    if(savedvals[i].valid){
    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
      int src_traced = savedvals[i].src;
      int dst_traced = savedvals[i].dst;
      int is_wait_for_receive = savedvals[i].recv;
      if (is_wait_for_receive) {
        if(src_traced==MPI_ANY_SOURCE)
        src_traced = (status!=MPI_STATUSES_IGNORE) ?
                          savedvals[i].comm->group()->rank(status[i].MPI_SOURCE) : savedvals[i].src;
        TRACE_smpi_recv(rank_traced, src_traced, dst_traced,savedvals[i].tag);
      }
    }
  }
  TRACE_smpi_ptp_out(rank_traced, -1, -1, __FUNCTION__);
  xbt_free(savedvals);

  smpi_bench_begin();
  return retval;
}

int PMPI_Waitsome(int incount, MPI_Request requests[], int *outcount, int *indices, MPI_Status status[])
{
  int retval = 0;

  smpi_bench_end();
  if (outcount == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *outcount = smpi_mpi_waitsome(incount, requests, indices, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testsome(int incount, MPI_Request requests[], int* outcount, int* indices, MPI_Status status[])
{
  int retval = 0;

  smpi_bench_end();
  if (outcount == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *outcount = smpi_mpi_testsome(incount, requests, indices, status);
    retval    = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}


int PMPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_ARG;
  } else {
    int rank        = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int root_traced = comm->group()->index(root);

    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_BCAST;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = smpi_datatype_size(datatype);
    extra->send_size = count * dt_size_send;
    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);
    if (comm->size() > 1)
      mpi_coll_bcast_fun(buf, count, datatype, root, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Barrier(MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_BARRIER;
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    mpi_coll_barrier_fun(comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            ((comm->rank() == root) && (recvtype == MPI_DATATYPE_NULL))){
    retval = MPI_ERR_TYPE;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendcount <0)) || ((comm->rank() == root) && (recvcount <0))){
    retval = MPI_ERR_COUNT;
  } else {

    char* sendtmpbuf = static_cast<char*>(sendbuf);
    int sendtmpcount = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if( (comm->rank() == root) && (sendbuf == MPI_IN_PLACE )) {
      sendtmpcount=0;
      sendtmptype=recvtype;
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int root_traced        = comm->group()->index(root);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_GATHER;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtmptype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = smpi_datatype_size(sendtmptype);
    extra->send_size = sendtmpcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if ((comm->rank() == root) && known == 0)
      dt_size_recv   = smpi_datatype_size(recvtype);
    extra->recv_size = recvcount * dt_size_recv;

    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    mpi_coll_gather_fun(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, root, comm);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            ((comm->rank() == root) && (recvtype == MPI_DATATYPE_NULL))){
    retval = MPI_ERR_TYPE;
  } else if (( sendbuf != MPI_IN_PLACE) && (sendcount <0)){
    retval = MPI_ERR_COUNT;
  } else if (recvcounts == nullptr || displs == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    char* sendtmpbuf = static_cast<char*>(sendbuf);
    int sendtmpcount = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if( (comm->rank() == root) && (sendbuf == MPI_IN_PLACE )) {
      sendtmpcount=0;
      sendtmptype=recvtype;
    }

    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int root_traced        = comm->group()->index(root);
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_GATHERV;
    extra->num_processes   = size;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtmptype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = smpi_datatype_size(sendtype);
    extra->send_size = sendtmpcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv = smpi_datatype_size(recvtype);
    if ((comm->rank() == root)) {
      extra->recvcounts = xbt_new(int, size);
      for (i                 = 0; i < size; i++) // copy data to avoid bad free
        extra->recvcounts[i] = recvcounts[i] * dt_size_recv;
    }
    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    smpi_mpi_gatherv(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcounts, displs, recvtype, root, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            (recvtype == MPI_DATATYPE_NULL)){
    retval = MPI_ERR_TYPE;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendcount <0)) ||
            (recvcount <0)){
    retval = MPI_ERR_COUNT;
  } else {
    if(sendbuf == MPI_IN_PLACE) {
      sendbuf=static_cast<char*>(recvbuf)+smpi_datatype_get_extent(recvtype)*recvcount*comm->rank();
      sendcount=recvcount;
      sendtype=recvtype;
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLGATHER;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = smpi_datatype_size(sendtype);
    extra->send_size = sendcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv   = smpi_datatype_size(recvtype);
    extra->recv_size = recvcount * dt_size_recv;

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    mpi_coll_allgather_fun(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (((sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) || (recvtype == MPI_DATATYPE_NULL)) {
    retval = MPI_ERR_TYPE;
  } else if (( sendbuf != MPI_IN_PLACE) && (sendcount <0)){
    retval = MPI_ERR_COUNT;
  } else if (recvcounts == nullptr || displs == nullptr) {
    retval = MPI_ERR_ARG;
  } else {

    if(sendbuf == MPI_IN_PLACE) {
      sendbuf=static_cast<char*>(recvbuf)+smpi_datatype_get_extent(recvtype)*displs[comm->rank()];
      sendcount=recvcounts[comm->rank()];
      sendtype=recvtype;
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLGATHERV;
    extra->num_processes   = size;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = smpi_datatype_size(sendtype);
    extra->send_size = sendcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv    = smpi_datatype_size(recvtype);
    extra->recvcounts = xbt_new(int, size);
    for (i                 = 0; i < size; i++) // copy data to avoid bad free
      extra->recvcounts[i] = recvcounts[i] * dt_size_recv;

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    mpi_coll_allgatherv_fun(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (((comm->rank() == root) && (!is_datatype_valid(sendtype))) ||
             ((recvbuf != MPI_IN_PLACE) && (!is_datatype_valid(recvtype)))) {
    retval = MPI_ERR_TYPE;
  } else if ((sendbuf == recvbuf) ||
      ((comm->rank()==root) && sendcount>0 && (sendbuf == nullptr))){
    retval = MPI_ERR_BUFFER;
  }else {

    if (recvbuf == MPI_IN_PLACE) {
      recvtype  = sendtype;
      recvcount = sendcount;
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int root_traced        = comm->group()->index(root);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_SCATTER;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if ((comm->rank() == root) && known == 0)
      dt_size_send   = smpi_datatype_size(sendtype);
    extra->send_size = sendcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv   = smpi_datatype_size(recvtype);
    extra->recv_size = recvcount * dt_size_recv;
    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    mpi_coll_scatter_fun(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scatterv(void *sendbuf, int *sendcounts, int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendcounts == nullptr || displs == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (((comm->rank() == root) && (sendtype == MPI_DATATYPE_NULL)) ||
             ((recvbuf != MPI_IN_PLACE) && (recvtype == MPI_DATATYPE_NULL))) {
    retval = MPI_ERR_TYPE;
  } else {
    if (recvbuf == MPI_IN_PLACE) {
      recvtype  = sendtype;
      recvcount = sendcounts[comm->rank()];
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int root_traced        = comm->group()->index(root);
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_SCATTERV;
    extra->num_processes   = size;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send = smpi_datatype_size(sendtype);
    if ((comm->rank() == root)) {
      extra->sendcounts = xbt_new(int, size);
      for (i                 = 0; i < size; i++) // copy data to avoid bad free
        extra->sendcounts[i] = sendcounts[i] * dt_size_send;
    }
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv   = smpi_datatype_size(recvtype);
    extra->recv_size = recvcount * dt_size_recv;
    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    smpi_mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype) || op == MPI_OP_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int root_traced        = comm->group()->index(root);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_REDUCE;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = smpi_datatype_size(datatype);
    extra->send_size = count * dt_size_send;
    extra->root      = root_traced;

    TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    mpi_coll_reduce_fun(sendbuf, recvbuf, count, datatype, op, root, comm);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_local(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op){
  int retval = 0;

  smpi_bench_end();
  if (!is_datatype_valid(datatype) || op == MPI_OP_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_op_apply(op, inbuf, inoutbuf, &count, &datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {

    char* sendtmpbuf = static_cast<char*>(sendbuf);
    if( sendbuf == MPI_IN_PLACE ) {
      sendtmpbuf = static_cast<char*>(xbt_malloc(count*smpi_datatype_get_extent(datatype)));
      smpi_datatype_copy(recvbuf, count, datatype,sendtmpbuf, count, datatype);
    }
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLREDUCE;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = smpi_datatype_size(datatype);
    extra->send_size = count * dt_size_send;

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    mpi_coll_allreduce_fun(sendtmpbuf, recvbuf, count, datatype, op, comm);

    if( sendbuf == MPI_IN_PLACE )
      xbt_free(sendtmpbuf);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_SCAN;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = smpi_datatype_size(datatype);
    extra->send_size = count * dt_size_send;

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    smpi_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm){
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_EXSCAN;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = smpi_datatype_size(datatype);
    extra->send_size = count * dt_size_send;
    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(count * smpi_datatype_size(datatype)));
      memcpy(sendtmpbuf, recvbuf, count * smpi_datatype_size(datatype));
    }
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    smpi_mpi_exscan(sendtmpbuf, recvbuf, count, datatype, op, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (recvcounts == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_REDUCE_SCATTER;
    extra->num_processes   = size;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send    = smpi_datatype_size(datatype);
    extra->send_size  = 0;
    extra->recvcounts = xbt_new(int, size);
    int totalcount    = 0;
    for (i = 0; i < size; i++) { // copy data to avoid bad free
      extra->recvcounts[i] = recvcounts[i] * dt_size_send;
      totalcount += recvcounts[i];
    }
    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(totalcount * smpi_datatype_size(datatype)));
      memcpy(sendtmpbuf, recvbuf, totalcount * smpi_datatype_size(datatype));
    }

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    mpi_coll_reduce_scatter_fun(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);

    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter_block(void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (recvcount < 0) {
    retval = MPI_ERR_ARG;
  } else {
    int count = comm->size();

    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_REDUCE_SCATTER;
    extra->num_processes   = count;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send    = smpi_datatype_size(datatype);
    extra->send_size  = 0;
    extra->recvcounts = xbt_new(int, count);
    for (int i             = 0; i < count; i++) // copy data to avoid bad free
      extra->recvcounts[i] = recvcount * dt_size_send;
    void* sendtmpbuf       = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(recvcount * count * smpi_datatype_size(datatype)));
      memcpy(sendtmpbuf, recvbuf, recvcount * count * smpi_datatype_size(datatype));
    }

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    int* recvcounts = static_cast<int*>(xbt_malloc(count * sizeof(int)));
    for (int i      = 0; i < count; i++)
      recvcounts[i] = recvcount;
    mpi_coll_reduce_scatter_fun(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm);
    xbt_free(recvcounts);
    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);

    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                  MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((sendbuf != MPI_IN_PLACE && sendtype == MPI_DATATYPE_NULL) || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLTOALL;

    void* sendtmpbuf         = static_cast<char*>(sendbuf);
    int sendtmpcount         = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(recvcount * comm->size() * smpi_datatype_size(recvtype)));
      memcpy(sendtmpbuf, recvbuf, recvcount * comm->size() * smpi_datatype_size(recvtype));
      sendtmpcount = recvcount;
      sendtmptype  = recvtype;
    }

    int known        = 0;
    extra->datatype1 = encode_datatype(sendtmptype, &known);
    if (known == 0)
      extra->send_size = sendtmpcount * smpi_datatype_size(sendtmptype);
    else
      extra->send_size = sendtmpcount;
    extra->datatype2   = encode_datatype(recvtype, &known);
    if (known == 0)
      extra->recv_size = recvcount * smpi_datatype_size(recvtype);
    else
      extra->recv_size = recvcount;

    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

    retval = mpi_coll_alltoall_fun(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, comm);

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);

    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoallv(void* sendbuf, int* sendcounts, int* senddisps, MPI_Datatype sendtype, void* recvbuf,
                   int* recvcounts, int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if ((sendbuf != MPI_IN_PLACE && (sendcounts == nullptr || senddisps == nullptr)) || recvcounts == nullptr ||
             recvdisps == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    int rank               = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLTOALLV;
    extra->send_size       = 0;
    extra->recv_size       = 0;
    extra->recvcounts      = xbt_new(int, size);
    extra->sendcounts      = xbt_new(int, size);
    int known              = 0;
    int dt_size_recv       = 1;
    extra->datatype2       = encode_datatype(recvtype, &known);
    dt_size_recv           = smpi_datatype_size(recvtype);

    void* sendtmpbuf         = static_cast<char*>(sendbuf);
    int* sendtmpcounts       = sendcounts;
    int* sendtmpdisps        = senddisps;
    MPI_Datatype sendtmptype = sendtype;
    int maxsize              = 0;
    for (i = 0; i < size; i++) { // copy data to avoid bad free
      extra->recv_size += recvcounts[i] * dt_size_recv;
      extra->recvcounts[i] = recvcounts[i] * dt_size_recv;
      if (((recvdisps[i] + recvcounts[i]) * dt_size_recv) > maxsize)
        maxsize = (recvdisps[i] + recvcounts[i]) * dt_size_recv;
    }

    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(maxsize));
      memcpy(sendtmpbuf, recvbuf, maxsize);
      sendtmpcounts = static_cast<int*>(xbt_malloc(size * sizeof(int)));
      memcpy(sendtmpcounts, recvcounts, size * sizeof(int));
      sendtmpdisps = static_cast<int*>(xbt_malloc(size * sizeof(int)));
      memcpy(sendtmpdisps, recvdisps, size * sizeof(int));
      sendtmptype = recvtype;
    }

    extra->datatype1 = encode_datatype(sendtmptype, &known);
    int dt_size_send = 1;
    dt_size_send     = smpi_datatype_size(sendtmptype);

    for (i = 0; i < size; i++) { // copy data to avoid bad free
      extra->send_size += sendtmpcounts[i] * dt_size_send;
      extra->sendcounts[i] = sendtmpcounts[i] * dt_size_send;
    }
    extra->num_processes = size;
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);
    retval = mpi_coll_alltoallv_fun(sendtmpbuf, sendtmpcounts, sendtmpdisps, sendtmptype, recvbuf, recvcounts,
                                    recvdisps, recvtype, comm);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);

    if (sendbuf == MPI_IN_PLACE) {
      xbt_free(sendtmpbuf);
      xbt_free(sendtmpcounts);
      xbt_free(sendtmpdisps);
    }
  }

  smpi_bench_begin();
  return retval;
}


int PMPI_Get_processor_name(char *name, int *resultlen)
{
  strncpy(name, SIMIX_host_self()->cname(), strlen(SIMIX_host_self()->cname()) < MPI_MAX_PROCESSOR_NAME - 1
                                                ? strlen(SIMIX_host_self()->cname()) + 1
                                                : MPI_MAX_PROCESSOR_NAME - 1);
  *resultlen = strlen(name) > MPI_MAX_PROCESSOR_NAME ? MPI_MAX_PROCESSOR_NAME : strlen(name);

  return MPI_SUCCESS;
}

int PMPI_Get_count(MPI_Status * status, MPI_Datatype datatype, int *count)
{
  if (status == nullptr || count == nullptr) {
    return MPI_ERR_ARG;
  } else if (!is_datatype_valid(datatype)) {
    return MPI_ERR_TYPE;
  } else {
    size_t size = smpi_datatype_size(datatype);
    if (size == 0) {
      *count = 0;
      return MPI_SUCCESS;
    } else if (status->count % size != 0) {
      return MPI_UNDEFINED;
    } else {
      *count = smpi_mpi_get_count(status, datatype);
      return MPI_SUCCESS;
    }
  }
}

int PMPI_Type_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return smpi_datatype_contiguous(count, old_type, new_type, 0);
  }
}

int PMPI_Type_commit(MPI_Datatype* datatype) {
  if (datatype == nullptr || *datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else {
    smpi_datatype_commit(datatype);
    return MPI_SUCCESS;
  }
}

int PMPI_Type_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0 || blocklen<0){
    return MPI_ERR_COUNT;
  } else {
    return smpi_datatype_vector(count, blocklen, stride, old_type, new_type);
  }
}

int PMPI_Type_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0 || blocklen<0){
    return MPI_ERR_COUNT;
  } else {
    return smpi_datatype_hvector(count, blocklen, stride, old_type, new_type);
  }
}

int PMPI_Type_create_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  return MPI_Type_hvector(count, blocklen, stride, old_type, new_type);
}

int PMPI_Type_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return smpi_datatype_indexed(count, blocklens, indices, old_type, new_type);
  }
}

int PMPI_Type_create_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return smpi_datatype_indexed(count, blocklens, indices, old_type, new_type);
  }
}

int PMPI_Type_create_indexed_block(int count, int blocklength, int* indices, MPI_Datatype old_type,
                                   MPI_Datatype* new_type)
{
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    int* blocklens=static_cast<int*>(xbt_malloc(blocklength*count*sizeof(int)));
    for (int i    = 0; i < count; i++)
      blocklens[i]=blocklength;
    int retval    = smpi_datatype_indexed(count, blocklens, indices, old_type, new_type);
    xbt_free(blocklens);
    return retval;
  }
}

int PMPI_Type_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return smpi_datatype_hindexed(count, blocklens, indices, old_type, new_type);
  }
}

int PMPI_Type_create_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type,
                              MPI_Datatype* new_type) {
  return PMPI_Type_hindexed(count, blocklens,indices,old_type,new_type);
}

int PMPI_Type_create_hindexed_block(int count, int blocklength, MPI_Aint* indices, MPI_Datatype old_type,
                                    MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    int* blocklens=(int*)xbt_malloc(blocklength*count*sizeof(int));
    for (int i     = 0; i < count; i++)
      blocklens[i] = blocklength;
    int retval     = smpi_datatype_hindexed(count, blocklens, indices, old_type, new_type);
    xbt_free(blocklens);
    return retval;
  }
}

int PMPI_Type_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* new_type) {
  if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return smpi_datatype_struct(count, blocklens, indices, old_types, new_type);
  }
}

int PMPI_Type_create_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types,
                            MPI_Datatype* new_type) {
  return PMPI_Type_struct(count, blocklens, indices, old_types, new_type);
}

int PMPI_Error_class(int errorcode, int* errorclass) {
  // assume smpi uses only standard mpi error codes
  *errorclass=errorcode;
  return MPI_SUCCESS;
}

int PMPI_Initialized(int* flag) {
   *flag=smpi_process_initialized();
   return MPI_SUCCESS;
}

/* The topo part of MPI_COMM_WORLD should always be nullptr. When other topologies will be implemented, not only should we
 * check if the topology is nullptr, but we should check if it is the good topology type (so we have to add a
 *  MPIR_Topo_Type field, and replace the MPI_Topology field by an union)*/

int PMPI_Cart_create(MPI_Comm comm_old, int ndims, int* dims, int* periodic, int reorder, MPI_Comm* comm_cart) {
  if (comm_old == MPI_COMM_NULL){
    return MPI_ERR_COMM;
  } else if (ndims < 0 || (ndims > 0 && (dims == nullptr || periodic == nullptr)) || comm_cart == nullptr) {
    return MPI_ERR_ARG;
  } else{
    new simgrid::smpi::Cart(comm_old, ndims, dims, periodic, reorder, comm_cart);
    return MPI_SUCCESS;
  }
}

int PMPI_Cart_rank(MPI_Comm comm, int* coords, int* rank) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (coords == nullptr) {
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Cart* topo = static_cast<simgrid::smpi::Cart*>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->rank(coords, rank);
}

int PMPI_Cart_shift(MPI_Comm comm, int direction, int displ, int* source, int* dest) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (source == nullptr || dest == nullptr || direction < 0 ) {
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Cart* topo = static_cast<simgrid::smpi::Cart*>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->shift(direction, displ, source, dest);
}

int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int* coords) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (rank < 0 || rank >= comm->size()) {
    return MPI_ERR_RANK;
  }
  if (maxdims <= 0) {
    return MPI_ERR_ARG;
  }
  if(coords == nullptr) {
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Cart* topo = static_cast<simgrid::smpi::Cart*>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->coords(rank, maxdims, coords);
}

int PMPI_Cart_get(MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords) {
  if(comm == nullptr || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if(maxdims <= 0 || dims == nullptr || periods == nullptr || coords == nullptr) {
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Cart* topo = static_cast<simgrid::smpi::Cart*>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->get(maxdims, dims, periods, coords);
}

int PMPI_Cartdim_get(MPI_Comm comm, int* ndims) {
  if (comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (ndims == nullptr) {
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Cart* topo = static_cast<simgrid::smpi::Cart*>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->dim_get(ndims);
}

int PMPI_Dims_create(int nnodes, int ndims, int* dims) {
  if(dims == nullptr) {
    return MPI_ERR_ARG;
  }
  if (ndims < 1 || nnodes < 1) {
    return MPI_ERR_DIMS;
  }
  return simgrid::smpi::Dims_create(nnodes, ndims, dims);
}

int PMPI_Cart_sub(MPI_Comm comm, int* remain_dims, MPI_Comm* comm_new) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (comm_new == nullptr) {
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Cart* topo = static_cast<simgrid::smpi::Cart*>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Cart* cart = topo->sub(remain_dims, comm_new);
  if(cart==nullptr)
    return  MPI_ERR_ARG;
  return MPI_SUCCESS;
}

int PMPI_Type_create_resized(MPI_Datatype oldtype,MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype){
  if (oldtype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  }
  int blocks[3]         = {1, 1, 1};
  MPI_Aint disps[3]     = {lb, 0, lb + extent};
  MPI_Datatype types[3] = {MPI_LB, oldtype, MPI_UB};

  s_smpi_mpi_struct_t* subtype = smpi_datatype_struct_create(blocks, disps, 3, types);
  smpi_datatype_create(newtype, oldtype->size, lb, lb + extent, sizeof(s_smpi_mpi_struct_t), subtype, DT_FLAG_VECTOR);

  (*newtype)->flags &= ~DT_FLAG_COMMITED;
  return MPI_SUCCESS;
}

int PMPI_Win_create( void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win){
  int retval = 0;
  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval= MPI_ERR_COMM;
  }else if ((base == nullptr && size != 0) || disp_unit <= 0 || size < 0 ){
    retval= MPI_ERR_OTHER;
  }else{
    *win = new simgrid::smpi::Win( base, size, disp_unit, info, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_free( MPI_Win* win){
  int retval = 0;
  smpi_bench_end();
  if (win == nullptr || *win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  }else{
    delete(*win);
    retval=MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_set_name(MPI_Win  win, char * name)
{
  if (win == MPI_WIN_NULL)  {
    return MPI_ERR_TYPE;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    win->set_name(name);
    return MPI_SUCCESS;
  }
}

int PMPI_Win_get_name(MPI_Win  win, char * name, int* len)
{
  if (win == MPI_WIN_NULL)  {
    return MPI_ERR_WIN;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    win->get_name(name, len);
    return MPI_SUCCESS;
  }
}

int PMPI_Win_get_group(MPI_Win  win, MPI_Group * group){
  if (win == MPI_WIN_NULL)  {
    return MPI_ERR_WIN;
  }else {
    win->get_group(group);
    (*group)->use();
    return MPI_SUCCESS;
  }
}

int PMPI_Win_fence( int assert,  MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
  int rank = smpi_process_index();
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__, nullptr);
  retval = win->fence(assert);
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (target_disp <0){
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0) ||
             (origin_addr==nullptr && origin_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((!is_datatype_valid(origin_datatype)) || (!is_datatype_valid(target_datatype))) {
    retval = MPI_ERR_TYPE;
  } else {
    int rank = smpi_process_index();
    MPI_Group group;
    win->get_group(&group);
    int src_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, nullptr);

    retval = win->get( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Put( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (target_disp <0){
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0) ||
            (origin_addr==nullptr && origin_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((!is_datatype_valid(origin_datatype)) || (!is_datatype_valid(target_datatype))) {
    retval = MPI_ERR_TYPE;
  } else {
    int rank = smpi_process_index();
    MPI_Group group;
    win->get_group(&group);
    int dst_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, nullptr);
    TRACE_smpi_send(rank, rank, dst_traced, SMPI_RMA_TAG, origin_count*smpi_datatype_size(origin_datatype));

    retval = win->put( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype);

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Accumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (target_disp <0){
    retval = MPI_ERR_ARG;
  } else if ((origin_count < 0 || target_count < 0) ||
             (origin_addr==nullptr && origin_count > 0)){
    retval = MPI_ERR_COUNT;
  } else if ((!is_datatype_valid(origin_datatype)) ||
            (!is_datatype_valid(target_datatype))) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int rank = smpi_process_index();
    MPI_Group group;
    win->get_group(&group);
    int src_traced = group->index(target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, nullptr);

    retval = win->accumulate( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                                  target_datatype, op);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (group==MPI_GROUP_NULL){
    retval = MPI_ERR_GROUP;
  } else {
    int rank = smpi_process_index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, nullptr);
    retval = win->post(group,assert);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (group==MPI_GROUP_NULL){
    retval = MPI_ERR_GROUP;
  } else {
    int rank = smpi_process_index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, nullptr);
    retval = win->start(group,assert);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_complete(MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
    int rank = smpi_process_index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, nullptr);

    retval = win->complete();

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_wait(MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
    int rank = smpi_process_index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, nullptr);

    retval = win->wait();

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr){
  void *ptr = xbt_malloc(size);
  if(ptr==nullptr)
    return MPI_ERR_NO_MEM;
  else {
    *static_cast<void**>(baseptr) = ptr;
    return MPI_SUCCESS;
  }
}

int PMPI_Free_mem(void *baseptr){
  xbt_free(baseptr);
  return MPI_SUCCESS;
}

int PMPI_Type_set_name(MPI_Datatype  datatype, char * name)
{
  if (datatype == MPI_DATATYPE_NULL)  {
    return MPI_ERR_TYPE;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    smpi_datatype_set_name(datatype, name);
    return MPI_SUCCESS;
  }
}

int PMPI_Type_get_name(MPI_Datatype  datatype, char * name, int* len)
{
  if (datatype == MPI_DATATYPE_NULL)  {
    return MPI_ERR_TYPE;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    smpi_datatype_get_name(datatype, name, len);
    return MPI_SUCCESS;
  }
}

MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype){
  return smpi_type_f2c(datatype);
}

MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype){
  return smpi_type_c2f( datatype);
}

MPI_Group PMPI_Group_f2c(MPI_Fint group){
  return smpi_group_f2c( group);
}

MPI_Fint PMPI_Group_c2f(MPI_Group group){
  return smpi_group_c2f(group);
}

MPI_Request PMPI_Request_f2c(MPI_Fint request){
  return smpi_request_f2c(request);
}

MPI_Fint PMPI_Request_c2f(MPI_Request request) {
  return smpi_request_c2f(request);
}

MPI_Win PMPI_Win_f2c(MPI_Fint win){
  return smpi_win_f2c(win);
}

MPI_Fint PMPI_Win_c2f(MPI_Win win){
  return smpi_win_c2f(win);
}

MPI_Op PMPI_Op_f2c(MPI_Fint op){
  return smpi_op_f2c(op);
}

MPI_Fint PMPI_Op_c2f(MPI_Op op){
  return smpi_op_c2f(op);
}

MPI_Comm PMPI_Comm_f2c(MPI_Fint comm){
  return smpi_comm_f2c(comm);
}

MPI_Fint PMPI_Comm_c2f(MPI_Comm comm){
  return smpi_comm_c2f(comm);
}

MPI_Info PMPI_Info_f2c(MPI_Fint info){
  return smpi_info_f2c(info);
}

MPI_Fint PMPI_Info_c2f(MPI_Info info){
  return smpi_info_c2f(info);
}

int PMPI_Keyval_create(MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state) {
  return smpi_comm_keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

int PMPI_Keyval_free(int* keyval) {
  return smpi_comm_keyval_free(keyval);
}

int PMPI_Attr_delete(MPI_Comm comm, int keyval) {
  if(keyval == MPI_TAG_UB||keyval == MPI_HOST||keyval == MPI_IO ||keyval == MPI_WTIME_IS_GLOBAL||keyval == MPI_APPNUM
       ||keyval == MPI_UNIVERSE_SIZE||keyval == MPI_LASTUSEDCODE)
    return MPI_ERR_ARG;
  else if (comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  else
    return comm->attr_delete(keyval);
}

int PMPI_Attr_get(MPI_Comm comm, int keyval, void* attr_value, int* flag) {
  static int one = 1;
  static int zero = 0;
  static int tag_ub = 1000000;
  static int last_used_code = MPI_ERR_LASTCODE;

  if (comm==MPI_COMM_NULL){
    *flag = 0;
    return MPI_ERR_COMM;
  }

  switch (keyval) {
  case MPI_HOST:
  case MPI_IO:
  case MPI_APPNUM:
    *flag = 1;
    *static_cast<int**>(attr_value) = &zero;
    return MPI_SUCCESS;
  case MPI_UNIVERSE_SIZE:
    *flag = 1;
    *static_cast<int**>(attr_value) = &smpi_universe_size;
    return MPI_SUCCESS;
  case MPI_LASTUSEDCODE:
    *flag = 1;
    *static_cast<int**>(attr_value) = &last_used_code;
    return MPI_SUCCESS;
  case MPI_TAG_UB:
    *flag=1;
    *static_cast<int**>(attr_value) = &tag_ub;
    return MPI_SUCCESS;
  case MPI_WTIME_IS_GLOBAL:
    *flag = 1;
    *static_cast<int**>(attr_value) = &one;
    return MPI_SUCCESS;
  default:
    return comm->attr_get(keyval, attr_value, flag);
  }
}

int PMPI_Attr_put(MPI_Comm comm, int keyval, void* attr_value) {
  if(keyval == MPI_TAG_UB||keyval == MPI_HOST||keyval == MPI_IO ||keyval == MPI_WTIME_IS_GLOBAL||keyval == MPI_APPNUM
       ||keyval == MPI_UNIVERSE_SIZE||keyval == MPI_LASTUSEDCODE)
    return MPI_ERR_ARG;
  else if (comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  else
  return comm->attr_put(keyval, attr_value);
}

int PMPI_Comm_get_attr (MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
  return PMPI_Attr_get(comm, comm_keyval, attribute_val,flag);
}

int PMPI_Comm_set_attr (MPI_Comm comm, int comm_keyval, void *attribute_val)
{
  return PMPI_Attr_put(comm, comm_keyval, attribute_val);
}

int PMPI_Comm_delete_attr (MPI_Comm comm, int comm_keyval)
{
  return PMPI_Attr_delete(comm, comm_keyval);
}

int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function* copy_fn, MPI_Comm_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state)
{
  return PMPI_Keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

int PMPI_Comm_free_keyval(int* keyval) {
  return PMPI_Keyval_free(keyval);
}

int PMPI_Type_get_attr (MPI_Datatype type, int type_keyval, void *attribute_val, int* flag)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return smpi_type_attr_get(type, type_keyval, attribute_val, flag);
}

int PMPI_Type_set_attr (MPI_Datatype type, int type_keyval, void *attribute_val)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return smpi_type_attr_put(type, type_keyval, attribute_val);
}

int PMPI_Type_delete_attr (MPI_Datatype type, int type_keyval)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return smpi_type_attr_delete(type, type_keyval);
}

int PMPI_Type_create_keyval(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state)
{
  return smpi_type_keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

int PMPI_Type_free_keyval(int* keyval) {
  return smpi_type_keyval_free(keyval);
}

int PMPI_Info_create( MPI_Info *info){
  if (info == nullptr)
    return MPI_ERR_ARG;
  *info = xbt_new(s_smpi_mpi_info_t, 1);
  (*info)->info_dict= xbt_dict_new_homogeneous(xbt_free_f);
  (*info)->refcount=1;
  return MPI_SUCCESS;
}

int PMPI_Info_set( MPI_Info info, char *key, char *value){
  if (info == nullptr || key == nullptr || value == nullptr)
    return MPI_ERR_ARG;

  xbt_dict_set(info->info_dict, key, xbt_strdup(value), nullptr);
  return MPI_SUCCESS;
}

int PMPI_Info_free( MPI_Info *info){
  if (info == nullptr || *info==nullptr)
    return MPI_ERR_ARG;
  (*info)->refcount--;
  if((*info)->refcount==0){
    xbt_dict_free(&((*info)->info_dict));
    xbt_free(*info);
  }
  *info=MPI_INFO_NULL;
  return MPI_SUCCESS;
}

int PMPI_Info_get(MPI_Info info,char *key,int valuelen, char *value, int *flag){
  *flag=false;
  if (info == nullptr || key == nullptr || valuelen <0)
    return MPI_ERR_ARG;
  if (value == nullptr)
    return MPI_ERR_INFO_VALUE;
  char* tmpvalue=static_cast<char*>(xbt_dict_get_or_null(info->info_dict, key));
  if(tmpvalue){
    memset(value, 0, valuelen);
    memcpy(value,tmpvalue, (strlen(tmpvalue) + 1 < static_cast<size_t>(valuelen)) ? strlen(tmpvalue) + 1 : valuelen);
    *flag=true;
  }
  return MPI_SUCCESS;
}

int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo){
  if (info == nullptr || newinfo==nullptr)
    return MPI_ERR_ARG;
  *newinfo = xbt_new(s_smpi_mpi_info_t, 1);
  (*newinfo)->info_dict= xbt_dict_new_homogeneous(xbt_free_f);
  (*newinfo)->refcount=1;
  xbt_dict_cursor_t cursor = nullptr;
  char* key;
  void* data;
  xbt_dict_foreach(info->info_dict,cursor,key,data){
    xbt_dict_set((*newinfo)->info_dict, key, xbt_strdup(static_cast<char*>(data)), nullptr);
  }
  return MPI_SUCCESS;
}

int PMPI_Info_delete(MPI_Info info, char *key){
  if (info == nullptr || key==nullptr)
    return MPI_ERR_ARG;
  try {
    xbt_dict_remove(info->info_dict, key);
  }
  catch(xbt_ex& e){
    return MPI_ERR_INFO_NOKEY;
  }
  return MPI_SUCCESS;
}

int PMPI_Info_get_nkeys( MPI_Info info, int *nkeys){
  if (info == nullptr || nkeys==nullptr)
    return MPI_ERR_ARG;
  *nkeys=xbt_dict_size(info->info_dict);
  return MPI_SUCCESS;
}

int PMPI_Info_get_nthkey( MPI_Info info, int n, char *key){
  if (info == nullptr || key==nullptr || n<0 || n> MPI_MAX_INFO_KEY)
    return MPI_ERR_ARG;

  xbt_dict_cursor_t cursor = nullptr;
  char *keyn;
  void* data;
  int num=0;
  xbt_dict_foreach(info->info_dict,cursor,keyn,data){
    if(num==n){
      strncpy(key,keyn,strlen(keyn)+1);
      xbt_dict_cursor_free(&cursor);
      return MPI_SUCCESS;
    }
    num++;
  }
  return MPI_ERR_ARG;
}

int PMPI_Info_get_valuelen( MPI_Info info, char *key, int *valuelen, int *flag){
  *flag=false;
  if (info == nullptr || key == nullptr || valuelen==nullptr)
    return MPI_ERR_ARG;
  char* tmpvalue=(char*)xbt_dict_get_or_null(info->info_dict, key);
  if(tmpvalue){
    *valuelen=strlen(tmpvalue);
    *flag=true;
  }
  return MPI_SUCCESS;
}

int PMPI_Unpack(void* inbuf, int incount, int* position, void* outbuf, int outcount, MPI_Datatype type, MPI_Comm comm) {
  if(incount<0 || outcount < 0 || inbuf==nullptr || outbuf==nullptr)
    return MPI_ERR_ARG;
  if(!is_datatype_valid(type))
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  return smpi_mpi_unpack(inbuf, incount, position, outbuf,outcount,type, comm);
}

int PMPI_Pack(void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position, MPI_Comm comm) {
  if(incount<0 || outcount < 0|| inbuf==nullptr || outbuf==nullptr)
    return MPI_ERR_ARG;
  if(!is_datatype_valid(type))
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  return smpi_mpi_pack(inbuf, incount, type, outbuf,outcount,position, comm);
}

int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int* size) {
  if(incount<0)
    return MPI_ERR_ARG;
  if(!is_datatype_valid(datatype))
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;

  *size=incount*smpi_datatype_size(datatype);

  return MPI_SUCCESS;
}

} // extern "C"
