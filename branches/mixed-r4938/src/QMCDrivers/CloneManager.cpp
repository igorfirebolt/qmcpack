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
#include "QMCDrivers/CloneManager.h"
#include "QMCApp/HamiltonianPool.h"
#include "Message/Communicate.h"
#include "Message/OpenMP.h"
#include "Utilities/IteratorUtility.h"

//comment this out to use only method to clone
#define ENABLE_CLONE_PSI_AND_H

namespace qmcplusplus { 

  //initialization of the static wClones
  vector<MCWalkerConfiguration*> CloneManager::wClones;
  //initialization of the static psiClones
  vector<TrialWaveFunction*> CloneManager::psiClones;
  //initialization of the static hClones
  vector<QMCHamiltonian*> CloneManager::hClones;

  /// Constructor.
  CloneManager::CloneManager(HamiltonianPool& hpool): cloneEngine(hpool)
  {
    NumThreads=omp_get_max_threads();
    wPerNode.resize(NumThreads+1,0);
  }

  ///clenup non-static data members
  CloneManager::~CloneManager()
  {
    delete_iter(Rng.begin(),Rng.end());
    delete_iter(Movers.begin(),Movers.end());
    delete_iter(branchClones.begin(),branchClones.end());
    delete_iter(estimatorClones.begin(),estimatorClones.end());
  }

  void CloneManager::makeClones(MCWalkerConfiguration& w, 
      TrialWaveFunction& psi, QMCHamiltonian& ham)
  {

    if(wClones.size()) 
    {
      app_log() << "  Cannot make clones again. Use existing " << NumThreads << " clones" << endl;
      return;
    }

    wClones.resize(NumThreads,0);
    psiClones.resize(NumThreads,0);
    hClones.resize(NumThreads,0);

    wClones[0]=&w;
    psiClones[0]=&psi;
    hClones[0]=&ham;

    if(NumThreads==1) return;

    app_log() << "  CloneManager::makeClones makes " << NumThreads << " clones for W/Psi/H." <<endl;
#if defined(ENABLE_CLONE_PSI_AND_H)
    app_log() << "  Cloning methods for both Psi and H are used" << endl;
    OhmmsInfo::Log->turnoff();
    OhmmsInfo::Warn->turnoff();
    char pname[16];
    for(int ip=1; ip<NumThreads; ++ip) 
    {
      wClones[ip]=new MCWalkerConfiguration(w);
      psiClones[ip]=psi.makeClone(*wClones[ip]);
      hClones[ip]=ham.makeClone(*wClones[ip],*psiClones[ip]);
    }
    OhmmsInfo::Log->reset();
    OhmmsInfo::Warn->reset();
#else
    app_log() << "Old parse method is used." << endl;
    cloneEngine.clone(w,psi,ham,wClones,psiClones,hClones);
#endif
  }
}

/***************************************************************************
 * $RCSfile: CloneManager.cpp,v $   $Author: jnkim $
 * $Revision: 1.5 $   $Date: 2006/10/18 17:23:35 $
 * $Id:CloneManagerDMCPbyPOMP.cpp,v 1.5 2006/10/18 17:23:35 jnkim Exp $ 
 ***************************************************************************/
