//////////////////////////////////////////////////////////////////
// (c) Copyright 2004- by Jeongnim Kim 
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
#ifndef QMCPLUSPLUS_PAIRCORRELATION_ESTIMATOR_H
#define QMCPLUSPLUS_PAIRCORRELATION_ESTIMATOR_H
#include "Estimators/CompositeEstimators.h"
//#include "Estimators/VectorEstimatorImpl.h"
//#include <boost/numeric/ublas/matrix.hpp>

namespace qmcplusplus {

  class GofREstimator: public CompositeEstimatorBase 
  {
    //typedef VectorEstimatorImpl<RealType> VectorEstimatorType;
    ///true if source == target
    bool Symmetric;
    /** number of centers */
    int Centers;
    /** number of distinct pair types */
    int NumPairTypes;
    /** number bins for gofr */
    int NumBins;
    /** maximum distance */
    RealType Dmax;
    /** bin size */
    RealType Delta;
    /** one of bin size */
    RealType DeltaInv;
    ///save the source particleset
    ParticleSet* sourcePtcl;
    ///save the target particleset
    ParticleSet* targetPtcl;
    /** distance table */
    const DistanceTableData*  myTable;
    /** local copy of pair index */
    vector<int> PairID;
    /** normalization factor for each bin*/
    vector<RealType> normFactor;
    /** instantaneous gofr */
    Matrix<RealType> gofrInst;
    public:

    /** constructor
     * @param source particleset
     */
    GofREstimator(ParticleSet& source);

    /** constructor
     * @param source particleset
     * @param target particleset
     */
    GofREstimator(ParticleSet& source, ParticleSet& target);

    /** virtal destrctor */
    ~GofREstimator();

    void resetTargetParticleSet(ParticleSet& p);
    /** prepare data collect */
    void startAccumulate();
    /** accumulate the observables */
    void accumulate(ParticleSet& p);
    /** reweight of the current cummulative  values */
    void stopAccumulate();
    void setBound(RealType dr);

    CompositeEstimatorBase* clone();
    
    private:
    GofREstimator(const GofREstimator& pc): sourcePtcl(pc.sourcePtcl) {}
  };
}

#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1415 $   $Date: 2006-10-23 11:51:53 -0500 (Mon, 23 Oct 2006) $
 * $Id: CompositeEstimatorBase.h 1415 2006-10-23 16:51:53Z jnkim $ 
 ***************************************************************************/
