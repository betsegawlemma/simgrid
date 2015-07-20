/* Copyright (c) 2007-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "host_ptask_L07.hpp"

#include "cpu_interface.hpp"
#include "surf_routing.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_host);

static int ptask_host_count = 0;
static xbt_dict_t ptask_parallel_task_link_set = NULL;
lmm_system_t ptask_maxmin_system = NULL;


/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/

static void ptask_netlink_parse_init(sg_platf_link_cbarg_t link)
{
  netlink_parse_init(link);
  current_property_set = NULL;
}

static void ptask_define_callbacks()
{
  sg_platf_host_add_cb(cpu_parse_init);
  sg_platf_host_add_cb(host_parse_init);
  sg_platf_link_add_cb(ptask_netlink_parse_init);
  sg_platf_postparse_add_cb(host_add_traces);
}

void surf_host_model_init_ptask_L07(void)
{
  XBT_INFO("surf_host_model_init_ptask_L07");
  xbt_assert(!surf_cpu_model_pm, "CPU model type already defined");
  xbt_assert(!surf_network_model, "network model type already defined");
  ptask_define_callbacks();
  surf_host_model = new HostL07Model();
  Model *model = surf_host_model;
  xbt_dynar_push(model_list, &model);
  xbt_dynar_push(model_list_invoke, &model);
}


HostL07Model::HostL07Model() : HostModel("Host ptask_L07") {
  if (!ptask_maxmin_system)
	ptask_maxmin_system = lmm_system_new(1);
  surf_host_model = NULL;
  surf_network_model = new NetworkL07Model();
  surf_cpu_model_pm = new CpuL07Model();

  routing_model_create(surf_network_model->createLink("__loopback__",
	                                                  498000000, NULL,
	                                                  0.000015, NULL,
	                                                  SURF_RESOURCE_ON, NULL,
	                                                  SURF_LINK_FATPIPE, NULL));
  p_cpuModel = surf_cpu_model_pm;
}

HostL07Model::~HostL07Model() {
  xbt_dict_free(&ptask_parallel_task_link_set);

  delete surf_cpu_model_pm;
  delete surf_network_model;
  ptask_host_count = 0;

  if (ptask_maxmin_system) {
    lmm_system_free(ptask_maxmin_system);
    ptask_maxmin_system = NULL;
  }
}

double HostL07Model::shareResources(double /*now*/)
{
  HostL07Action *action;

  ActionList *running_actions = getRunningActionSet();
  double min = this->shareResourcesMaxMin(running_actions,
                                              ptask_maxmin_system,
                                              bottleneck_solve);

  for(ActionList::iterator it(running_actions->begin()), itend(running_actions->end())
	 ; it != itend ; ++it) {
	action = static_cast<HostL07Action*>(&*it);
    if (action->m_latency > 0) {
      if (min < 0) {
        min = action->m_latency;
        XBT_DEBUG("Updating min (value) with %p (start %f): %f", action,
               action->getStartTime(), min);
      } else if (action->m_latency < min) {
        min = action->m_latency;
        XBT_DEBUG("Updating min (latency) with %p (start %f): %f", action,
               action->getStartTime(), min);
      }
    }
  }

  XBT_DEBUG("min value : %f", min);

  return min;
}

void HostL07Model::updateActionsState(double /*now*/, double delta)
{
  double deltap = 0.0;
  HostL07Action *action;

  ActionList *actionSet = getRunningActionSet();

  for(ActionList::iterator it(actionSet->begin()), itNext = it, itend(actionSet->end())
	 ; it != itend ; it=itNext) {
	++itNext;
    action = static_cast<HostL07Action*>(&*it);
    deltap = delta;
    if (action->m_latency > 0) {
      if (action->m_latency > deltap) {
        double_update(&(action->m_latency), deltap, sg_surf_precision);
        deltap = 0.0;
      } else {
        double_update(&(deltap), action->m_latency, sg_surf_precision);
        action->m_latency = 0.0;
      }
      if ((action->m_latency == 0.0) && (action->isSuspended() == 0)) {
        action->updateBound();
        lmm_update_variable_weight(ptask_maxmin_system, action->getVariable(), 1.0);
      }
    }
    XBT_DEBUG("Action (%p) : remains (%g) updated by %g.",
           action, action->getRemains(), lmm_variable_getvalue(action->getVariable()) * delta);
    action->updateRemains(lmm_variable_getvalue(action->getVariable()) * delta);

    if (action->getMaxDuration() != NO_MAX_DURATION)
      action->updateMaxDuration(delta);

    XBT_DEBUG("Action (%p) : remains (%g).",
           action, action->getRemains());
    if ((action->getRemains() <= 0) &&
        (lmm_get_variable_weight(action->getVariable()) > 0)) {
      action->finish();
      action->setState(SURF_ACTION_DONE);
    } else if ((action->getMaxDuration() != NO_MAX_DURATION) &&
               (action->getMaxDuration() <= 0)) {
      action->finish();
     action->setState(SURF_ACTION_DONE);
    } else {
      /* Need to check that none of the model has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;
      void *constraint_id = NULL;

      while ((cnst = lmm_get_cnst_from_var(ptask_maxmin_system, action->getVariable(),
                                    i++))) {
        constraint_id = lmm_constraint_id(cnst);

        if (static_cast<Host*>(constraint_id)->getState() == SURF_RESOURCE_OFF) {
          XBT_DEBUG("Action (%p) Failed!!", action);
          action->finish();
          action->setState(SURF_ACTION_FAILED);
          break;
        }
      }
    }
  }
  return;
}

Action *HostL07Model::executeParallelTask(int host_nb,
                                                   void **host_list,
                                                   double *flops_amount,
												   double *bytes_amount,
                                                   double rate)
{
  HostL07Action *action;
  int i, j;
  unsigned int cpt;
  int nb_link = 0;
  int nb_host = 0;
  double latency = 0.0;

  if (ptask_parallel_task_link_set == NULL)
    ptask_parallel_task_link_set = xbt_dict_new_homogeneous(NULL);

  xbt_dict_reset(ptask_parallel_task_link_set);

  /* Compute the number of affected resources... */
  for (i = 0; i < host_nb; i++) {
    for (j = 0; j < host_nb; j++) {
      xbt_dynar_t route=NULL;

      if (bytes_amount[i * host_nb + j] > 0) {
        double lat=0.0;
        unsigned int cpt;
        void *_link;
        LinkL07 *link;

        routing_platf->getRouteAndLatency(static_cast<HostL07*>(host_list[i])->p_netElm,
        		                          static_cast<HostL07*>(host_list[j])->p_netElm,
        		                          &route,
        		                          &lat);
        latency = MAX(latency, lat);

        xbt_dynar_foreach(route, cpt, _link) {
           link = static_cast<LinkL07*>(_link);
           xbt_dict_set(ptask_parallel_task_link_set, link->getName(), link, NULL);
        }
      }
    }
  }

  nb_link = xbt_dict_length(ptask_parallel_task_link_set);
  xbt_dict_reset(ptask_parallel_task_link_set);

  for (i = 0; i < host_nb; i++)
    if (flops_amount[i] > 0)
      nb_host++;

  action = new HostL07Action(this, 1, 0);
  XBT_DEBUG("Creating a parallel task (%p) with %d cpus and %d links.",
         action, host_nb, nb_link);
  action->m_suspended = 0;        /* Should be useless because of the
                                   calloc but it seems to help valgrind... */
  action->m_hostNb = host_nb;
  action->p_hostList = (Host **) host_list;
  action->p_computationAmount = flops_amount;
  action->p_communicationAmount = bytes_amount;
  action->m_latency = latency;
  action->m_rate = rate;

  action->p_variable = lmm_variable_new(ptask_maxmin_system, action, 1.0,
                       (action->m_rate > 0) ? action->m_rate : -1.0,
                       host_nb + nb_link);

  if (action->m_latency > 0)
    lmm_update_variable_weight(ptask_maxmin_system, action->getVariable(), 0.0);

  for (i = 0; i < host_nb; i++)
    lmm_expand(ptask_maxmin_system,
    	         static_cast<HostL07*>(host_list[i])->p_cpu->getConstraint(),
               action->getVariable(), flops_amount[i]);

  for (i = 0; i < host_nb; i++) {
    for (j = 0; j < host_nb; j++) {
      void *_link;
      LinkL07 *link;

      xbt_dynar_t route=NULL;
      if (bytes_amount[i * host_nb + j] == 0.0)
        continue;

      routing_platf->getRouteAndLatency(static_cast<HostL07*>(host_list[i])->p_netElm,
                                        static_cast<HostL07*>(host_list[j])->p_netElm,
    		                            &route, NULL);

      xbt_dynar_foreach(route, cpt, _link) {
        link = static_cast<LinkL07*>(_link);
        lmm_expand_add(ptask_maxmin_system, link->getConstraint(),
                       action->getVariable(),
                       bytes_amount[i * host_nb + j]);
      }
    }
  }

  if (nb_link + nb_host == 0) {
    action->setCost(1.0);
    action->setRemains(0.0);
  }

  return action;
}

Host *HostL07Model::createHost(const char *name)
{
  HostL07 *wk = NULL;
  sg_host_t sg_host = sg_host_by_name(name);

  xbt_assert(!surf_host_resource_priv(sg_host),
              "Host '%s' declared several times in the platform file.",
              name);

  wk = new HostL07(this, name, NULL,
		                  sg_host_edge(sg_host),
						  sg_host_surfcpu(sg_host));

  xbt_lib_set(host_lib, name, SURF_HOST_LEVEL, wk);

  return wk;
}

Action *HostL07Model::communicate(Host *src, Host *dst,
                                       double size, double rate)
{
  void **host_list = xbt_new0(void *, 2);
  double *flops_amount = xbt_new0(double, 2);
  double *bytes_amount = xbt_new0(double, 4);
  Action *res = NULL;

  host_list[0] = src;
  host_list[1] = dst;
  bytes_amount[1] = size;

  res = executeParallelTask(2, host_list,
                                    flops_amount,
                                    bytes_amount, rate);

  return res;
}

xbt_dynar_t HostL07Model::getRoute(Host *src, Host *dst)
{
  xbt_dynar_t route=NULL;
  routing_platf->getRouteAndLatency(src->p_netElm, dst->p_netElm, &route, NULL);
  return route;
}

Cpu *CpuL07Model::createCpu(const char *name,  xbt_dynar_t powerPeak,
                          int pstate, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties)
{
  double power_initial = xbt_dynar_get_as(powerPeak, pstate, double);
  xbt_dynar_free(&powerPeak);   // kill memory leak
  sg_host_t sg_host = sg_host_by_name(name);

  xbt_assert(!surf_host_resource_priv(sg_host),
              "Host '%s' declared several times in the platform file.",
              name);

  CpuL07 *cpu = new CpuL07(this, name, cpu_properties,
		                     power_initial, power_scale, power_trace,
                         core, state_initial, state_trace);

  sg_host_surfcpu_set(sg_host, cpu);

  return cpu;
}

Link* NetworkL07Model::createLink(const char *name,
                                 double bw_initial,
                                 tmgr_trace_t bw_trace,
                                 double lat_initial,
                                 tmgr_trace_t lat_trace,
                                 e_surf_resource_state_t state_initial,
                                 tmgr_trace_t state_trace,
                                 e_surf_link_sharing_policy_t policy,
                                 xbt_dict_t properties)
{
  xbt_assert(!Link::byName(name),
	         "Link '%s' declared several times in the platform file.", name);

  return new LinkL07(this, name, properties,
		             bw_initial, bw_trace,
					 lat_initial, lat_trace,
					 state_initial, state_trace,
					 policy);
}

void HostL07Model::addTraces()
{
  xbt_dict_cursor_t cursor = NULL;
  char *trace_name, *elm;

  if (!trace_connect_list_host_avail)
    return;

  /* Connect traces relative to cpu */
  xbt_dict_foreach(trace_connect_list_host_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuL07 *host = static_cast<CpuL07*>(sg_host_surfcpu(sg_host_by_name(elm)));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_stateEvent = tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuL07 *host = static_cast<CpuL07*>(sg_host_surfcpu(sg_host_by_name(elm)));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_powerEvent = tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }

  /* Connect traces relative to network */
  xbt_dict_foreach(trace_connect_list_link_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    LinkL07 *link = static_cast<LinkL07*>(Link::byName(elm));

    xbt_assert(link, "Link %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    link->p_stateEvent = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_bandwidth, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    LinkL07 *link = static_cast<LinkL07*>(Link::byName(elm));

    xbt_assert(link, "Link %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    link->p_bwEvent = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }

  xbt_dict_foreach(trace_connect_list_latency, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    LinkL07 *link = static_cast<LinkL07*>(Link::byName(elm));

    xbt_assert(link, "Link %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    link->p_latEvent = tmgr_history_add_trace(history, trace, 0.0, 0, link);
  }
}

/************
 * Resource *
 ************/

HostL07::HostL07(HostModel *model, const char* name, xbt_dict_t props, RoutingEdge *netElm, Cpu *cpu)
  : Host(model, name, props, NULL, netElm, cpu)
{
}

double HostL07::getPowerPeakAt(int /*pstate_index*/)
{
	THROW_UNIMPLEMENTED;
}

int HostL07::getNbPstates()
{
	THROW_UNIMPLEMENTED;
}

void HostL07::setPstate(int /*pstate_index*/)
{
	THROW_UNIMPLEMENTED;
}

int HostL07::getPstate()
{
	THROW_UNIMPLEMENTED;
}

double HostL07::getConsumedEnergy()
{
	THROW_UNIMPLEMENTED;
}

CpuL07::CpuL07(CpuL07Model *model, const char* name, xbt_dict_t props,
	             double power_initial, double power_scale, tmgr_trace_t power_trace,
		           int core, e_surf_resource_state_t state_initial, tmgr_trace_t state_trace)
 : Cpu(model, name, props, lmm_constraint_new(ptask_maxmin_system, this, power_initial * power_scale),
	   core, power_initial, power_scale)
{
  xbt_assert(m_powerScale > 0, "Power has to be >0");

  if (power_trace)
    p_powerEvent = tmgr_history_add_trace(history, power_trace, 0.0, 0, this);
  else
    p_powerEvent = NULL;

  setState(state_initial);
  if (state_trace)
	p_stateEvent = tmgr_history_add_trace(history, state_trace, 0.0, 0, this);
}

LinkL07::LinkL07(NetworkL07Model *model, const char* name, xbt_dict_t props,
		         double bw_initial,
		         tmgr_trace_t bw_trace,
		         double lat_initial,
		         tmgr_trace_t lat_trace,
		         e_surf_resource_state_t state_initial,
		         tmgr_trace_t state_trace,
		         e_surf_link_sharing_policy_t policy)
 : Link(model, name, props, lmm_constraint_new(ptask_maxmin_system, this, bw_initial), history, state_trace)
{
  m_bwCurrent = bw_initial;
  if (bw_trace)
    p_bwEvent = tmgr_history_add_trace(history, bw_trace, 0.0, 0, this);

  setState(state_initial);
  m_latCurrent = lat_initial;

  if (lat_trace)
	p_latEvent = tmgr_history_add_trace(history, lat_trace, 0.0, 0, this);

  if (policy == SURF_LINK_FATPIPE)
	lmm_constraint_shared(getConstraint());
}

bool CpuL07::isUsed(){
  return lmm_constraint_used(ptask_maxmin_system, getConstraint());
}

bool LinkL07::isUsed(){
  return lmm_constraint_used(ptask_maxmin_system, getConstraint());
}

void CpuL07::updateState(tmgr_trace_event_t event_type, double value, double /*date*/){
  XBT_DEBUG("Updating cpu %s (%p) with value %g", getName(), this, value);
  if (event_type == p_powerEvent) {
	  m_powerScale = value;
    lmm_update_constraint_bound(ptask_maxmin_system, getConstraint(), m_powerPeak * m_powerScale);
    if (tmgr_trace_event_free(event_type))
      p_powerEvent = NULL;
  } else if (event_type == p_stateEvent) {
    if (value > 0)
      setState(SURF_RESOURCE_ON);
    else
      setState(SURF_RESOURCE_OFF);
    if (tmgr_trace_event_free(event_type))
      p_stateEvent = NULL;
  } else {
    XBT_CRITICAL("Unknown event ! \n");
    xbt_abort();
  }
  return;
}

void LinkL07::updateState(tmgr_trace_event_t event_type, double value, double date) {
  XBT_DEBUG("Updating link %s (%p) with value=%f for date=%g", getName(), this, value, date);
  if (event_type == p_bwEvent) {
    updateBandwidth(value, date);
    if (tmgr_trace_event_free(event_type))
      p_bwEvent = NULL;
  } else if (event_type == p_latEvent) {
    updateLatency(value, date);
    if (tmgr_trace_event_free(event_type))
      p_latEvent = NULL;
  } else if (event_type == p_stateEvent) {
    if (value > 0)
      setState(SURF_RESOURCE_ON);
    else
      setState(SURF_RESOURCE_OFF);
    if (tmgr_trace_event_free(event_type))
      p_stateEvent = NULL;
  } else {
    XBT_CRITICAL("Unknown event ! \n");
    xbt_abort();
  }
  return;
}

e_surf_resource_state_t HostL07::getState() {
  return p_cpu->getState();
}

Action *HostL07::execute(double size)
{
  void **host_list = xbt_new0(void *, 1);
  double *flops_amount = xbt_new0(double, 1);
  double *bytes_amount = xbt_new0(double, 1);

  host_list[0] = this;
  bytes_amount[0] = 0.0;
  flops_amount[0] = size;

  return static_cast<HostL07Model*>(getModel())->executeParallelTask(1, host_list,
		                              flops_amount,
                                     bytes_amount, -1);
}

Action *HostL07::sleep(double duration)
{
  HostL07Action *action = NULL;

  XBT_IN("(%s,%g)", getName(), duration);

  action = static_cast<HostL07Action*>(execute(1.0));
  action->m_maxDuration = duration;
  action->m_suspended = 2;
  lmm_update_variable_weight(ptask_maxmin_system, action->getVariable(), 0.0);

  XBT_OUT();
  return action;
}

double LinkL07::getBandwidth()
{
  return m_bwCurrent;
}

void LinkL07::updateBandwidth(double value, double date)
{
  m_bwCurrent = value;
  lmm_update_constraint_bound(ptask_maxmin_system, getConstraint(), m_bwCurrent);
}

double LinkL07::getLatency()
{
  return m_latCurrent;
}

void LinkL07::updateLatency(double value, double date)
{
  lmm_variable_t var = NULL;
  HostL07Action *action;
  lmm_element_t elem = NULL;

  m_latCurrent = value;
  while ((var = lmm_get_var_from_cnst(ptask_maxmin_system, getConstraint(), &elem))) {
    action = static_cast<HostL07Action*>(lmm_variable_id(var));
    action->updateBound();
  }
}


bool LinkL07::isShared()
{
  return lmm_constraint_is_shared(getConstraint());
}

/**********
 * Action *
 **********/

HostL07Action::~HostL07Action(){
  free(p_hostList);
  free(p_communicationAmount);
  free(p_computationAmount);
}

void HostL07Action::updateBound()
{
  double lat_current = 0.0;
  double lat_bound = -1.0;
  int i, j;

  for (i = 0; i < m_hostNb; i++) {
    for (j = 0; j < m_hostNb; j++) {
      xbt_dynar_t route=NULL;

      if (p_communicationAmount[i * m_hostNb + j] > 0) {
        double lat = 0.0;
        routing_platf->getRouteAndLatency(static_cast<HostL07*>(((void**)p_hostList)[i])->p_netElm,
                                          static_cast<HostL07*>(((void**)p_hostList)[j])->p_netElm,
                				          &route, &lat);

        lat_current = MAX(lat_current, lat * p_communicationAmount[i * m_hostNb + j]);
      }
    }
  }
  lat_bound = sg_tcp_gamma / (2.0 * lat_current);
  XBT_DEBUG("action (%p) : lat_bound = %g", this, lat_bound);
  if ((m_latency == 0.0) && (m_suspended == 0)) {
    if (m_rate < 0)
      lmm_update_variable_bound(ptask_maxmin_system, getVariable(), lat_bound);
    else
      lmm_update_variable_bound(ptask_maxmin_system, getVariable(), min(m_rate, lat_bound));
  }
}

int HostL07Action::unref()
{
  m_refcount--;
  if (!m_refcount) {
    if (actionHook::is_linked())
	  p_stateSet->erase(p_stateSet->iterator_to(*this));
    if (getVariable())
      lmm_variable_free(ptask_maxmin_system, getVariable());
    delete this;
    return 1;
  }
  return 0;
}

void HostL07Action::cancel()
{
  setState(SURF_ACTION_FAILED);
  return;
}

void HostL07Action::suspend()
{
  XBT_IN("(%p))", this);
  if (m_suspended != 2) {
    m_suspended = 1;
    lmm_update_variable_weight(ptask_maxmin_system, getVariable(), 0.0);
  }
  XBT_OUT();
}

void HostL07Action::resume()
{
  XBT_IN("(%p)", this);
  if (m_suspended != 2) {
    lmm_update_variable_weight(ptask_maxmin_system, getVariable(), 1.0);
    m_suspended = 0;
  }
  XBT_OUT();
}

bool HostL07Action::isSuspended()
{
  return m_suspended == 1;
}

void HostL07Action::setMaxDuration(double duration)
{                               /* FIXME: should inherit */
  XBT_IN("(%p,%g)", this, duration);
  m_maxDuration = duration;
  XBT_OUT();
}

void HostL07Action::setPriority(double priority)
{                               /* FIXME: should inherit */
  XBT_IN("(%p,%g)", this, priority);
  m_priority = priority;
  XBT_OUT();
}

double HostL07Action::getRemains()
{
  XBT_IN("(%p)", this);
  XBT_OUT();
  return m_remains;
}
