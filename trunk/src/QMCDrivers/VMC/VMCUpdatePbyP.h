//////////////////////////////////////////////////////////////////
// (c) Copyright 2003- by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//   Tel:    217-244-6319 (NCSA) 217-333-3324 (MCC)
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#ifndef QMCPLUSPLUS_VMC_PARTICLEBYPARTICLE_UPDATE_H
#define QMCPLUSPLUS_VMC_PARTICLEBYPARTICLE_UPDATE_H
#include "QMCDrivers/QMCUpdateBase.h"

namespace qmcplusplus {


  /** @ingroup QMCDrivers  ParticleByParticle
   *@brief Implements the VMC algorithm using particle-by-particle move. 
   */
  class VMCUpdatePbyP: public QMCUpdateBase {
  public:
    /// Constructor.
    VMCUpdatePbyP(MCWalkerConfiguration& w, TrialWaveFunction& psi, 
        QMCHamiltonian& h, RandomGenerator_t& rg);

    ~VMCUpdatePbyP();

    void advanceWalkers(WalkerIter_t it, WalkerIter_t it_end, bool measure);

  private:
    ///sub steps
    int nSubSteps;
    vector<NewTimer*> myTimers;
  };

  /** @ingroup QMCDrivers  ParticleByParticle
   *@brief Implements the VMC algorithm using particle-by-particle move with the drift equation. 
   */
  class VMCUpdatePbyPWithDrift: public QMCUpdateBase {
  public:
    /// Constructor.
    VMCUpdatePbyPWithDrift(MCWalkerConfiguration& w, TrialWaveFunction& psi, 
        QMCHamiltonian& h, RandomGenerator_t& rg);

    ~VMCUpdatePbyPWithDrift();

    void advanceWalkers(WalkerIter_t it, WalkerIter_t it_end, bool measure);
 
  private:
    vector<NewTimer*> myTimers;
  };

  /** @ingroup QMCDrivers  ParticleByParticle
   *@brief Implements the VMC algorithm using particle-by-particle move with the drift equation. 
   */
  class VMCUpdatePbyPWithDriftFast: public QMCUpdateBase {
  public:
    /// Constructor.
    VMCUpdatePbyPWithDriftFast(MCWalkerConfiguration& w, TrialWaveFunction& psi, 
        QMCHamiltonian& h, RandomGenerator_t& rg);

    ~VMCUpdatePbyPWithDriftFast();

    void advanceWalkers(WalkerIter_t it, WalkerIter_t it_end, bool measure);
 
  private:
    vector<NewTimer*> myTimers;
  };
}

#endif
/***************************************************************************
 * $RCSfile: VMCUpdatePbyP.h,v $   $Author: jnkim $
 * $Revision: 1.5 $   $Date: 2006/07/17 14:29:40 $
 * $Id: VMCUpdatePbyP.h,v 1.5 2006/07/17 14:29:40 jnkim Exp $ 
 ***************************************************************************/
