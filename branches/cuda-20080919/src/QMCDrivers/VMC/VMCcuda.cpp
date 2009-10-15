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
#include "QMCDrivers/VMC/VMCcuda.h"
#include "OhmmsApp/RandomNumberControl.h"
#include "Utilities/RandomGenerator.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "QMCDrivers/DriftOperators.h"

namespace qmcplusplus { 

  /// Constructor.
  VMCcuda::VMCcuda(MCWalkerConfiguration& w, TrialWaveFunction& psi, 
		   QMCHamiltonian& h):
    QMCDriver(w,psi,h), myWarmupSteps(0), UseDrift("yes"),    nSubSteps(1)
  { 
    RootName = "vmc";
    QMCType ="VMCcuda";
    QMCDriverMode.set(QMC_UPDATE_MODE,1);
    QMCDriverMode.set(QMC_WARMUP,0);
    m_param.add(UseDrift,"useDrift","string"); 
    m_param.add(UseDrift,"usedrift","string");
    m_param.add(myWarmupSteps,"warmupSteps","int");
    m_param.add(nTargetSamples,"targetWalkers","int");
    m_param.add(nSubSteps, "substeps", "int");
    m_param.add(nSubSteps, "subSteps", "int");
  }
  
  bool VMCcuda::run() { 
    if (UseDrift == "yes")
      return runWithDrift();

    resetRun();
    IndexType block = 0;
    IndexType nAcceptTot = 0;
    IndexType nRejectTot = 0;
    IndexType updatePeriod= (QMCDriverMode[QMC_UPDATE_MODE]) 
      ? Period4CheckProperties 
      : (nBlocks+1)*nSteps;
    
    int nat = W.getTotalNum();
    int nw  = W.getActiveWalkers();
    
    vector<RealType>  LocalEnergy(nw);
    vector<PosType>   delpos(nw);
    vector<PosType>   newpos(nw);
    vector<ValueType> ratios(nw);
    vector<GradType>  oldG(nw), newG(nw);
    vector<ValueType> oldL(nw), newL(nw);
    vector<Walker_t*> accepted(nw);
    Matrix<ValueType> lapl(nw, nat);
    Matrix<GradType>  grad(nw, nat);
    double Esum;

    do {
      IndexType step = 0;
      nAccept = nReject = 0;
      Esum = 0.0;
      Estimators->startBlock(nSteps);
      do
      {
        ++step;++CurrentStep;
	for (int isub=0; isub<nSubSteps; isub++) {
	  for(int iat=0; iat<nat; ++iat)  {
	    //create a 3N-Dimensional Gaussian with variance=1
	    makeGaussRandomWithEngine(delpos,Random);
	    for(int iw=0; iw<nw; ++iw) {
	      PosType G = W[iw]->Grad[iat];
	      newpos[iw]=W[iw]->R[iat] + m_sqrttau*delpos[iw];
	      ratios[iw] = 1.0;
	    }
	    W.proposeMove_GPU(newpos, iat);
	    
	    Psi.ratio(W,iat,ratios,newG, newL);
	    
	    accepted.clear();
	    vector<bool> acc(nw, false);
	    for(int iw=0; iw<nw; ++iw) {
	      if(ratios[iw]*ratios[iw] > Random()) {
		accepted.push_back(W[iw]);
		nAccept++;
		W[iw]->R[iat] = newpos[iw];
		acc[iw] = true;
	      }
	      else 
		nReject++;
	    }
	    W.acceptMove_GPU(acc);
	    if (accepted.size())
	      Psi.update(accepted,iat);
	  }
	  // cerr << "Rank = " << myComm->rank() <<
	  //   "  CurrentStep = " << CurrentStep << 
	  //   "  isub = " << isub << endl;
	}
	
	Psi.gradLapl(W, grad, lapl);
	H.evaluate (W, LocalEnergy);
	Estimators->accumulate(W);
      } while(step<nSteps);
      Psi.recompute(W);

      // vector<RealType> logPsi(W.WalkerList.size(), 0.0);
      // Psi.evaluateLog(W, logPsi);
      
      double accept_ratio = (double)nAccept/(double)(nAccept+nReject);
      Estimators->stopBlock(accept_ratio);

      nAcceptTot += nAccept;
      nRejectTot += nReject;
      ++block;
      recordBlock(block);
    } while(block<nBlocks);

    //Mover->stopRun();

    //finalize a qmc section
    return finalize(block);
  }



  bool VMCcuda::runWithDrift() 
  { 
    resetRun();
    IndexType block = 0;
    IndexType nAcceptTot = 0;
    IndexType nRejectTot = 0;
    int nat = W.getTotalNum();
    int nw  = W.getActiveWalkers();
    
    vector<RealType>  LocalEnergy(nw), oldScale(nw), newScale(nw);
    vector<PosType>   delpos(nw);
    vector<PosType>   dr(nw);
    vector<PosType>   newpos(nw);
    vector<ValueType> ratios(nw), rplus(nw), rminus(nw);
    vector<PosType>  oldG(nw), newG(nw);
    vector<ValueType> oldL(nw), newL(nw);
    vector<Walker_t*> accepted(nw);
    Matrix<ValueType> lapl(nw, nat);
    Matrix<GradType>  grad(nw, nat);

    do {
      IndexType step = 0;
      nAccept = nReject = 0;
      Estimators->startBlock(nSteps);
      do {
        step++;
	CurrentStep++;
	for (int isub=0; isub<nSubSteps; isub++) {
	  for(int iat=0; iat<nat; iat++) {
	    Psi.getGradient (W, iat, oldG);
	    
	    //create a 3N-Dimensional Gaussian with variance=1
	    makeGaussRandomWithEngine(delpos,Random);
	    for(int iw=0; iw<nw; iw++) {
	      oldScale[iw] = getDriftScale(m_tauovermass,oldG[iw]);
	      dr[iw] = (m_sqrttau*delpos[iw]) + (oldScale[iw]*oldG[iw]);
	      newpos[iw]=W[iw]->R[iat] + dr[iw];
	      ratios[iw] = 1.0;
	    }
	    W.proposeMove_GPU(newpos, iat);
	    
	    Psi.ratio(W,iat,ratios,newG, newL);
	    
	    accepted.clear();
	    vector<bool> acc(nw, false);
	    for(int iw=0; iw<nw; ++iw) {
	      PosType drOld = 
		newpos[iw] - (W[iw]->R[iat] + oldScale[iw]*oldG[iw]);
	      // if (dot(drOld, drOld) > 25.0)
	      //   cerr << "Large drift encountered!  Old drift = " << drOld << endl;
	      RealType logGf = -m_oneover2tau * dot(drOld, drOld);
	      newScale[iw]   = getDriftScale(m_tauovermass,newG[iw]);
	      PosType drNew  = 
		(newpos[iw] + newScale[iw]*newG[iw]) - W[iw]->R[iat];
	      // if (dot(drNew, drNew) > 25.0)
	      //   cerr << "Large drift encountered!  Drift = " << drNew << endl;
	      RealType logGb =  -m_oneover2tau * dot(drNew, drNew);
	      RealType x = logGb - logGf;
	      RealType prob = ratios[iw]*ratios[iw]*std::exp(x);
	      
	      if(Random() < prob) {
		accepted.push_back(W[iw]);
		nAccept++;
		W[iw]->R[iat] = newpos[iw];
		acc[iw] = true;
	      }
	      else 
		nReject++;
	    }
	    W.acceptMove_GPU(acc);
	    if (accepted.size())
	      Psi.update(accepted,iat);
	  }
	  if (Period4WalkerDump && (CurrentStep % myPeriod4WalkerDump)==0) 
	     W.saveEnsemble();
	   // cerr << "Rank = " << myComm->rank() <<
	  //   "  CurrentStep = " << CurrentStep << "  isub = " << isub << endl;

	}
	Psi.gradLapl(W, grad, lapl);
	H.evaluate (W, LocalEnergy);
	Estimators->accumulate(W);
      } while(step<nSteps);
      Psi.recompute(W);
      
      double accept_ratio = (double)nAccept/(double)(nAccept+nReject);
      Estimators->stopBlock(accept_ratio);

      nAcceptTot += nAccept;
      nRejectTot += nReject;
      ++block;
      recordBlock(block);
    } while(block<nBlocks);
    //finalize a qmc section
    return finalize(block);
  }





  void VMCcuda::resetRun()
  {
    SpeciesSet tspecies(W.getSpeciesSet());
    int massind=tspecies.addAttribute("mass");
    RealType mass = tspecies(massind,0);
    RealType oneovermass = 1.0/mass;
    RealType oneoversqrtmass = std::sqrt(oneovermass);
    m_oneover2tau = 0.5*mass/Tau;
    m_sqrttau = std::sqrt(Tau/mass);
    m_tauovermass = Tau/mass;

    // Compute the size of data needed for each walker on the GPU card
    PointerPool<Walker_t::cuda_Buffer_t > pool;
    Psi.reserve (pool);
    app_log() << "Each walker requires " << pool.getTotalSize() * sizeof(CudaRealType)
	      << " bytes in GPU memory.\n";

    // Now allocate memory on the GPU card for each walker
    for (int iw=0; iw<W.WalkerList.size(); iw++) {
      Walker_t &walker = *(W.WalkerList[iw]);
      pool.allocate(walker.cuda_DataSet);
    }
    app_log() << "Successfully allocated walkers.\n";
    W.copyWalkersToGPU();
    W.updateLists_GPU();
    vector<RealType> logPsi(W.WalkerList.size(), 0.0);
    //Psi.evaluateLog(W, logPsi);
    Psi.recompute(W, true);
    Estimators->start(nBlocks, true);
  }

  bool 
  VMCcuda::put(xmlNodePtr q){
    //nothing to add
    return true;
  }
}

/***************************************************************************
 * $RCSfile: VMCParticleByParticle.cpp,v $   $Author: jnkim $
 * $Revision: 1.25 $   $Date: 2006/10/18 17:03:05 $
 * $Id: VMCParticleByParticle.cpp,v 1.25 2006/10/18 17:03:05 jnkim Exp $ 
 ***************************************************************************/
