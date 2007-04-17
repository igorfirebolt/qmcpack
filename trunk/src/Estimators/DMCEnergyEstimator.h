//////////////////////////////////////////////////////////////////
// (c) Copyright 2006- by Jeongnim Kim 
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#ifndef QMCPLUSPLUS_DMC_LOCALENERGYESTIMATOR_H
#define QMCPLUSPLUS_DMC_LOCALENERGYESTIMATOR_H
#include "Message/CommOperators.h"
#include "Estimators/ScalarEstimatorBase.h"
#include "QMCDrivers/WalkerControlBase.h"

namespace qmcplusplus {

  /** EnergyEstimator specialized for DMC
   *
   * This is a fake ScalarEstimatorBase<T> in that nothing is accumulated by
   * this object. However, it uses WalkerControlBase* which performs 
   * the functions of an estimator and more. 
   */
  struct DMCEnergyEstimator: public ScalarEstimatorBase {

    enum {ENERGY_INDEX=0, ENERGY_SQ_INDEX, WALKERSIZE_INDEX, WEIGHT_INDEX, EREF_INDEX, LE_MAX};

    ///index for LocalEnergy
    int averageIndex;
    ///index for Variance
    int varianceIndex;
    ///index for number of walkers
    int popIndex;
    ///current index
    int Current;
    ///WalkerControlBase* which actually performs accumulation
    WalkerControlBase* walkerControl;
    
    /** constructor
     */
    DMCEnergyEstimator():walkerControl(0),Current(0) {}

    void setWalkerControl(WalkerControlBase* wc) { 
      walkerControl=wc;
    }

    /**  add the local energy, variance and all the Hamiltonian components to the scalar record container
     * @param record storage of scalar records (name,value)
     * @param msg buffer for message passing
     *
     * Nothing to be added to msg.
     */
    void add2Record(RecordListType& record, BufferType& msg) {
      averageIndex = record.add("LocalEnergy");
      varianceIndex= record.add("Variance");
      popIndex     = record.add("Population");
    }

    inline void accumulate(const Walker_t& awalker, RealType wgt) {
      app_error() << "Disabled DMCEnergyEstimator::accumulate(const Walker_t& awalker, T wgt) " << endl;
    }

    inline void accumulate(ParticleSet& P, MCWalkerConfiguration::Walker_t& awalker) {
      //Current counter is not correctly implemented. 
    }

    /** accumulate a Current counter for normalizations. 
     *
     * walkerControl accumulate the local energy and other quantities.
     */
    inline RealType accumulate(WalkerIterator first, WalkerIterator last) { 
      Current++;
      return walkerControl->getValue(WEIGHT_INDEX); 
    }

    ///reset all the cumulative sums to zero
    inline void reset() { 
      walkerControl->reset();
    }

    /** copy the value to a message buffer
     *
     * msg buffer is used to collect data over MPI. There is nothing to be collected.
     */
    inline void copy2Buffer(BufferType& msg) { }

    /** calculate the averages and reset to zero
     * @param record a container class for storing scalar records (name,value)
     * @param wgtinv the inverse weight
     *
     * The weight is overwritten by 1.0/Current, since the normalized weight over
     * an ensemble, i.e., over the walkers, has been already
     * included by WalkerControlBase::branch operation.
     */
    inline void report(RecordListType& record, RealType wgtinv) {
      wgtinv=1.0/static_cast<RealType>(Current);
      d_sum=walkerControl->getValue(ENERGY_INDEX);
      d_sumsq=walkerControl->getValue(ENERGY_SQ_INDEX);
      record[averageIndex]= d_average =  wgtinv*d_sum;
      record[varianceIndex]= d_variance = wgtinv*d_sumsq-d_average*d_average;
      record[popIndex]= wgtinv*walkerControl->getValue(WALKERSIZE_INDEX);
      walkerControl->reset();
      Current=0;
      walkerControl->setEnergyAndVariance(d_average,d_variance);
    }

    inline void report(RecordListType& record, RealType wgtinv, BufferType& msg) {
      report(record,wgtinv);
    }
  };

}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
