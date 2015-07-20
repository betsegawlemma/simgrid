/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "host_interface.hpp"

#ifndef VM_INTERFACE_HPP_
#define VM_INTERFACE_HPP_

#define GUESTOS_NOISE 100 // This value corresponds to the cost of the global action associated to the VM
                          // It corresponds to the cost of a VM running no tasks.

/***********
 * Classes *
 ***********/

class VMModel;
class VM;

/*************
 * Callbacks *
 *************/

/** @ingroup SURF_callbacks
 * @brief Callbacks fired after VM creation. Signature: `void(VM*)`
 */
extern surf_callback(void, VM*) VMCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks fired after VM destruction. Signature: `void(VM*)`
 */
extern surf_callback(void, VM*) VMDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks after VM State changes. Signature: `void(VMAction*)`
 */
extern surf_callback(void, VM*) VMStateChangedCallbacks;

/*********
 * Model *
 *********/
/** @ingroup SURF_vm_interface
 * @brief SURF VM model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class VMModel : public HostModel {
public:
  VMModel() :HostModel(){}
  ~VMModel(){};

  Host *createHost(const char *name){DIE_IMPOSSIBLE;}

  /**
   * @brief Create a new VM
   *
   * @param name The name of the new VM
   * @param host_PM The real machine hosting the VM
   *
   */
  virtual VM *createVM(const char *name, surf_resource_t host_PM)=0;
  void adjustWeightOfDummyCpuActions() {};

  typedef boost::intrusive::list<VM, boost::intrusive::constant_time_size<false> > vm_list_t;
  static vm_list_t ws_vms;
};

/************
 * Resource *
 ************/

/** @ingroup SURF_vm_interface
 * @brief SURF VM interface class
 * @details A VM represent a virtual machine
 */
class VM : public Host,
           public boost::intrusive::list_base_hook<> {
public:
  /**
   * @brief Constructor
   *
   * @param model VMModel associated to this VM
   * @param name The name of the VM
   * @param props Dictionary of properties associated to this VM
   * @param netElm The RoutingEdge associated to this VM
   * @param cpu The Cpu associated to this VM
   */
  VM(Model *model, const char *name, xbt_dict_t props,
		        RoutingEdge *netElm, Cpu *cpu);

  /** @brief Destructor */
  ~VM();

  void setState(e_surf_resource_state_t state);

  /** @brief Suspend the VM */
  virtual void suspend()=0;

  /** @brief Resume the VM */
  virtual void resume()=0;

  /** @brief Save the VM (Not yet implemented) */
  virtual void save()=0;

  /** @brief Restore the VM (Not yet implemented) */
  virtual void restore()=0;

  /** @brief Migrate the VM to the destination host */
  virtual void migrate(surf_resource_t dest_PM)=0;

  /** @brief Get the physical machine hosting the VM */
  virtual surf_resource_t getPm()=0;

  virtual void setBound(double bound)=0;
  virtual void setAffinity(Cpu *cpu, unsigned long mask)=0;

  /* The vm object of the lower layer */
  CpuAction *p_action;
  Host *p_subWs;  // Pointer to the ''host'' OS
  e_surf_vm_state_t p_currentState;
};

/**********
 * Action *
 **********/

#endif /* VM_INTERFACE_HPP_ */
