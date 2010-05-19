//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
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
#include "QMCWaveFunctions/MultiSlaterDeterminant.h"
#include "ParticleBase/ParticleAttribOps.h"

namespace qmcplusplus {

  MultiSlaterDeterminant::MultiSlaterDeterminant(ParticleSet& targetPtcl, SPOSetProxyPtr upspo, SPOSetProxyPtr dnspo):spo_up(upspo),spo_dn(dnspo) 
  { 
    //Optimizable=true;
    Optimizable=false;
    OrbitalName="MultiSlaterDeterminant";

    FirstIndex_up = targetPtcl.first(0); 
    LastIndex_up = targetPtcl.last(0);
    FirstIndex_dn = targetPtcl.first(1); 
    LastIndex_dn = targetPtcl.last(1);

    nels_up = LastIndex_up-FirstIndex_up;
    nels_dn = LastIndex_dn-FirstIndex_dn;

    DetID.resize(targetPtcl.getTotalNum());
    for(int i=0; i<targetPtcl.groups(); ++i)
      for(int j=targetPtcl.first(i); j<targetPtcl.last(i); ++j) DetID[j]=i;    
  }
  
  OrbitalBasePtr MultiSlaterDeterminant::makeClone(ParticleSet& tqp) const
  { 
    SPOSetProxyForMSD* spo_up_C = new SPOSetProxyForMSD(spo_up->refPhi->makeClone(),FirstIndex_up,LastIndex_up);
    SPOSetProxyForMSD* spo_dn_C = new SPOSetProxyForMSD(spo_dn->refPhi->makeClone(),FirstIndex_dn,LastIndex_dn);
    spo_up_C->occup= spo_up->occup;
    spo_dn_C->occup= spo_dn->occup;
    spo_up_C->refPhi->resetTargetParticleSet(tqp);
    spo_dn_C->refPhi->resetTargetParticleSet(tqp);
    
    MultiSlaterDeterminant* clone = new MultiSlaterDeterminant(tqp,spo_up_C,spo_dn_C); 
    clone->C2node_up=C2node_up;
    clone->C2node_dn=C2node_dn;
    clone->resize(dets_up.size(),dets_dn.size());
    
//     SPOSetProxyForMSD* spo = clone->spo_up;
//     spo->occup.resize(uniqueConfg_up.size(),clone->nels_up);
for(int i=0; i<dets_up.size(); i++)
    {
//       int nq=0;
// //       configuration& ci = uniqueConfg_up[i];
//       for(int k=0; k<uniqueConfg_up[i].occup.size(); k++) {
//         if(uniqueConfg_up[i].occup[k]) { 
//           spo->occup(i,nq++) = k;
//         }
//       }
DiracDeterminantBase* adet = new DiracDeterminantBase((SPOSetBasePtr) clone->spo_up,0);
      adet->set(clone->FirstIndex_up,clone->nels_up);
      clone->dets_up.push_back(adet);
    }
//     spo = clone->spo_dn;
//     spo->occup.resize(uniqueConfg_dn.size(),clone->nels_dn);
for(int i=0; i<dets_dn.size(); i++)
    {
//       int nq=0;
// //       configuration& ci = uniqueConfg_dn[i];
//       for(int k=0; k<uniqueConfg_dn[i].occup.size(); k++) {
//         if(uniqueConfg_dn[i].occup[k]) {
//           spo->occup(i,nq++) = k;
//         }
//       }
DiracDeterminantBase* adet = new DiracDeterminantBase((SPOSetBasePtr) clone->spo_dn,0);
      adet->set(clone->FirstIndex_dn,clone->nels_dn);
      clone->dets_dn.push_back(adet);
    }

    clone->Optimizable=Optimizable;
    clone->C=C;
    clone->myVars=myVars;
    
    return clone;
  }
  

  MultiSlaterDeterminant::~MultiSlaterDeterminant() { }
  void MultiSlaterDeterminant::resetTargetParticleSet(ParticleSet& P) 
  {
    for(int i=0; i<dets_up.size(); i++)
      dets_up[i]->resetTargetParticleSet(P);
    for(int i=0; i<dets_dn.size(); i++)
      dets_dn[i]->resetTargetParticleSet(P);
  }

  void MultiSlaterDeterminant::resize(int n1, int n2)
  {
    NumUniqueDets_up=n1;
    NumUniqueDets_dn=n2;
    myG.resize(nels_up+nels_dn);
    myL.resize(nels_up+nels_dn);
    
    grads_up.resize(NumUniqueDets_up);
    grads_dn.resize(NumUniqueDets_dn);
    lapls_up.resize(NumUniqueDets_up);
    lapls_dn.resize(NumUniqueDets_dn);
    for(int i=0; i<NumUniqueDets_up; i++) 
    {
      grads_up[i].resize(nels_up+nels_dn);
      lapls_up[i].resize(nels_up+nels_dn);
    }
    for(int i=0; i<NumUniqueDets_dn; i++)
    {
      grads_dn[i].resize(nels_up+nels_dn);
      lapls_dn[i].resize(nels_up+nels_dn);
    }

    detValues_up.resize(NumUniqueDets_up);
    detValues_dn.resize(NumUniqueDets_dn);

    int maxDet = NumUniqueDets_up;
    if(NumUniqueDets_dn > NumUniqueDets_up) maxDet=NumUniqueDets_dn;
    detsRatios.resize(maxDet);
    lapl_temp.resize(maxDet);
    grad_temp.resize(maxDet);
    tempgrad.resize(maxDet);
    templapl.resize(maxDet);
    for(int i=0; i<maxDet; i++) {
      tempgrad[i].resize(nels_up+nels_dn);
      templapl[i].resize(nels_up+nels_dn);
    }
  }

  OrbitalBase::ValueType MultiSlaterDeterminant::evaluate(ParticleSet& P
      , ParticleSet::ParticleGradient_t& G, ParticleSet::ParticleLaplacian_t& L)
  { 
// mmorales: For now always assume 2 electron components, up/down

    spo_up->evaluateForWalkerMove(P,FirstIndex_up,LastIndex_up);
    spo_dn->evaluateForWalkerMove(P,FirstIndex_dn,LastIndex_dn);

    for(int i=0; i<dets_up.size(); i++) {
      spo_up->prepareFor(i);
      grads_up[i]=0.0; 
      lapls_up[i]=0.0; 
      detValues_up[i]=dets_up[i]->evaluate(P,grads_up[i],lapls_up[i]);
      // need \nabla^2 Det / Det
      for(int k=FirstIndex_up; k<LastIndex_up; k++) 
        lapls_up[i][k] += dot(grads_up[i][k],grads_up[i][k]);
    }
    for(int i=0; i<dets_dn.size(); i++) {
      spo_dn->prepareFor(i);
      grads_dn[i]=0.0; 
      lapls_dn[i]=0.0; 
      detValues_dn[i]=dets_dn[i]->evaluate(P,grads_dn[i],lapls_dn[i]);
      // need \nabla^2 Det / Det
      for(int k=FirstIndex_dn; k<LastIndex_dn; k++) 
        lapls_dn[i][k] += dot(grads_dn[i][k],grads_dn[i][k]);
    }

    ValueType psi=0.0;
    myG=0.0;
    myL=0.0;
    for(int i=0; i<C.size(); i++){
      int upC = C2node_up[i];
      int dnC = C2node_dn[i];
      ValueType tmp = C[i]*detValues_up[upC]*detValues_dn[dnC];
      psi += tmp;
      //for(int n=FirstIndex_up; n<LastIndex_up; n++) {
        myG += tmp*grads_up[upC]; // other spin sector should be zero 
        myL += tmp*lapls_up[upC]; 
      //}
      //for(int n=FirstIndex_dn; n<LastIndex_dn; n++) {
        myG += tmp*grads_dn[dnC]; // other spin sector should be zero 
        myL += tmp*lapls_dn[dnC]; 
      //}
    }
    ValueType psiinv = 1.0/psi;
    myG *= psiinv;
    myL *= psiinv;
    G += myG;
    for(int i=0; i<L.size(); i++) 
      L(i) += myL[i] - dot(myG[i],myG[i]);
    return psi;
  }

  OrbitalBase::RealType MultiSlaterDeterminant::evaluateLog(ParticleSet& P
      , ParticleSet::ParticleGradient_t& G, ParticleSet::ParticleLaplacian_t& L)
  {
    ValueType psi = evaluate(P,G,L);
    return LogValue = evaluateLogAndPhase(psi,PhaseValue);
  }

  OrbitalBase::GradType MultiSlaterDeterminant::evalGrad(ParticleSet& P, int iat)
  {
    GradType grad_iat=0.0;
    if(DetID[iat] == 0) {
      ValueType psi=0.0;
      for(int i=0; i<C.size(); i++){
        int upC = C2node_up[i];
        int dnC = C2node_dn[i];
        ValueType tmp = C[i]*detValues_up[upC]*detValues_dn[dnC];
        psi += tmp;
        grad_iat += tmp*grads_up[upC](iat);
      }
      grad_iat *= 1.0/psi;
      return grad_iat;
    } else {
      ValueType psi=0.0;
      for(int i=0; i<C.size(); i++){ 
        int upC = C2node_up[i];
        int dnC = C2node_dn[i];
        ValueType tmp = C[i]*detValues_up[upC]*detValues_dn[dnC];
        psi += tmp;
        grad_iat += tmp*grads_dn[dnC](iat);
      }
      grad_iat *= 1.0/psi;
      return grad_iat;
    }
  }

  OrbitalBase::ValueType MultiSlaterDeterminant::ratioGrad(ParticleSet& P
      , int iat, GradType& grad_iat)
  {
    UpdateMode=ORB_PBYP_PARTIAL;
    if(DetID[iat] == 0) {
      spo_up->evaluateAllForPtclMove(P,iat);

      grad_temp=0.0; 
      for(int i=0; i<dets_up.size(); i++) {
        spo_up->prepareFor(i);
        detsRatios[i]=dets_up[i]->ratioGrad(P,iat,grad_temp[i]);
      }

      ValueType psiOld=0.0,psiNew=0.0;
      GradType dummy=0.0; 
      for(int i=0; i<C.size(); i++){
        int upC = C2node_up[i];
        int dnC = C2node_dn[i];
        ValueType tmp2 = C[i]*detValues_up[upC]*detValues_dn[dnC];
        ValueType tmp = tmp2*detsRatios[upC];
        psiOld += tmp2;
        psiNew += tmp;
        dummy += tmp*grad_temp[upC]; 
      }
      grad_iat+=dummy/psiNew;
      curRatio = psiNew/psiOld;
      return curRatio; 
    } else {
      spo_dn->evaluateAllForPtclMove(P,iat);

      grad_temp=0.0;
      for(int i=0; i<dets_dn.size(); i++) {
        spo_dn->prepareFor(i);
        detsRatios[i]=dets_dn[i]->ratioGrad(P,iat,grad_temp[i]);
      }

      ValueType psiOld=0.0,psiNew=0.0;
      GradType dummy=0.0;
      for(int i=0; i<C.size(); i++){
        int upC = C2node_up[i];
        int dnC = C2node_dn[i];
        ValueType tmp2 = C[i]*detValues_up[upC]*detValues_dn[dnC];
        ValueType tmp = tmp2*detsRatios[dnC];
        psiOld += tmp2;
        psiNew += tmp;
        dummy += tmp*grad_temp[dnC];
      }
      grad_iat+=dummy/psiNew;
      curRatio = psiNew/psiOld;
      return curRatio; 
    }
  }


// This routine need work, sloppy for now
  OrbitalBase::ValueType  MultiSlaterDeterminant::ratio(ParticleSet& P, int iat
     , ParticleSet::ParticleGradient_t& dG,ParticleSet::ParticleLaplacian_t& dL)
  {
    UpdateMode=ORB_PBYP_ALL;
    if(DetID[iat] == 0) {
      spo_up->evaluateAllForPtclMove(P,iat);

      for(int i=0; i<dets_up.size(); i++) {
        spo_up->prepareFor(i);
        ParticleSet::ParticleLaplacian_t& li = templapl[i];
        ParticleSet::ParticleGradient_t& gi = tempgrad[i];
        li=0.0;
        gi=0.0;
// HACK HACK HACK, I don't differential properties at this point
        dets_up[i]->myG=0.0;
        dets_up[i]->myL=0.0;
        detsRatios[i]=dets_up[i]->ratio(P,iat,gi,li);
        for(int k=FirstIndex_up; k<LastIndex_up; k++)
          li[k] += dot(gi[k],gi[k]);
      }

      ValueType psiNew=0.0,psiOld=0.0;
      myG=0.0;
      myL=0.0;
      ParticleSet::ParticleLaplacian_t lold(nels_up+nels_dn);
      ParticleSet::ParticleGradient_t gold(nels_up+nels_dn);
      for(int i=0; i<C.size(); i++){
        int upC = C2node_up[i];
        int dnC = C2node_dn[i];
        ValueType tmp = C[i]*detValues_up[upC]*detValues_dn[dnC];
        ValueType tmp2 = tmp*detsRatios[upC]; 
        psiNew += tmp2;
        psiOld += tmp;
        //for(int n=FirstIndex_up; n<LastIndex_up; n++) {
          myG += tmp2*tempgrad[upC]; // other spin sector should be zero 
          myL += tmp2*templapl[upC];
        //}
        //for(int n=FirstIndex_up; n<LastIndex_up; n++) {
          gold += tmp*grads_up[upC]; // other spin sector should be zero 
          lold += tmp*lapls_up[upC];
        //}
      }

      ValueType psiNinv=1.0/psiNew;
      ValueType psiOinv=1.0/psiOld;
      myG *= psiNinv;
      gold *= psiOinv;
      myL *= psiNinv;
      lold *= psiOinv;
      dG += myG-gold;
      for(int i=0; i<dL.size(); i++) 
        dL(i) += myL[i] - lold[i] - dot(myG[i],myG[i]) + dot(gold[i],gold[i]);
      curRatio = psiNew/psiOld;
      return curRatio; 
    } else {
      spo_dn->evaluateAllForPtclMove(P,iat);

      for(int i=0; i<dets_dn.size(); i++) {
        spo_dn->prepareFor(i);
        ParticleSet::ParticleLaplacian_t& li = templapl[i];
        ParticleSet::ParticleGradient_t& gi = tempgrad[i];
        li=0.0;
        gi=0.0;
// HACK HACK HACK, I don't differential properties at this point
        dets_dn[i]->myG=0.0;
        dets_dn[i]->myL=0.0;
        detsRatios[i]=dets_dn[i]->ratio(P,iat,gi,li);
        for(int k=FirstIndex_dn; k<LastIndex_dn; k++)
          li[k] += dot(gi[k],gi[k]);
      }

      ValueType psiNew=0.0,psiOld=0.0;
      myG=0.0;
      myL=0.0;
      ParticleSet::ParticleLaplacian_t lold(nels_up+nels_dn);
      ParticleSet::ParticleGradient_t gold(nels_up+nels_dn);
      for(int i=0; i<C.size(); i++){
        int upC = C2node_up[i];
        int dnC = C2node_dn[i];
        ValueType tmp = C[i]*detValues_up[upC]*detValues_dn[dnC];
        ValueType tmp2 = tmp*detsRatios[dnC];
        psiNew += tmp2;
        psiOld += tmp;
        //for(int n=FirstIndex_up; n<LastIndex_up; n++) {
          myG += tmp2*tempgrad[dnC]; // other spin sector should be zero 
          myL += tmp2*templapl[dnC];
        //}
        //for(int n=FirstIndex_up; n<LastIndex_up; n++) {
          gold += tmp*grads_dn[dnC]; // other spin sector should be zero 
          lold += tmp*lapls_dn[dnC];
        //}
      }

      ValueType psiNinv=1.0/psiNew;
      ValueType psiOinv=1.0/psiOld;
      myG *= psiNinv;
      gold *= psiOinv;
      myL *= psiNinv;
      lold *= psiOinv;
      dG += myG-gold;
      for(int i=0; i<dL.size(); i++) 
        dL(i) += myL[i] - lold[i] - dot(myG[i],myG[i]) + dot(gold[i],gold[i]);
      curRatio = psiNew/psiOld;
      return curRatio; 
    }
  }

// use ci_node for this routine only
  OrbitalBase::ValueType MultiSlaterDeterminant::ratio(ParticleSet& P, int iat)
  {
    UpdateMode=ORB_PBYP_RATIO;
    if(DetID[iat] == 0) {
      spo_up->evaluateForPtclMove(P,iat);

      for(int i=0; i<dets_up.size(); i++) {
        spo_up->prepareFor(i);
        detsRatios[i]=dets_up[i]->ratio(P,iat);
      }

      ValueType psiOld=0.0,psiNew=0.0;
      for(int i=0; i<C.size(); i++){
        int upC = C2node_up[i];
        int dnC = C2node_dn[i];
        ValueType tmp2 = C[i]*detValues_up[upC]*detValues_dn[dnC];
        psiOld += tmp2;
        psiNew += tmp2*detsRatios[upC];
      }
      curRatio = psiNew/psiOld;
      return curRatio; 
    } else {
      spo_dn->evaluateForPtclMove(P,iat);

      for(int i=0; i<dets_dn.size(); i++) {
        spo_dn->prepareFor(i);
        detsRatios[i]=dets_dn[i]->ratio(P,iat);
      }

      ValueType psiOld=0.0,psiNew=0.0;
      for(int i=0; i<C.size(); i++){
        int upC = C2node_up[i];
        int dnC = C2node_dn[i];
        ValueType tmp2 = C[i]*detValues_up[upC]*detValues_dn[dnC];
        psiOld += tmp2;
        psiNew += tmp2*detsRatios[dnC];
      }
      curRatio = psiNew/psiOld;
      return curRatio; 
    }
  }

  void MultiSlaterDeterminant::acceptMove(ParticleSet& P, int iat)
  {
// this should depend on the type of update, ratio / ratioGrad 
// for now is incorrect fot ratio(P,iat,dG,dL) updates 
    if(DetID[iat] == 0) {
      for(int i=0; i<dets_up.size(); i++) 
        dets_up[i]->acceptMove(P,iat); 
      switch(UpdateMode) 
      {
        case ORB_PBYP_RATIO:
          // ratio(P,iat)
          for(int i=0; i<detValues_up.size(); i++)
            detValues_up[i] *= detsRatios[i];
          PhaseValue += evaluatePhase(curRatio);
          LogValue +=std::log(std::abs(curRatio));
          curRatio=1.0;
          break;
        case ORB_PBYP_PARTIAL:
          // ratioGrad(P,iat,grad)
          for(int i=0; i<detValues_up.size(); i++){
            detValues_up[i] *= detsRatios[i];
            grads_up[i][iat] = grad_temp[i];
          }
          PhaseValue += evaluatePhase(curRatio);
          LogValue +=std::log(std::abs(curRatio));
          curRatio=1.0;
          break;
        case ORB_PBYP_ALL:
          // ratio(P,iat,dG,dL)
          for(int i=0; i<detValues_up.size(); i++){
            detValues_up[i] *= detsRatios[i];
            grads_up[i] = tempgrad[i];
            lapls_up[i] = templapl[i];
          }
          PhaseValue += evaluatePhase(curRatio);
          LogValue +=std::log(std::abs(curRatio));
          curRatio=1.0;
          break;
        default: 
          for(int i=0; i<detValues_up.size(); i++)
            detValues_up[i] *= detsRatios[i];
          PhaseValue += evaluatePhase(curRatio);
          LogValue +=std::log(std::abs(curRatio));
          curRatio=1.0;
          break;
      }
    } else {
      for(int i=0; i<dets_dn.size(); i++) 
        dets_dn[i]->acceptMove(P,iat); 
      switch(UpdateMode)
      {
        case ORB_PBYP_RATIO:
          // ratio(P,iat)
          for(int i=0; i<detValues_dn.size(); i++)
            detValues_dn[i] *= detsRatios[i];
          PhaseValue += evaluatePhase(curRatio);
          LogValue +=std::log(std::abs(curRatio));
          curRatio=1.0;
          break;
        case ORB_PBYP_PARTIAL:
          // ratioGrad(P,iat,grad)
          for(int i=0; i<detValues_dn.size(); i++){
            detValues_dn[i] *= detsRatios[i];
            grads_dn[i][iat] = grad_temp[i];
          }
          PhaseValue += evaluatePhase(curRatio);
          LogValue +=std::log(std::abs(curRatio));
          curRatio=1.0;
          break;
        case ORB_PBYP_ALL:
          // ratio(P,iat,dG,dL)
          for(int i=0; i<detValues_dn.size(); i++){
            detValues_dn[i] *= detsRatios[i];
            grads_dn[i] = tempgrad[i];
            lapls_dn[i] = templapl[i];
          }
          PhaseValue += evaluatePhase(curRatio);
          LogValue +=std::log(std::abs(curRatio));
          curRatio=1.0;
          break;
        default:
          for(int i=0; i<detValues_dn.size(); i++)
            detValues_dn[i] *= detsRatios[i];
          PhaseValue += evaluatePhase(curRatio);
          LogValue +=std::log(std::abs(curRatio));
          curRatio=1.0;
          break;
      }
    }
  }

  void MultiSlaterDeterminant::restore(int iat)
  {
    if(DetID[iat] == 0) {
      for(int i=0; i<dets_up.size(); i++) 
        dets_up[i]->restore(iat);
    } else {
      for(int i=0; i<dets_dn.size(); i++)
        dets_dn[i]->restore(iat);
    }
    curRatio=1.0;
  }

  void MultiSlaterDeterminant::update(ParticleSet& P
      , ParticleSet::ParticleGradient_t& dG, ParticleSet::ParticleLaplacian_t& dL
      , int iat)
  {
    APP_ABORT("IMPLEMENT MultiSlaterDeterminant::update");
  }

  OrbitalBase::RealType MultiSlaterDeterminant::evaluateLog(ParticleSet& P,BufferType& buf)
  {

    ValueType logpsi(0.0);
    for (int i=0; i<dets_up.size(); i++)
      logpsi += dets_up[i]->evaluateLog(P,buf);
    for (int i=0; i<dets_dn.size(); i++)
      logpsi += dets_dn[i]->evaluateLog(P,buf);

    int TotalDim = PosType::Size*P.getTotalNum();
    buf.put(detValues_up.begin(),detValues_up.end());
    buf.put(detValues_dn.begin(),detValues_dn.end());
    for(int i=0; i<NumUniqueDets_up; i++) {
      buf.put(&(grads_up[i][0][0]), &(grads_up[i][0][0])+TotalDim);
      buf.put(&(lapls_up[i][0]), &(lapls_up[i][P.getTotalNum()]));
    }
    for(int i=0; i<NumUniqueDets_dn; i++) {
      buf.put(&(grads_dn[i][0][0]), &(grads_dn[i][0][0])+TotalDim);
      buf.put(&(lapls_dn[i][0]), &(lapls_dn[i][P.getTotalNum()]));
    }
    return LogValue; 
  }

  OrbitalBase::RealType MultiSlaterDeterminant::registerData(ParticleSet& P, BufferType& buf)
  {
// move resize of pbyp structures to here
    spo_up->evaluateForWalkerMove(P,FirstIndex_up,LastIndex_up);
    spo_dn->evaluateForWalkerMove(P,FirstIndex_dn,LastIndex_dn);

    myG = P.G;
    myL = P.L;

    ValueType logpsi(0.0);
    PhaseValue=0.0;
    for (int i=0; i<dets_up.size(); i++)
    {
      spo_up->prepareFor(i);
      logpsi += dets_up[i]->registerData(P,buf);
    }
    for (int i=0; i<dets_dn.size(); i++)
    {
      spo_dn->prepareFor(i);
      logpsi += dets_dn[i]->registerData(P,buf);
    }

    P.G = myG;
    P.L = myL;
    logpsi = evaluateLog(P,P.G,P.L);

    int TotalDim = PosType::Size*P.getTotalNum();

    buf.add(detValues_up.begin(),detValues_up.end());
    buf.add(detValues_dn.begin(),detValues_dn.end());
    for(int i=0; i<NumUniqueDets_up; i++) {
      buf.add(&(grads_up[i][0][0]), &(grads_up[i][0][0])+TotalDim);
      buf.add(&(lapls_up[i][0]), &(lapls_up[i][P.getTotalNum()]));
    }
    for(int i=0; i<NumUniqueDets_dn; i++) {
      buf.add(&(grads_dn[i][0][0]), &(grads_dn[i][0][0])+TotalDim);
      buf.add(&(lapls_dn[i][0]), &(lapls_dn[i][P.getTotalNum()]));
    }
    return LogValue;
  }

// FIX FIX FIX
  OrbitalBase::RealType MultiSlaterDeterminant::updateBuffer(ParticleSet& P, BufferType& buf, bool fromscratch)
  {

    if(fromscratch || dets_up[0]->UpdateMode == ORB_PBYP_RATIO) {
      spo_up->evaluateForWalkerMove(P,FirstIndex_up,LastIndex_up);
      spo_dn->evaluateForWalkerMove(P,FirstIndex_dn,LastIndex_dn);
    }

    myG = P.G;
    myL = P.L;

    ValueType logpsi(0.0);
    PhaseValue=0.0;
    for (int i=0; i<dets_up.size(); i++) {
      spo_up->prepareFor(i);
      logpsi += dets_up[i]->updateBuffer(P,buf,fromscratch);
    }
    for (int i=0; i<dets_dn.size(); i++) {
      spo_dn->prepareFor(i);
      logpsi += dets_dn[i]->updateBuffer(P,buf,fromscratch);
    }

    int TotalDim = PosType::Size*P.getTotalNum();
    buf.put(detValues_up.begin(),detValues_up.end());
    buf.put(detValues_dn.begin(),detValues_dn.end());
    for(int i=0; i<NumUniqueDets_up; i++) {
      buf.put(&(grads_up[i][0][0]), &(grads_up[i][0][0])+TotalDim);
      buf.put(&(lapls_up[i][0]), &(lapls_up[i][P.getTotalNum()]));
    }
    for(int i=0; i<NumUniqueDets_dn; i++) {
      buf.put(&(grads_dn[i][0][0]), &(grads_dn[i][0][0])+TotalDim);
      buf.put(&(lapls_dn[i][0]), &(lapls_dn[i][P.getTotalNum()]));
    }

    P.G=myG;
    P.L=myL;

    ValueType psi=0.0;
    myG=0.0;
    myL=0.0;
    for(int i=0; i<C.size(); i++){
      int upC = C2node_up[i];
      int dnC = C2node_dn[i];
      ValueType tmp = C[i]*detValues_up[upC]*detValues_dn[dnC];
      psi += tmp;
      //for(int n=FirstIndex_up; n<LastIndex_up; n++) {
        myG += tmp*grads_up[upC]; // other spin sector should be zero 
        myL += tmp*lapls_up[upC];
      //}
      //for(int n=FirstIndex_dn; n<LastIndex_dn; n++) {
        myG += tmp*grads_dn[dnC]; // other spin sector should be zero 
        myL += tmp*lapls_dn[dnC];
      //}
    }
    ValueType psiinv = 1.0/psi;
    myG *= psiinv;
    myL *= psiinv;
    P.G += myG;
    for(int i=0; i<P.L.size(); i++)
      P.L(i) += myL[i] - dot(myG[i],myG[i]);

    return LogValue = evaluateLogAndPhase(psi,PhaseValue);;
  }

  void MultiSlaterDeterminant::copyFromBuffer(ParticleSet& P, BufferType& buf)
  {

    for (int i=0; i<dets_up.size(); i++)
      dets_up[i]->copyFromBuffer(P,buf);
    for (int i=0; i<dets_dn.size(); i++)
      dets_dn[i]->copyFromBuffer(P,buf);

    int TotalDim = PosType::Size*P.getTotalNum();
    buf.get(detValues_up.begin(),detValues_up.end());
    buf.get(detValues_dn.begin(),detValues_dn.end());
    for(int i=0; i<NumUniqueDets_up; i++) {
      buf.get(&(grads_up[i][0][0]), &(grads_up[i][0][0])+TotalDim);
      buf.get(&(lapls_up[i][0]), &(lapls_up[i][P.getTotalNum()]));
    }
    for(int i=0; i<NumUniqueDets_dn; i++) {
      buf.get(&(grads_dn[i][0][0]), &(grads_dn[i][0][0])+TotalDim);
      buf.get(&(lapls_dn[i][0]), &(lapls_dn[i][P.getTotalNum()]));
    }
  }


  void MultiSlaterDeterminant::checkInVariables(opt_variables_type& active)
  {
    if(Optimizable) 
    {
      if(myVars.size()) 
        active.insertFrom(myVars);
      else  
        Optimizable=false;
    }
  }

  void MultiSlaterDeterminant::checkOutVariables(const opt_variables_type& active)
  {
    if(Optimizable) myVars.getIndex(active);
  }

  /** resetParameters with optVariables
   *
   * USE_resetParameters
   */
  void MultiSlaterDeterminant::resetParameters(const opt_variables_type& active)
  {  
    if(Optimizable) 
    {
      for(int i=0; i<C.size(); i++) 
      {
        int loc=myVars.where(i);
        if(loc>=0) C[i]=myVars[i]=active[loc];
      }
      //for(int i=0; i<SDets.size(); i++) SDets[i]->resetParameters(active);
    }
  }
  void MultiSlaterDeterminant::reportStatus(ostream& os)
  {
  }

//   OrbitalBasePtr MultiSlaterDeterminant::makeClone(ParticleSet& tqp) const
//   {
//      APP_ABORT("IMPLEMENT OrbitalBase::makeClone");
//      return 0;
//   }

  void MultiSlaterDeterminant::evaluateDerivatives(ParticleSet& P, 
      const opt_variables_type& optvars,
      vector<RealType>& dlogpsi,
      vector<RealType>& dhpsioverpsi)
  {
    bool recalculate(false);
    for (int k=0; k<myVars.size(); ++k)
    {
      int kk=myVars.where(k);
      if (kk<0) continue;
      if (optvars.recompute(kk)) recalculate=true;
    }


// need to modify for CSF later on, right now assume Slater Det basis 
    if (recalculate)
    {
      int n = P.getTotalNum();
      ValueType psi = std::cos(PhaseValue)*std::exp(LogValue);
      ValueType psiinv = 1.0/psi;;
      ValueType lapl_sum=0.0;
      ParticleSet::ParticleGradient_t g(n);
      ValueType gg=0.0, ggP=0.0;
      g=0.0;
      for(int i=0; i<C.size(); i++){
        int upC = C2node_up[i];
        int dnC = C2node_dn[i];
        ValueType tmp = C[i]*detValues_up[upC]*detValues_dn[dnC]*psiinv;
        lapl_sum += tmp*(Sum(lapls_up[upC])+Sum(lapls_dn[dnC]));
        g += tmp*grads_up[upC];
        g += tmp*grads_dn[dnC];
      }
      gg=Dot(g,g);
      ggP=Dot(P.G,g);
 
      for(int i=0; i<C.size(); i++){
        int kk=myVars.where(i);
        if (kk<0) continue;
        //dlogpsi[kk] = cdet;
        int upC = C2node_up[i];
        int dnC = C2node_dn[i];
        ValueType cdet=detValues_up[upC]*detValues_dn[dnC]*psiinv;
        convert(cdet,dlogpsi[kk]);
        ValueType dhpsi =  (-0.5*cdet)*
                       ( Sum(lapls_up[upC])+Sum(lapls_dn[dnC])-lapl_sum
                          +2.0*(gg-Dot(g,grads_up[upC])-Dot(g,grads_dn[dnC])
                          +Dot(P.G,grads_up[upC])+Dot(P.G,grads_dn[dnC])-ggP));
        convert(dhpsi,dhpsioverpsi[kk]);
      }
    
    }
  }
}
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 3416 $   $Date: 2008-12-07 11:34:49 -0600 (Sun, 07 Dec 2008) $
 * $Id: MultiSlaterDeterminant.cpp 3416 2008-12-07 17:34:49Z jnkim $
 ***************************************************************************/
