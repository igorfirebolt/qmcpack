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
using namespace std;
#include "Particle/MCWalkerConfiguration.h"
#include "Particle/DistanceTable.h"
#include "ParticleBase/ParticleUtility.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "Message/Communicate.h"
#include "QMCDrivers/WaveFunctionTester.h"
#include "QMCDrivers/DriftOperators.h"
#include "Utilities/OhmmsInform.h"
#include "LongRange/StructFact.h"
namespace qmcplusplus {


WaveFunctionTester::WaveFunctionTester(MCWalkerConfiguration& w, 
				       TrialWaveFunction& psi, 
				       QMCHamiltonian& h):
  QMCDriver(w,psi,h),checkRatio("no") { 
    m_param.add(checkRatio,"ratio","string");
  }


/*!
 * \brief Test the evaluation of the wavefunction, gradient and laplacian
 by comparing to the numerical evaluation.
 *
 Use the finite difference formulas formulas
 \f[ 
 \nabla_i f({\bf R}) = \frac{f({\bf R+\Delta r_i}) - f({\bf R})}{2\Delta r_i}
 \f]
 and
 \f[
 \nabla_i^2 f({\bf R}) = \sum_{x,y,z} \frac{f({\bf R}+\Delta x_i) 
 - 2 f({\bf R}) + f({\bf R}-\Delta x_i)}{2\Delta x_i^2},
 \f]
 where \f$ f = \ln \Psi \f$ and \f$ \Delta r_i \f$ is a 
 small displacement for the ith particle.
*/

bool 
WaveFunctionTester::run() {

  app_log() << "Starting a Wavefunction tester" << endl;

  DistanceTable::create(1);

  put(qmcNode);

  if(checkRatio == "yes") {
    runRatioTest();
  } else {
    runBasicTest();
  }

  RealType ene = H.evaluate(W);

  cout << " Energy " << ene << endl;

  return true;
}

void WaveFunctionTester::runBasicTest() {
  IndexType nskipped = 0;
  RealType sig2Enloc=0, sig2Drift=0;
  RealType delta = 0.0001;
  RealType delta2 = 2*delta;
  ValueType c1 = 1.0/delta/2.0;
  ValueType c2 = 1.0/delta/delta;

  int nat = W.getTotalNum();

  ParticleSet::ParticlePos_t deltaR(nat);
  MCWalkerConfiguration::PropertyContainer_t Properties;
  //pick the first walker
  MCWalkerConfiguration::Walker_t* awalker = *(W.begin());

  //copy the properties of the working walker
  Properties = awalker->Properties;
  
  //sample a new walker configuration and copy to ParticleSet::R
  //makeGaussRandom(deltaR);
 
  W.R = awalker->R;

  //W.R += deltaR;

  W.update();
  ValueType psi = Psi.evaluate(W);
  //RealType psi = Psi.evaluateLog(W);
  ParticleSet::ParticleGradient_t G(nat), G1(nat);
  ParticleSet::ParticleLaplacian_t L(nat), L1(nat);
  G = W.G;
  L = W.L;

#if defined(QMC_COMPLEX)
  ValueType logpsi(std::log(psi));
#else
  ValueType logpsi(std::log(std::abs(psi)));
#endif

  for(int iat=0; iat<nat; iat++) {
    PosType r0 = W.R[iat];
    GradType g0;  
    ValueType lap = 0.0;
    for(int idim=0; idim<3; idim++) {
   
      W.R[iat][idim] = r0[idim]+delta;         
      W.update();
      ValueType psi_p = Psi.evaluate(W);
      
      W.R[iat][idim] = r0[idim]-delta;         
      W.update();
      ValueType psi_m = Psi.evaluate(W);
#if defined(QMC_COMPLEX)
      lap += std::log(psi_m*psi_p);
      g0[idim] = std::log(psi_p/psi_m);
#else
      lap += std::log(std::abs(psi_m*psi_p));
      g0[idim] = std::log(std::abs(psi_p/psi_m));
#endif
      W.R[iat] = r0;
    }

    G1[iat] = c1*g0;
    L1[iat] = c2*(lap-6.0*logpsi);
  }
  
  cout.precision(15);
  for(int iat=0; iat<nat; iat++) {
    cout.precision(15);
    cout << "For particle #" << iat << " at " << W.R[iat] << endl;
    cout << "Gradient      = " << setw(12) << G[iat] << endl 
	 << "  Finite diff = " << setw(12) << G1[iat] << endl 
	 << "  Error       = " << setw(12) << G[iat]-G1[iat] << endl << endl;
    cout << "Laplacian     = " << setw(12) << L[iat] << endl 
	 << "  Finite diff = " << setw(12) << L1[iat] << endl 
	 << "  Error       = " << setw(12) << L[iat]-L1[iat] << endl << endl;
  }

  makeGaussRandom(deltaR);
  //testing ratio alone
  for(int iat=0; iat<nat; iat++) {
    W.update();
    //ValueType psi_p = log(fabs(Psi.evaluate(W)));
    RealType psi_p = Psi.evaluateLog(W);
    RealType phase_p=Psi.getPhase();

    W.makeMove(iat,deltaR[iat]);
    RealType aratio = Psi.ratio(W,iat);
    W.rejectMove(iat);
    Psi.rejectMove(iat);

    W.R[iat] += deltaR[iat];         
    W.update();
    //ValueType psi_m = log(fabs(Psi.evaluate(W)));
    RealType psi_m = Psi.evaluateLog(W);
    RealType phase_m=Psi.getPhase();

    RealType ratDiff=std::exp(psi_m-psi_p)*std::cos(phase_m-phase_p) ;
    cout << iat << " ratio " << aratio/ratDiff << " " << ratDiff << endl;
  }
} 

void WaveFunctionTester::runRatioTest() {

  int nat = W.getTotalNum();
  ParticleSet::ParticleGradient_t G(nat), dG(nat);
  ParticleSet::ParticleLaplacian_t L(nat), dL(nat);

  MCWalkerConfiguration::iterator it(W.begin()), it_end(W.end());
  while(it != it_end) {
    (*it)->DataSet.rewind();
    W.registerData(**it,(*it)->DataSet);
    RealType logpsi=Psi.registerData(W,(*it)->DataSet);
    //RealType vsq = Dot(W.G,W.G);
    //(*it)->Drift = Tau*W.G;
    setScaledDrift(Tau,W.G,(*it)->Drift);

    RealType ene = H.evaluate(W);
    (*it)->resetProperty(logpsi,Psi.getPhase(),ene);
    H.saveProperty((*it)->getPropertyBase());
    ++it;
  } 

  it=W.begin();
  int iw=0;
  while(it != it_end) {

    cout << "\nStart Walker " << iw++ << endl;

    Walker_t& thisWalker(**it);
    Walker_t::Buffer_t& w_buffer(thisWalker.DataSet);

    w_buffer.rewind();
    W.copyFromBuffer(w_buffer);
    Psi.copyFromBuffer(W,w_buffer);

    RealType eold(thisWalker.Properties(LOCALENERGY));
    RealType logpsi(thisWalker.Properties(LOGPSI));
    RealType emixed(eold), enew(eold);

    makeGaussRandom(deltaR);

    //mave a move
    RealType ratio_accum(1.0);
    for(int iat=0; iat<nat; iat++) {
      PosType dr(Tau*deltaR[iat]);

      PosType newpos(W.makeMove(iat,dr));

      RealType ratio=Psi.ratio(W,iat,dG,dL);

      //if(ratio > 0 && iat%2 == 1) {//accept only the even index
      if(ratio > Random()) {
        cout << " Accepting a move for " << iat << endl;
        W.acceptMove(iat);
        Psi.acceptMove(W,iat);

        W.G += dG;
        W.L += dL;
        //G=W.G+dG;
        //L=W.L+dL;
        ratio_accum *= ratio;
      } else {
        cout << " Rejecting a move for " << iat << endl;
        W.rejectMove(iat); 
        Psi.rejectMove(iat);
      }
    }


    G = W.G;
    L = W.L;

    w_buffer.rewind();
    W.copyToBuffer(w_buffer);
    RealType psi = Psi.evaluate(W,w_buffer);

    W.update();
    RealType newlogpsi=Psi.evaluateLog(W);

    cout << " Ratio " << ratio_accum*ratio_accum 
      << " | " << std::exp(2.0*(newlogpsi-logpsi)) << " " 
      << ratio_accum*ratio_accum/std::exp(2.0*(newlogpsi-logpsi)) << endl
      << " new log(psi) " << newlogpsi 
      << " old log(psi) " << logpsi << endl;

    cout << " Gradients " << endl;
    for(int iat=0; iat<nat; iat++) {
      cout << W.G[iat]-G[iat] << W.G[iat] << endl; //W.G[iat] << G[iat] << endl;
    }
    cout << " Laplacians " << endl;
    for(int iat=0; iat<nat; iat++) {
      cout << W.L[iat]-L[iat] << " " << W.L[iat] << endl;
    }
    ++it;
  }
}

bool 
WaveFunctionTester::put(xmlNodePtr q){
  return putQMCInfo(q);
}
}

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
