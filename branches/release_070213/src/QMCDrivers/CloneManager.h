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
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
/** @file CloneManager.h
 * @brief Manager class to handle multiple threads
 */
#ifndef QMCPLUSPLUS_CLONEMANAGER_H
#define QMCPLUSPLUS_CLONEMANAGER_H
#include "QMCDrivers/QMCUpdateBase.h" 

namespace qmcplusplus {

  class HamiltonianPool;

  /** Manager clones for threaded applications
   *
   * Clones for the ParticleSet, TrialWaveFunction and QMCHamiltonian
   * are static to ensure only one set of clones persist during a run.
   */
  class CloneManager: public QMCTraits {
  public:
    /// Constructor.
    CloneManager(HamiltonianPool& hpool);
    ///virtual destructor
    virtual ~CloneManager();

    void makeClones(MCWalkerConfiguration& w, TrialWaveFunction& psi, QMCHamiltonian& ham);

    RealType acceptRatio() const 
    {
      IndexType nAcceptTot=0;
      IndexType nRejectTot=0;
      for(int ip=0; ip<NumThreads; ip++) {
        nAcceptTot+=Movers[ip]->nAccept;
        nRejectTot+=Movers[ip]->nReject;
      }
      return static_cast<RealType>(nAcceptTot)/static_cast<RealType>(nAcceptTot+nRejectTot);
    }

  protected:
    ///reference to HamiltonianPool to clone everything
    HamiltonianPool& cloneEngine;
    ///number of threads
    IndexType NumThreads;
    ///walkers
    static vector<ParticleSet*> wClones;
    ///trial wavefunctions
    static vector<TrialWaveFunction*> psiClones;
    ///Hamiltonians
    static vector<QMCHamiltonian*> hClones;
    ///Random number generators
    vector<RandomGenerator_t*> Rng;
    ///update engines
    vector<QMCUpdateBase*> Movers;
    ///Brnach engines
    vector<SimpleFixedNodeBranch*> branchClones;
    ///Walkers per node
    vector<int> wPerNode;
  };
}
#endif
/***************************************************************************
 * $RCSfile: DMCPbyPOMP.h,v $   $Author: jnkim $
 * $Revision: 1.2 $   $Date: 2006/02/26 17:41:10 $
 * $Id: DMCPbyPOMP.h,v 1.2 2006/02/26 17:41:10 jnkim Exp $ 
 ***************************************************************************/
