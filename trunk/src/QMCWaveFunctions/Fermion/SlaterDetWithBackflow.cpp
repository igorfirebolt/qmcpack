//////////////////////////////////////////////////////////////////
// (c) Copyright 2003  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
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
#include "QMCWaveFunctions/Fermion/SlaterDetWithBackflow.h"
#include "QMCWaveFunctions/Fermion/BackflowTransformation.h"
#include "QMCWaveFunctions/Fermion/RNDiracDeterminantBase.h"
#include "QMCWaveFunctions/Fermion/RNDiracDeterminantBaseAlternate.h"
#include "Message/Communicate.h"
#include "Utilities/OhmmsInfo.h"

namespace qmcplusplus {

  SlaterDetWithBackflow::SlaterDetWithBackflow(ParticleSet& targetPtcl, BackflowTransformation *BF):SlaterDet(targetPtcl),BFTrans(BF)
  {
    Optimizable=true;
    OrbitalName="SlaterDetWithBackflow";
  }

  ///destructor
  SlaterDetWithBackflow::~SlaterDetWithBackflow() 
  { 
    ///clean up SPOSet
  }

  void SlaterDetWithBackflow::get_ratios(ParticleSet& P, vector<ValueType>& ratios)
  {
    for(int i=0; i<Dets.size(); ++i) Dets[i]->get_ratios(P,ratios);
  }


  SlaterDetWithBackflow::ValueType 
    SlaterDetWithBackflow::evaluate(ParticleSet& P, 
        ParticleSet::ParticleGradient_t& G, 
        ParticleSet::ParticleLaplacian_t& L) 
    {
      BFTrans->evaluate(P);

      ValueType psi = 1.0;
      for(int i=0; i<Dets.size(); i++) psi *= Dets[i]->evaluate(P,G,L);
      return psi;
    }

  SlaterDetWithBackflow::RealType 
    SlaterDetWithBackflow::evaluateLog(ParticleSet& P, 
        ParticleSet::ParticleGradient_t& G, 
        ParticleSet::ParticleLaplacian_t& L) 
    {
      BFTrans->evaluate(P);

      LogValue=0.0;
      PhaseValue=0.0;
      for(int i=0; i<Dets.size(); ++i)
      {
        LogValue+=Dets[i]->evaluateLog(P,G,L);
        PhaseValue += Dets[i]->PhaseValue;
      }
      return LogValue;
    }

  SlaterDetWithBackflow::RealType SlaterDetWithBackflow::registerData(ParticleSet& P, PooledData<RealType>& buf)
  {
    BFTrans->evaluate(P);

    LogValue=0.0;
    PhaseValue=0.0;
    for(int i=0; i<Dets.size(); ++i)
    {
      LogValue+=Dets[i]->registerData(P,buf);
      PhaseValue += Dets[i]->PhaseValue;
    }
    return LogValue;
  }

  SlaterDetWithBackflow::RealType SlaterDetWithBackflow::updateBuffer(ParticleSet& P, PooledData<RealType>& buf,
      bool fromscratch)
  {
    BFTrans->evaluate(P);

    LogValue=0.0;
    PhaseValue=0.0;
    for(int i=0; i<Dets.size(); ++i)
    {
      LogValue+=Dets[i]->updateBuffer(P,buf,fromscratch);
      PhaseValue+=Dets[i]->PhaseValue;
    }
    return LogValue;
  }

  void SlaterDetWithBackflow::copyFromBuffer(ParticleSet& P, PooledData<RealType>& buf) 
  {
    for(int i=0; i<Dets.size(); i++) 	Dets[i]->copyFromBuffer(P,buf);
  }

  void SlaterDetWithBackflow::dumpToBuffer(ParticleSet& P, PooledData<RealType>& buf) 
  {
    for(int i=0; i<Dets.size(); i++) 	Dets[i]->dumpToBuffer(P,buf);
  }

  void SlaterDetWithBackflow::dumpFromBuffer(ParticleSet& P, PooledData<RealType>& buf) 
  {
    for(int i=0; i<Dets.size(); i++) 	Dets[i]->dumpFromBuffer(P,buf);
  }

  SlaterDetWithBackflow::RealType 
    SlaterDetWithBackflow::evaluateLog(ParticleSet& P, PooledData<RealType>& buf) 
    {
//      BFTrans->evaluate(P);

      LogValue=0.0;
      PhaseValue=0.0;
      for(int i=0; i<Dets.size(); i++) 	
      { 
        LogValue += Dets[i]->evaluateLog(P,buf);
        PhaseValue +=Dets[i]->PhaseValue;
      }
      return LogValue;
    }
  //SlaterDetWithBackflow::ValueType 
  //  SlaterDetWithBackflow::evaluate(ParticleSet& P, PooledData<RealType>& buf) 
  //  {
  //    ValueType r=1.0;
  //    for(int i=0; i<Dets.size(); i++) 	r *= Dets[i]->evaluate(P,buf);
  //    return r;
  //  }

  OrbitalBasePtr SlaterDetWithBackflow::makeClone(ParticleSet& tqp) const
  {
    BackflowTransformation *tr = BFTrans->makeClone();
    SlaterDetWithBackflow* myclone=new SlaterDetWithBackflow(tqp,tr);
    if(mySPOSet.size()>1)//each determinant owns its own set
    {
      for(int i=0; i<Dets.size(); ++i)
      {
        SPOSetBasePtr spo=Dets[i]->getPhi();
	// Check to see if this determinants SPOSet has already been
	// cloned
	bool found = false;
        SPOSetBasePtr spo_clone;
	for (int j=0; j<i; j++)
	  if (spo == Dets[j]->getPhi()) {
	    found = true;
	    spo_clone = myclone->Dets[j]->getPhi();
	  }
	// If it hasn't, clone it now
	if (!found) {
	  spo_clone=spo->makeClone();
	  myclone->add(spo_clone,spo->objectName);
	}
	// Make a copy of the determinant.
        myclone->add(Dets[i]->makeCopy(spo_clone),i);
      }
    }
    else
    {
      SPOSetBasePtr spo=Dets[0]->getPhi();
      SPOSetBasePtr spo_clone=spo->makeClone();
      myclone->add(spo_clone,spo->objectName);
      for(int i=0; i<Dets.size(); ++i)
        myclone->add(Dets[i]->makeCopy(spo_clone),i);
    }

    return myclone;
  }

  void SlaterDetWithBackflow::testDerivGL(ParticleSet& P)
  {

// testing derivatives of G and L
         
       app_log() <<"testing derivatives of G and L \n";
       opt_variables_type wfVars,wfvar_prime;
       checkInVariables(wfVars);
       checkOutVariables(wfVars);
       int Nvars= wfVars.size();
       wfvar_prime= wfVars;
       wfVars.print(cout);
       vector<RealType> dlogpsi;
       vector<RealType> dhpsi;
       dlogpsi.resize(Nvars);
       dhpsi.resize(Nvars);
       ParticleSet::ParticleGradient_t G0,G1,G2;
       ParticleSet::ParticleLaplacian_t L0,L1,L2;
       G0.resize(P.getTotalNum());
       G1.resize(P.getTotalNum());
       G2.resize(P.getTotalNum());
       L0.resize(P.getTotalNum());
       L1.resize(P.getTotalNum());
       L2.resize(P.getTotalNum());
       ValueType psi0 = 1.0;
       ValueType psi1 = 1.0;
       ValueType psi2 = 1.0;
       double dh=0.00001;

         for(int k=0; k<Dets.size(); k++) {
           DiracDeterminantWithBackflow* Dets_ = (DiracDeterminantWithBackflow*) Dets[k];
           Dets_->testGGG(P);
           for( int i=0; i<Nvars; i++) {
             Dets_->testDerivFjj(P,i);
             Dets_->testDerivLi(P,i);
           }
         }


       app_log() <<"Nvars: " <<Nvars <<endl;
       for(int i=0; i<Nvars; i++) {

         for (int j=0; j<Nvars; j++) wfvar_prime[j]=wfVars[j];
         resetParameters(wfvar_prime);        
         BFTrans->evaluateDerivatives(P);
         G0=0.0;G1=0.0;G2=0.0;
         L0=0.0;L1=0.0;L2=0.0;
         for(int k=0; k<Dets.size(); k++) {
           DiracDeterminantWithBackflow* Dets_ = (DiracDeterminantWithBackflow*) Dets[k];
           Dets_->evaluateDerivatives(P,wfVars,dlogpsi,dhpsi,&G0,&L0,i);
         }
         

         for (int j=0; j<Nvars; j++) wfvar_prime[j]=wfVars[j];
         wfvar_prime[i] = wfVars[i]+ dh;
         resetParameters(wfvar_prime);

         BFTrans->evaluate(P);
         for(int k=0; k<Dets.size(); k++) psi1 += Dets[k]->evaluateLog(P,G1,L1);

         for (int j=0; j<Nvars; j++) wfvar_prime[j]=wfVars[j];
         wfvar_prime[i] = wfVars[i]- dh;
         resetParameters(wfvar_prime);

         BFTrans->evaluate(P);
         for(int k=0; k<Dets.size(); k++) psi2 += Dets[k]->evaluateLog(P,G2,L2);

         ValueType tmp=0.0;
         for(int q=0; q<P.getTotalNum(); q++) tmp+=(L1[q]-L2[q])/(2*dh);
         app_log() <<i <<"\n"
                     <<"Ldiff : " <<L0[0] <<"  " <<tmp
                     <<"  " <<L0[0]-tmp <<endl;

         for(int k=0; k<P.getTotalNum(); k++) {
           app_log()<<G0[k] <<endl
                     <<(G1[k]-G2[k])/(2*dh) <<endl
                     <<"Gdiff: " <<G0[k]-(G1[k]-G2[k])/(2*dh) <<endl <<endl;
         }

       }
       resetParameters(wfVars);        
      //APP_ABORT("Testing bF derivs \n");
  }


  void SlaterDetWithBackflow::evaluateDerivatives(ParticleSet& P,
                                     const opt_variables_type& optvars,
                                     vector<RealType>& dlogpsi,
                                     vector<RealType>& dhpsioverpsi)
  {
      //testDerivGL(P);

      // build QP,Amat,Bmat_full,Xmat,Cmat,Ymat
      BFTrans->evaluateDerivatives(P);

      ValueType psi = 1.0;
      for(int i=0; i<Dets.size(); i++) Dets[i]->evaluateDerivatives(P,optvars,dlogpsi,dhpsioverpsi);

  }


}
/***************************************************************************
 * $RCSfile$   $Author: kpesler $
 * $Revision: 4721 $   $Date: 2010-03-12 17:11:47 -0600 (Fri, 12 Mar 2010) $
 * $Id: SlaterDetWithBackflow.cpp 4721 2010-03-12 23:11:47Z kpesler $ 
 ***************************************************************************/
