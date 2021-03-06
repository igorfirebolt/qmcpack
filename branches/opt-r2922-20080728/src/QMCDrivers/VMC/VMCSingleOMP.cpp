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
#include "QMCDrivers/VMC/VMCSingleOMP.h"
#include "QMCDrivers/VMC/VMCUpdatePbyP.h"
#include "QMCDrivers/VMC/VMCUpdateAll.h"
#include "OhmmsApp/RandomNumberControl.h"
#include "Message/OpenMP.h"
//#define ENABLE_VMC_OMP_MASTER

namespace qmcplusplus { 

  /// Constructor.
  VMCSingleOMP::VMCSingleOMP(MCWalkerConfiguration& w, TrialWaveFunction& psi, QMCHamiltonian& h,
      HamiltonianPool& hpool):
    QMCDriver(w,psi,h),  CloneManager(hpool), UseDrift("yes") 
    { 
    RootName = "vmc";
    QMCType ="VMCSingleOMP";
    QMCDriverMode.set(QMC_UPDATE_MODE,1);
    m_param.add(UseDrift,"useDrift","string"); m_param.add(UseDrift,"usedrift","string");
  }
  
  bool VMCSingleOMP::run() 
  { 
    resetRun();

    //start the main estimator
    Estimators->start(nBlocks);

#pragma omp parallel
    {
      int now=1;

#pragma omp for  nowait
      for(int ip=0; ip<NumThreads; ++ip) 
        Movers[ip]->startRun(nBlocks,false);

      for(int block=0;block<nBlocks; ++block)
      {
#pragma omp for 
        for(int ip=0; ip<NumThreads; ++ip)
        {
          //assign the iterators and resuse them
          MCWalkerConfiguration::iterator wit(W.begin()+wPerNode[ip]), wit_end(W.begin()+wPerNode[ip+1]);

          //if(QMCDriverMode[QMC_UPDATE_MODE]&&now%100==0) 
          //  Movers[ip]->updateWalkers(wit,wit_end);
          
          Movers[ip]->startBlock(nSteps);
          int now_loc=now;
          for(int step=0; step<nSteps;++step)
          {
            Movers[ip]->advanceWalkers(wit,wit_end,false);
            Movers[ip]->accumulate(wit,wit_end);

            ++now_loc;
            //save walkers for optimization
            if(QMCDriverMode[QMC_OPTIMIZE]&&now_loc%Period4WalkerDump==0) wClones[ip]->saveEnsemble(wit,wit_end);
          } 
          Movers[ip]->stopBlock();
        }//end-of-parallel for

        //increase now
        now+=nSteps;
#pragma omp master
        {
          CurrentStep+=nSteps;
          Estimators->stopBlock(estimatorClones);
          recordBlock(block+1);
        }//end of mater
      }//block
    }//end of parallel
    Estimators->stop(estimatorClones);

    //copy back the random states
    for(int ip=0; ip<NumThreads; ++ip) 
      *(RandomNumberControl::Children[ip])=*(Rng[ip]);

    //finalize a qmc section
    return finalize(nBlocks);
  }

  void VMCSingleOMP::resetRun() 
  {

    makeClones(W,Psi,H);

    if(Movers.empty()) 
    {
      Movers.resize(NumThreads,0);
      branchClones.resize(NumThreads,0);
      estimatorClones.resize(NumThreads,0);
      Rng.resize(NumThreads,0);
      int nwtot=(W.getActiveWalkers()/NumThreads)*NumThreads;
      FairDivideLow(nwtot,NumThreads,wPerNode);

      app_log() << "  Initial partition of walkers ";
      std::copy(wPerNode.begin(),wPerNode.end(),ostream_iterator<int>(app_log()," "));
      app_log() << endl;

#pragma omp parallel  
      {
        int ip = omp_get_thread_num();
        //if(ip) hClones[ip]->add2WalkerProperty(*wClones[ip]);
        estimatorClones[ip]= new EstimatorManager(*Estimators);//,*hClones[ip]);  
        estimatorClones[ip]->resetTargetParticleSet(*wClones[ip]);
        estimatorClones[ip]->setCollectionMode(false);
        Rng[ip]=new RandomGenerator_t(*(RandomNumberControl::Children[ip]));
        hClones[ip]->setRandomGenerator(Rng[ip]);

        branchClones[ip] = new BranchEngineType(*branchEngine);

        if(QMCDriverMode[QMC_UPDATE_MODE])
        {
          if(UseDrift == "yes")
            Movers[ip]=new VMCUpdatePbyPWithDrift(*wClones[ip],*psiClones[ip],*hClones[ip],*Rng[ip]); 
          else
            Movers[ip]=new VMCUpdatePbyP(*wClones[ip],*psiClones[ip],*hClones[ip],*Rng[ip]); 
          Movers[ip]->resetRun(branchClones[ip],estimatorClones[ip]);
        }
        else
        {
          if(UseDrift == "yes")
            Movers[ip]=new VMCUpdateAllWithDrift(*wClones[ip],*psiClones[ip],*hClones[ip],*Rng[ip]); 
          else
            Movers[ip]=new VMCUpdateAll(*wClones[ip],*psiClones[ip],*hClones[ip],*Rng[ip]); 
          Movers[ip]->resetRun(branchClones[ip],estimatorClones[ip]);
        }
      }
    }

#pragma omp parallel  for
    for(int ip=0; ip<NumThreads; ++ip)
    {
      if(QMCDriverMode[QMC_UPDATE_MODE])
        Movers[ip]->initWalkersForPbyP(W.begin()+wPerNode[ip],W.begin()+wPerNode[ip+1]);
      else
        Movers[ip]->initWalkers(W.begin()+wPerNode[ip],W.begin()+wPerNode[ip+1]);
    }
    //Used to debug and benchmark opnemp
    //#pragma omp parallel for
    //    for(int ip=0; ip<NumThreads; ip++)
    //    {
    //      Movers[ip]->benchMark(W.begin()+wPerNode[ip],W.begin()+wPerNode[ip+1],ip);
    //    }
  }

  bool 
  VMCSingleOMP::put(xmlNodePtr q){
    //nothing to add
    return true;
  }
}

/***************************************************************************
 * $RCSfile: VMCSingleOMP.cpp,v $   $Author: jnkim $
 * $Revision: 1.25 $   $Date: 2006/10/18 17:03:05 $
 * $Id: VMCSingleOMP.cpp,v 1.25 2006/10/18 17:03:05 jnkim Exp $ 
 ***************************************************************************/
