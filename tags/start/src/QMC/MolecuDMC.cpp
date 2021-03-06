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
//   Department of Physics, Ohio State University
//   Ohio Supercomputer Center
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "QMC/MolecuDMC.h"
#include "Utilities/OhmmsInfo.h"
#include "Particle/MCWalkerConfiguration.h"
#include "Particle/DistanceTable.h"
#include "Particle/HDFWalkerIO.h"
#include "ParticleBase/ParticleUtility.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "QMCWaveFunctions/TrialWaveFunction.h"
#include "QMCHamiltonians/QMCHamiltonianBase.h"
#include "QMC/MolecuFixedNodeBranch.h"
#include "Message/CommCreate.h"
#include "Utilities/Clock.h"

namespace ohmmsqmc {

  MolecuDMC::MolecuDMC(MCWalkerConfiguration& w, 
		       TrialWaveFunction& psi, 
		       QMCHamiltonian& h, 
		       xmlNodePtr q): 
    QMCDriver(w,psi,h,q) { 
    RootName = "dmc";
    QMCType ="dmc";
  }
  
  /** Advance the walkers nblocks*nsteps timesteps. 
   * @param nblocks number of blocks
   * @param nsteps number of steps
   * @param tau the timestep
   *
   * For each timestep:
   * <ul>
   * <li> Move all the particles of a walker.
   * <li> Calculate the properties for the new walker configuration.
   * <li> Accept/reject the new configuration.
   * <li> Accumulate the estimators.
   * <li> Update the trial energy \f$ E_T. \f$
   * <li> Branch the population of walkers (birth/death algorithm).
   * </ul>
   * For each block:
   * <ul>
   * <li> Flush the estimators and print to file.
   * <li> Update the estimate of the local energy.
   * <li> (Optional) Print the ensemble of walker configurations.
   * </ul>
   * Default mode: Print the ensemble of walker configurations 
   * at the end of the run.
   */
  bool MolecuDMC::run() { 

    //create a distance table for one walker
    DistanceTable::create(1);
    
    if(put(qmc_node)){
      
      //set the data members to start a new run
      //    getReady();
      int PopIndex, E_TIndex;
      Estimators.resetReportSettings(RootName);
      AcceptIndex = Estimators.addColumn("AcceptRatio");
      PopIndex = Estimators.addColumn("Population");
      E_TIndex = Estimators.addColumn("E_T");
      Estimators.reportHeader();
      
      for(MCWalkerConfiguration::iterator it = W.begin(); 
	  it != W.end(); ++it) {
	(*it)->Properties(Weight) = 1.0;
	(*it)->Properties(Multiplicity) = 1.0;
      }
      
      MolecuFixedNodeBranch<RealType> brancher(Tau,W.getActiveWalkers());
      brancher.put(qmc_node);
      
      /*if VMC/DMC directly preceded DMC (Counter > 0) then
	use the average value of the energy estimator for
	the reference energy of the brancher*/
      if(Counter) {
	RealType e_ref = W.getLocalEnergy();
	LOGMSG("Overwriting the reference energy by the local energy " << e_ref)    
	brancher.setEguess(e_ref);
      }
      
      IndexType block = 0;
      
      Pooma::Clock timer;
      int Population = W.getActiveWalkers();
      int tPopulation = W.getActiveWalkers();
      RealType E_T = brancher.E_T;
      RealType Eest = E_T;
      RealType feed = brancher.feed;
      IndexType accstep=0;
      IndexType nAcceptTot = 0;
      IndexType nRejectTot = 0;
      
      do {
	IndexType step = 0;
	timer.start();
	do {
	  Population = W.getActiveWalkers();
	  advanceWalkerByWalker(brancher);
	  step++; accstep++;
	  Estimators.accumulate(W);
	  E_T = brancher.update(Population,Eest);
	  brancher.branch(accstep,W);
	} while(step<nSteps);
	timer.stop();
	
	nAcceptTot += nAccept;
	nRejectTot += nReject;
	Estimators.flush();
	Eest = Estimators.average(0);
	
	Estimators.setColumn(PopIndex,static_cast<double>(Population));
	Estimators.setColumn(E_TIndex,E_T);
	Estimators.setColumn(AcceptIndex,
			     static_cast<double>(nAccept)/static_cast<double>(nAccept+nReject));
	Estimators.report(accstep);
	LogOut->getStream() << "Block " << block << " " << timer.cpu_time()
			    << " " << Population << endl;
	
	nAccept = 0; nReject = 0;
	block++;
	if(pStride) {
	  //create an output engine: could accumulate the configurations
	  HDFWalkerOutput WO(RootName);
	  WO.get(W);
	}
	W.reset();
      } while(block<nBlocks);
      
      LogOut->getStream() 
	<< "ratio = " << static_cast<double>(nAcceptTot)/static_cast<double>(nAcceptTot+nRejectTot)
	<< endl;
      
      if(!pStride) {
	//create an output engine: could accumulate the configurations
	HDFWalkerOutput WO(RootName);
	WO.get(W);
      }
      Estimators.finalize();
      return true;
    } else 
      return false;
  }

  bool 
  MolecuDMC::put(xmlNodePtr q){
    xmlNodePtr qsave=q;
    bool success = putQMCInfo(q);
    success = Estimators.put(qsave);
    return success;
  }
  
  /**  Advance all the walkers one timstep. 
   * 
   Propose a move for each walker from its old 
   position \f${\bf R'}\f$ to a new position \f${\bf R}\f$ 
   \f[ 
   {\bf R'} + {\bf \chi} + 
   \tau {\bf v_{drift}}({\bf R'}) =  {\bf R},
   \f]
   where \f$ {\bf \chi} \f$ is a 3N-diminsional 
   gaussian of mean zero and variance \f$ \tau \f$
   and \f$ {\bf v_{drift}} \f$ is the drift velocity
   \f[
   {\bf v_{drift}}({\bf R'}) = {\bf \nabla} 
   \ln |\Psi_T({\bf R'})| = \Psi_T({\bf R'})^{-1} 
   {\bf \nabla} \Psi_T({\bf R'}). 
   \f]
   For DMC it is necessary to check if the walker 
   crossed the nodal surface, if this is the case 
   then reject the move, otherwise Metropolis 
   accept/reject with probability
   \f[
   P_{accept}(\mathbf{R'}\rightarrow\mathbf{R}) = 
   \min\left[1,\frac{G(\mathbf{R}\rightarrow\mathbf{R'})
   \Psi_T(\mathbf{R})^2}{G(\mathbf{R'}\rightarrow\mathbf{R})
   \Psi_T(\mathbf{R'})^2}\right],
   \f] 
   where \f$ G \f$ is the drift-diffusion Green's function 
   \f[
   G(\mathbf{R'} \rightarrow 
   \mathbf{R}) = (2\pi\tau)^{-3/2}\exp \left[ -
   (\mathbf{R}-\mathbf{R'}-\tau \mathbf{v_{drift}}
   (\mathbf{R'}))^2/2\tau \right].
   \f]
   If the move is accepted, update the walker configuration and
   properties.  For rejected moves, do not update except for the
   Age which needs to be incremented by one.
   *
   Assign a weight and multiplicity for each walker
   \f[ weight = \exp \left[-\tau(E_L(\mathbf{R})+
   E_L(\mathbf{R})-2E_T)/2 \right]. \f]
   \f[ multiplicity = \exp \left[-\tau(E_L(\mathbf{R})+
   E_L(\mathbf{R})-2E_T)/2 \right] + \nu, \f]
   where \f$ \nu \f$ is a uniform random number.
   *
   Due to the fact that the drift velocity diverges on the nodal
   surface of the trial function \f$ \Psi_T \f$, it is possible
   for walkers close to the nodes to make excessively large proposed
   moves \f$ {\bf R'} \longrightarrow {\bf R} \f$.  With the
   accept/reject step this can lead to persistent configurations;
   a remedy is to impose a cutoff on the magnitude of the drift
   velocity.  We use the smooth cutoff proposed by Umrigar, 
   Nightingale and Runge
   [J. Chem. Phys., {\textbf 99}, 2865, (1993)]
   \f[
   {\bf \bar{v}_{drift}} = \frac{-1+\sqrt{1+2 \tau v^2_{drift}}}
   {\tau v^2_{drift}}{\bf v_{drift}},
   \f]
   where \f$ {\bf v_{drift}} \f$ is evaluated at 
   \f$ {\bf R'} \f$ and the magnitude of the drift
   \f$ \tau {\bf v_{drift}} \f$ is unchanged for small
   \f$ \tau v^2_{drift} \f$ and is limited to \f$ \sqrt{2\tau} \f$
   for large \f$ \tau v^2_{drift} \f$. 
   */
  template<class BRANCHER>
  void 
  MolecuDMC::advanceWalkerByWalker(BRANCHER& Branch) {
    
    //Pooma::Clock timer;
    RealType oneovertau = 1.0/Tau;
    RealType oneover2tau = 0.5*oneovertau;
    RealType g = sqrt(Tau);
    
    MCWalkerConfiguration::PropertyContainer_t Properties;
    static ParticleSet::ParticlePos_t deltaR(W.getTotalNum());
    static ParticleSet::ParticlePos_t drift(W.getTotalNum());
    int nh = H.size()+1;
    
    for (MCWalkerConfiguration::iterator it = W.begin();
	 it != W.end(); ++it) {
      
      (*it)->Properties(Weight) = 1.0;
      (*it)->Properties(Multiplicity) = 1.0;
      
      //copy the properties of the working walker
      Properties = (*it)->Properties;
      
      //save old local energy
      ValueType eold = Properties(LocalEnergy);
      ValueType emixed = eold;  
      
      //create a 3N-Dimensional Gaussian with variance=1
      makeGaussRandom(deltaR);
      
      W.R = g*deltaR + (*it)->R + (*it)->Drift;
      
      //update the distance table associated with W
      DistanceTable::update(W);
      
      //evaluate wave function
      ValueType psi = Psi.evaluate(W);
      //update the properties
      Properties(PsiSq) = psi*psi;
      Properties(Sign) = psi;
      Properties(LocalEnergy) = H.evaluate(W);
      
      //deltaR = W.R - (*it)->R - (*it)->Drift;
      //RealType forwardGF = exp(-oneover2tau*Dot(deltaR,deltaR));
      RealType forwardGF = exp(-0.5*Dot(deltaR,deltaR));
      
      //scale the drift term to prevent persistent cofigurations
      ValueType vsq = Dot(W.G,W.G);
      
      //converting gradients to drifts, D = tau*G (reuse G)
      //   W.G *= Tau;//original implementation with bare drift
      ValueType scale = ((-1.0+sqrt(1.0+2.0*Tau*vsq))/vsq);
      drift = scale*W.G;
      deltaR = (*it)->R - W.R - drift;
      RealType backwardGF = exp(-oneover2tau*Dot(deltaR,deltaR));
      
      //set acceptance probability
      RealType prob= min(backwardGF/forwardGF*Properties(PsiSq)/(*it)->Properties(PsiSq),1.0);
      
      bool accepted=false; 
      if(Random() > prob){
	(*it)->Properties(Age)++;
	emixed += eold;
      } else {
	accepted=true;  
	Properties(Age) = 0;
	(*it)->R = W.R;
	(*it)->Drift = drift;
	(*it)->Properties = Properties;
	H.get((*it)->E);
	emixed += Properties(LocalEnergy);
      }
      
      //calculate the weight and multiplicity
      ValueType M = Branch.branchGF(Tau,emixed*0.5,1.0-prob);
      if((*it)->Properties(Age) > 3.0) M = min(0.5,M);
      if((*it)->Properties(Age) > 0.9) M = min(1.0,M);
      (*it)->Properties(Weight) = M; 
      (*it)->Properties(Multiplicity) = M + Random();
      
      //node-crossing: kill it for the time being
      if(Branch(Properties(Sign),(*it)->Properties(Sign))) {
	accepted=false;     
	(*it)->Properties(Weight) = 0.0; 
	(*it)->Properties(Multiplicity) = 0.0;
      }
      
      if(accepted) 
	nAccept++;
      else 
	nReject++;
      
    }
  }

  /**  Advance all the walkers simultaneously. 
   * \param Branch a class to perform branching    
   */
  template<class BRANCHER>
  void MolecuDMC::advanceAllWalkers(BRANCHER& Branch) {
    //WARNING: This function hasn't been tested recently
    static ParticleSet::ParticlePos_t deltaR(W.getTotalNum());
    
    WalkerSetRef Wref(W);
    Wref.resize(W.getActiveWalkers(),W.getTotalNum());
    
    //Pooma::Clock timer;
    RealType oneovertau = 1.0/Tau;
    RealType oneover2tau = 0.5*oneovertau;
    RealType g = sqrt(Tau);
    
    MCWalkerConfiguration::PropertyContainer_t Properties;
    makeGaussRandom(Wref.R);
    
    Wref.R *= g;
    
    int nptcl = W.getTotalNum();
    int iw = 0;
    MCWalkerConfiguration::iterator it = W.begin();
    while(it !=  W.end()) {
      const ParticleSet::ParticlePos_t& r = (*it)->R;
      for(int jat=0; jat<nptcl; jat++) {
	Wref.R(iw,jat) += r(jat) + (*it)->Drift(jat);
      }
      iw++; it++;
    }
    
    //set acceptance probability
    RealType prob = 0.0;
    DistanceTable::update(Wref);
    
    OrbitalBase::ValueVectorType psi(iw), energy(iw);
    
    Psi.evaluate(Wref,psi);
    
    H.evaluate(Wref,energy);
    
    //multiply tau to convert gradient to drift term
    Wref.G *= Tau;
    
    iw = 0;
    it = W.begin();
    while(it !=  W.end()) {
      if(Branch(psi(iw),(*it)->Properties(Sign))) {
	++nReject;
      } else {
	
	ValueType eold = Properties(LocalEnergy);
	
	for(int iat=0; iat<nptcl; iat++)
	  deltaR(iat) = Wref.R(iw,iat) - (*it)->R(iat) - (*it)->Drift(iat);
	RealType forwardGF = exp(-oneover2tau*Dot(deltaR,deltaR));
	
	for(int iat=0; iat<nptcl; iat++)
	  deltaR(iat) = (*it)->R(iat) - Wref.R(iw,iat) - Wref.G(iw,iat);
	
	RealType backwardGF = exp(-oneover2tau*Dot(deltaR,deltaR));
	
	ValueType psisq = psi(iw)*psi(iw);
	
	prob = min(backwardGF/forwardGF*psisq/(*it)->Properties(PsiSq),1.0);
	if(Random() > prob) { 
	  ++nReject; 
	  (*it)->Properties(Age) += 1;
	} else {
	  (*it)->Properties(Age) = 0;
	  for(int iat=0; iat<nptcl; iat++) (*it)->R(iat) = Wref.R(iw,iat);
	  for(int iat=0; iat<nptcl; iat++) (*it)->Drift(iat) = Wref.G(iw,iat);
	  (*it)->Properties(Sign) = psi(iw);
	  (*it)->Properties(PsiSq) = psisq;
	  (*it)->Properties(LocalEnergy) = energy(iw);
	  ++nAccept;
	}
	
	RealType m = Branch.ratio(Tau,(*it)->Properties(LocalEnergy),eold,1.0-prob);
	(*it)->Properties(Multiplicity) *= m;
	(*it)->Properties(Weight) *= m;
      }
      iw++;it++;
    }
  }
}
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
