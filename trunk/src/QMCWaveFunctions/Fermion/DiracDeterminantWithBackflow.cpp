//////////////////////////////////////////////////////////////////
// (c) Copyright 1998-2002,2003- by Jeongnim Kim
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

#include "QMCWaveFunctions/Fermion/DiracDeterminantWithBackflow.h"
#include "QMCWaveFunctions/Fermion/BackflowTransformation.h"
#include "Numerics/DeterminantOperators.h"
#include "Numerics/OhmmsBlas.h"
#include "Numerics/MatrixOperators.h"
#include "OhmmsPETE/Tensor.h"

namespace qmcplusplus {

  /** constructor
   *@param spos the single-particle orbital set
   *@param first index of the first particle
   */
  DiracDeterminantWithBackflow::DiracDeterminantWithBackflow(SPOSetBasePtr const &spos, BackflowTransformation * BF, int first): DiracDeterminantBase(spos,first)
  {
    Optimizable=true;
    OrbitalName="DiracDeterminantWithBackflow";
    registerTimers();
    BFTrans=BF;
  }

  ///default destructor
  DiracDeterminantWithBackflow::~DiracDeterminantWithBackflow() {}

  DiracDeterminantWithBackflow& DiracDeterminantWithBackflow::operator=(const DiracDeterminantWithBackflow& s) {
    NP=0;
    resize(s.NumPtcls, s.NumOrbitals);
    return *this;
  }


  ///reset the size: with the number of particles and number of orbtials
  void DiracDeterminantWithBackflow::resize(int nel, int morb) {
    int norb=morb;
    if(norb <= 0) norb = nel; // for morb == -1 (default)
    psiM.resize(nel,norb);
    dpsiM.resize(nel,norb);
    //d2psiM.resize(nel,norb);
    //psiM_temp.resize(nel,norb);
    dpsiM_temp.resize(nel,norb);
    //d2psiM_temp.resize(nel,norb);
    psiMinv.resize(nel,norb);
    //psiV.resize(norb);
    WorkSpace.resize(nel);
    Pivot.resize(nel);
    LastIndex = FirstIndex + nel;
    NumPtcls=nel;
    NumOrbitals=norb;
    grad_grad_psiM.resize(nel,norb);
    grad_grad_grad_psiM.resize(nel,norb);
    Gtemp.resize(2*nel); // not correct for spin polarized...
    dFa.resize(nel,norb);
    Ajk_sum.resize(nel,norb);
    Qmat.resize(nel,norb);

    // For forces
    /*  not used
    grad_source_psiM.resize(nel,norb);
    grad_lapl_source_psiM.resize(nel,norb);
    grad_grad_source_psiM.resize(nel,norb);
    phi_alpha_Minv.resize(nel,norb);
    grad_phi_Minv.resize(nel,norb);
    lapl_phi_Minv.resize(nel,norb);
    grad_phi_alpha_Minv.resize(nel,norb);
    */
  }

  DiracDeterminantWithBackflow::RealType 
    DiracDeterminantWithBackflow::registerData(ParticleSet& P, PooledData<RealType>& buf) 
    {

    if(NP == 0) {//first time, allocate once
      //int norb = cols();
      NP=P.getTotalNum();
      myG.resize(NP);
      myL.resize(NP);
      myG_temp.resize(NP);
      myL_temp.resize(NP);
      resize(NumPtcls,NumOrbitals);
      FirstAddressOfG = &myG[0][0];
      LastAddressOfG = FirstAddressOfG + NP*DIM;
      FirstAddressOfdV = &(dpsiM(0,0)[0]); //(*dpsiM.begin())[0]);
      LastAddressOfdV = FirstAddressOfdV + NumPtcls*NumOrbitals*DIM;
    }

    myG=0.0;
    myL=0.0;

    //ValueType x=evaluate(P,myG,myL); 
    LogValue=evaluateLog(P,myG,myL); 

    P.G += myG;
    P.L += myL;

    //add the data: determinant, inverse, gradient and laplacians
    buf.add(psiM.first_address(),psiM.last_address());
    buf.add(FirstAddressOfdV,LastAddressOfdV);
    buf.add(d2psiM.first_address(),d2psiM.last_address());
    buf.add(myL.first_address(), myL.last_address());
    buf.add(FirstAddressOfG,LastAddressOfG);
    buf.add(LogValue);
    buf.add(PhaseValue);

    return LogValue;
  }

  DiracDeterminantWithBackflow::RealType DiracDeterminantWithBackflow::updateBuffer(ParticleSet& P, 
      PooledData<RealType>& buf, bool fromscratch) 
  {
    myG=0.0;
    myL=0.0;

    UpdateTimer.start();

    LogValue=evaluateLog(P,myG,myL);

    P.G += myG;
    P.L += myL;

    //copy psiM to psiM_temp
    psiM_temp=psiM;

    buf.put(psiM.first_address(),psiM.last_address());
    buf.put(FirstAddressOfdV,LastAddressOfdV);
    buf.put(d2psiM.first_address(),d2psiM.last_address());
    buf.put(myL.first_address(), myL.last_address());
    buf.put(FirstAddressOfG,LastAddressOfG);
    buf.put(LogValue);
    buf.put(PhaseValue);

    UpdateTimer.stop();
    return LogValue;
  }

  void DiracDeterminantWithBackflow::copyFromBuffer(ParticleSet& P, PooledData<RealType>& buf) {

    buf.get(psiM.first_address(),psiM.last_address());
    buf.get(FirstAddressOfdV,LastAddressOfdV);
    buf.get(d2psiM.first_address(),d2psiM.last_address());
    buf.get(myL.first_address(), myL.last_address());
    buf.get(FirstAddressOfG,LastAddressOfG);
    buf.get(LogValue);
    buf.get(PhaseValue);

    //re-evaluate it for testing
    //Phi.evaluate(P, FirstIndex, LastIndex, psiM, dpsiM, d2psiM);
    //CurrentDet = Invert(psiM.data(),NumPtcls,NumOrbitals);
    //need extra copy for gradient/laplacian calculations without updating it
    //psiM_temp = psiM;
    //dpsiM_temp = dpsiM;
    //d2psiM_temp = d2psiM;
  }

  /** return the ratio only for the  iat-th partcle move
   * @param P current configuration
   * @param iat the particle thas is being moved
   */
  DiracDeterminantWithBackflow::ValueType DiracDeterminantWithBackflow::ratio(ParticleSet& P, int iat) 
  {
    RealType OldLog = LogValue;
    RealType OldPhase = PhaseValue;

    Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM,dpsiM,grad_grad_psiM);
    //app_log() <<psiM <<endl;

    //std::copy(psiM.begin(),psiM.end(),psiMinv.begin());
    psiMinv=psiM;

    // invert backflow matrix
    InverseTimer.start();
    LogValue=InvertWithLog(psiMinv.data(),NumPtcls,NumOrbitals,WorkSpace.data(),Pivot.data(),PhaseValue);
    InverseTimer.stop();
   
    return LogValue/OldLog; 
  }
    
  void DiracDeterminantWithBackflow::get_ratios(ParticleSet& P, vector<ValueType>& ratios)
  {
     APP_ABORT(" Need to implement DiracDeterminantWithBackflow::get_ratios(ParticleSet& P, int iat). \n");
  }

  DiracDeterminantWithBackflow::GradType 
    DiracDeterminantWithBackflow::evalGrad(ParticleSet& P, int iat)
  {
     APP_ABORT(" Need to implement DiracDeterminantWithBackflow::ratio(ParticleSet& P, int iat). \n");
     return 0.0;
  }

  DiracDeterminantWithBackflow::GradType 
    DiracDeterminantWithBackflow::evalGradSource(ParticleSet& P, ParticleSet& source,
					 int iat)
  {
     APP_ABORT(" Need to implement DiracDeterminantWithBackflow::evalGradSource() \n");
     return 0.0;
  }

  DiracDeterminantWithBackflow::GradType
  DiracDeterminantWithBackflow::evalGradSourcep
  (ParticleSet& P, ParticleSet& source,int iat,
   TinyVector<ParticleSet::ParticleGradient_t, OHMMS_DIM> &grad_grad,
   TinyVector<ParticleSet::ParticleLaplacian_t,OHMMS_DIM> &lapl_grad)
  {
     APP_ABORT(" Need to implement DiracDeterminantWithBackflow::evalGradSourcep() \n");
     return 0.0;
  }


  DiracDeterminantWithBackflow::GradType
  DiracDeterminantWithBackflow::evalGradSource
  (ParticleSet& P, ParticleSet& source,int iat,
   TinyVector<ParticleSet::ParticleGradient_t, OHMMS_DIM> &grad_grad,
   TinyVector<ParticleSet::ParticleLaplacian_t,OHMMS_DIM> &lapl_grad)
  {
     APP_ABORT(" Need to implement DiracDeterminantWithBackflow::evalGradSource() \n");
     return 0.0;
  }

    DiracDeterminantWithBackflow::ValueType 
      DiracDeterminantWithBackflow::ratioGrad(ParticleSet& P, int iat, GradType& grad_iat)
  {
     APP_ABORT(" Need to implement DiracDeterminantWithBackflow::ratioGrad() \n");
     return 0.0;
  }

  /** return the ratio
   * @param P current configuration
   * @param iat particle whose position is moved
   * @param dG differential Gradients
   * @param dL differential Laplacians
   *
   * Data member *_temp contain the data assuming that the move is accepted
   * and are used to evaluate differential Gradients and Laplacians.
   */
  DiracDeterminantWithBackflow::ValueType DiracDeterminantWithBackflow::ratio(ParticleSet& P, int iat,
      ParticleSet::ParticleGradient_t& dG, 
      ParticleSet::ParticleLaplacian_t& dL) 
  { 
     APP_ABORT(" Need to implement DiracDeterminantWithBackflow::ratio() \n");
     return 0.0;
  }


  DiracDeterminantWithBackflow::RealType
    DiracDeterminantWithBackflow::evaluateLog(ParticleSet& P,
        ParticleSet::ParticleGradient_t& G,
        ParticleSet::ParticleLaplacian_t& L)
  {
    // calculate backflow matrix, 1st and 2nd derivatives
    Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM,dpsiM,grad_grad_psiM);
    //app_log() <<psiM <<endl;

    //std::copy(psiM.begin(),psiM.end(),psiMinv.begin());
    psiMinv=psiM;

    // invert backflow matrix
    InverseTimer.start();
    LogValue=InvertWithLog(psiMinv.data(),NumPtcls,NumOrbitals,WorkSpace.data(),Pivot.data(),PhaseValue);
    InverseTimer.stop();

    // calculate F matrix (gradients wrt bf coordinates)
    // could use dgemv with increments of 3*nCols  
    for(int i=0; i<NumPtcls; i++)
    for(int j=0; j<NumPtcls; j++)
    {
       dpsiM_temp(i,j)=dot(psiMinv[i],dpsiM[j],NumOrbitals);
    }
    //for(int i=0, iat=FirstIndex; i<NumPtcls; i++, iat++)
    // G(iat) += dpsiM_temp(i,i);

    // calculate gradients and first piece of laplacians 
    GradType temp;
    ValueType temp2;
    int num = P.getTotalNum();
    for(int i=0; i<num; i++) {
      temp=0.0;
      temp2=0.0;
      for(int j=0; j<NumPtcls; j++) {
        temp2 += dot(BFTrans->Bmat_full(i,FirstIndex+j),dpsiM_temp(j,j));
        temp  += dot(BFTrans->Amat(i,FirstIndex+j),dpsiM_temp(j,j));
      } 
      G(i) += temp; 
      L(i) += temp2;
    }

    for(int j=0; j<NumPtcls; j++) {
      HessType q_j = 0.0;
      for(int k=0; k<NumPtcls; k++)  q_j += psiMinv(j,k)*grad_grad_psiM(j,k);  
      for(int i=0; i<num; i++) {
        L(i) += traceAtB(dot(transpose(BFTrans->Amat(i,FirstIndex+j)),BFTrans->Amat(i,FirstIndex+j)),q_j);
      }

      for(int k=0; k<NumPtcls; k++) {
        for(int i=0; i<num; i++) {
          L(i) -= traceAtB(dot(transpose(BFTrans->Amat(i,FirstIndex+j)),BFTrans->Amat(i,FirstIndex+k)), outerProduct(dpsiM_temp(k,j),dpsiM_temp(j,k)));
        }
      }
    }

    return LogValue;
  }

  DiracDeterminantWithBackflow::ValueType DiracDeterminantWithBackflow::logRatio(ParticleSet& P, int iat,
      ParticleSet::ParticleGradient_t& dG, 
      ParticleSet::ParticleLaplacian_t& dL) {
    APP_ABORT("  logRatio is not allowed");
    //THIS SHOULD NOT BE CALLED
    ValueType r=ratio(P,iat,dG,dL);
    return LogValue = evaluateLogAndPhase(r,PhaseValue);
  }


  /** move was accepted, update the real container
  */
  void DiracDeterminantWithBackflow::acceptMove(ParticleSet& P, int iat) 
  {
     APP_ABORT(" Need to implement DiracDeterminantWithBackflow::acceptMove() \n");
  }

  /** move was rejected. Nothing to restore for now. 
  */
  void DiracDeterminantWithBackflow::restore(int iat) {
     curRatio=1.0;
  }

  void DiracDeterminantWithBackflow::update(ParticleSet& P, 
      ParticleSet::ParticleGradient_t& dG, 
      ParticleSet::ParticleLaplacian_t& dL,
      int iat) {
      
     APP_ABORT(" Need to implement DiracDeterminantWithBackflow::update() \n");

  }


  
  DiracDeterminantWithBackflow::RealType
    DiracDeterminantWithBackflow::evaluateLog(ParticleSet& P, PooledData<RealType>& buf)
    {

     APP_ABORT(" Need to implement DiracDeterminantWithBackflow::evaluateLog(PooldedData)() \n");
      return LogValue;
    }


  /** Calculate the value of the Dirac determinant for particles
   *@param P input configuration containing N particles
   *@param G a vector containing N gradients
   *@param L a vector containing N laplacians
   *@return the value of the determinant
   *
   *\f$ (first,first+nel). \f$  Add the gradient and laplacian 
   *contribution of the determinant to G(radient) and L(aplacian)
   *for local energy calculations.
   */ 
  DiracDeterminantWithBackflow::ValueType
    DiracDeterminantWithBackflow::evaluate(ParticleSet& P, 
        ParticleSet::ParticleGradient_t& G, 
        ParticleSet::ParticleLaplacian_t& L)
    {

//       APP_ABORT("  DiracDeterminantWithBackflow::evaluate is disabled");

      ValueType logval = evaluateLog(P, G, L);
      return std::cos(PhaseValue)*std::exp(logval);
    }

  void
  DiracDeterminantWithBackflow::evaluateDerivatives(ParticleSet& P,
                                            const opt_variables_type& active,
                                            vector<RealType>& dlogpsi,
                                            vector<RealType>& dhpsioverpsi)
  {
/*  Note:
 *    Since evaluateDerivatives seems to always be called after
 *    evaluateDeltaLog, which in turn calls evaluateLog, many
 *    of the structures calculated here do not need to be calculated
 *    again. The only one that need to be called is a routine
 *    to calculate grad_grad_grad_psiM. The following structures
 *    should already be known here:
 *       -psiM
 *       -dpsiM
 *       -grad_grad_psiM
 *       -psiM_inv
 *       -dpsiM_temp
 */     

    Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM,dpsiM,grad_grad_psiM,grad_grad_grad_psiM); 
    
    //std::copy(psiM.begin(),psiM.end(),psiMinv.begin());
    psiMinv=psiM;

    // invert backflow matrix
    InverseTimer.start();
    LogValue=InvertWithLog(psiMinv.data(),NumPtcls,NumOrbitals,WorkSpace.data(),Pivot.data(),PhaseValue); 
    InverseTimer.stop();
    
    // calculate F matrix (gradients wrt bf coordinates)
    // could use dgemv with increments of 3*nCols  
    for(int i=0; i<NumPtcls; i++)
    for(int j=0; j<NumPtcls; j++)
    {
       dpsiM_temp(i,j)=dot(psiMinv[i],dpsiM[j],NumOrbitals);
    }
    //for(int i=0, iat=FirstIndex; i<NumPtcls; i++, iat++)
    // G(iat) += dpsiM_temp(i,i);

    int num = P.getTotalNum();
    for(int j=0; j<NumPtcls; j++)  
      for(int k=0; k<NumPtcls; k++) { 

        HessType& q_jk = Qmat(j,k);
        q_jk=0.0;
        for(int n=0; n<NumPtcls; n++)  
          q_jk += psiMinv(j,n)*grad_grad_psiM(k,n);
      
        HessType& a_jk = Ajk_sum(j,k);
        a_jk=0.0;
        for(int n=0; n<num; n++)  
          a_jk += dot(transpose(BFTrans->Amat(n,FirstIndex+j)),BFTrans->Amat(n,FirstIndex+k));

      }

   // this is a mess, there should be a better way
   // to rearrange this  
   for (int pa=0; pa<BFTrans->optIndexMap.size(); ++pa)
   //for (int pa=0; pa<BFTrans->numParams; ++pa)
   {
      ValueType dpsia=0.0;
      Gtemp=0.0;
      ValueType dLa=0.0;
      GradType temp;
      ValueType temp2;
      for(int i=0; i<NumPtcls; i++)
        for(int j=0; j<NumPtcls; j++) {
          GradType f_a=0.0;
          GradType& cj = BFTrans->Cmat(pa,FirstIndex+j);
          for(int k=0; k<NumPtcls; k++) {
             f_a += (psiMinv(i,k)*dot(grad_grad_psiM(j,k),cj)
                   -  dpsiM_temp(k,j)*dot(BFTrans->Cmat(pa,FirstIndex+k),dpsiM_temp(i,k)));
          }
          dFa(i,j)=f_a;
        }
      for(int i=0; i<num; i++) {
        temp=0.0;
        for(int j=0; j<NumPtcls; j++)
          temp += (dot(BFTrans->Xmat(pa,i,FirstIndex+j),dpsiM_temp(j,j))
                    + dot(BFTrans->Amat(i,FirstIndex+j),dFa(j,j)));
        Gtemp(i) += temp;
      }
      for(int j=0; j<NumPtcls; j++) {
        GradType B_j=0.0;
        for(int i=0; i<num; i++) B_j += BFTrans->Bmat_full(i,FirstIndex+j);
        dLa += (dot(dpsiM_temp(j,j),BFTrans->Ymat(pa,FirstIndex+j)) +
                  dot(B_j,dFa(j,j)));
        dpsia += dot(dpsiM_temp(j,j),BFTrans->Cmat(pa,FirstIndex+j));
      }

     for(int j=0; j<NumPtcls; j++) {

      HessType a_j_prime = 0.0;
      for(int i=0; i<num; i++) a_j_prime += ( dot(transpose(BFTrans->Xmat(pa,i,FirstIndex+j)),BFTrans->Amat(i,FirstIndex+j)) + dot(transpose(BFTrans->Amat(i,FirstIndex+j)),BFTrans->Xmat(pa,i,FirstIndex+j)) );

      HessType q_j_prime = 0.0; // ,tmp;
      GradType& cj = BFTrans->Cmat(pa,FirstIndex+j);
      for(int k=0; k<NumPtcls; k++)  {
        //tmp=0.0;
        //for(int n=0; n<NumPtcls; n++) tmp+=psiMinv(k,n)*grad_grad_psiM(j,n);
        //tmp *= dot(BFTrans->Cmat(pa,FirstIndex+k),dpsiM_temp(j,k));

        q_j_prime += ( psiMinv(j,k)*(cj[0]*grad_grad_grad_psiM(j,k)[0]
                       + cj[1]*grad_grad_grad_psiM(j,k)[1]
                       + cj[2]*grad_grad_grad_psiM(j,k)[2]) 
                     - dot(BFTrans->Cmat(pa,FirstIndex+k),dpsiM_temp(j,k))
                       *Qmat(k,j) );
      }

      dLa += (traceAtB(a_j_prime,Qmat(j,j)) + traceAtB(Ajk_sum(j,j),q_j_prime));
     }
     for(int j=0; j<NumPtcls; j++) {
      for(int k=0; k<NumPtcls; k++) {

        HessType a_jk_prime = 0.0;
        for(int i=0; i<num; i++) a_jk_prime += ( dot(transpose(BFTrans->Xmat(pa,i,FirstIndex+j)),BFTrans->Amat(i,FirstIndex+k)) + dot(transpose(BFTrans->Amat(i,FirstIndex+j)),BFTrans->Xmat(pa,i,FirstIndex+k)) );

        dLa -= (traceAtB(a_jk_prime, outerProduct(dpsiM_temp(k,j),dpsiM_temp(j,k)))
               + traceAtB(Ajk_sum(j,k), outerProduct(dFa(k,j),dpsiM_temp(j,k))
               + outerProduct(dpsiM_temp(k,j),dFa(j,k)) ));

      }  // k
     }   // j

     //int kk = pa; //BFTrans->optIndexMap[pa];
     int kk = BFTrans->optIndexMap[pa];
     dlogpsi[kk]+=dpsia;
     dhpsioverpsi[kk] -= (0.5*dLa+Dot(P.G,Gtemp));
   }
  }

  void DiracDeterminantWithBackflow::evaluateDerivatives(ParticleSet& P,
                                       const opt_variables_type& active,
                                       vector<RealType>& dlogpsi,
                                       vector<RealType>& dhpsioverpsi,
                                       ParticleSet::ParticleGradient_t* G0,
                                       ParticleSet::ParticleLaplacian_t* L0,
                                       int pa )
   {

    Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM,dpsiM,grad_grad_psiM,grad_grad_grad_psiM);

    //std::copy(psiM.begin(),psiM.end(),psiMinv.begin());
    psiMinv=psiM;

    // invert backflow matrix
    InverseTimer.start();
    LogValue=InvertWithLog(psiMinv.data(),NumPtcls,NumOrbitals,WorkSpace.data(),Pivot.data(),PhaseValue);
    InverseTimer.stop();

    // calculate F matrix (gradients wrt bf coordinates)
    // could use dgemv with increments of 3*nCols  
    for(int i=0; i<NumPtcls; i++)
    for(int j=0; j<NumPtcls; j++)
    {
       dpsiM_temp(i,j)=dot(psiMinv[i],dpsiM[j],NumOrbitals);
    }
    //for(int i=0, iat=FirstIndex; i<NumPtcls; i++, iat++)
    // G(iat) += dpsiM_temp(i,i);

   // this is a mess, there should be a better way
   // to rearrange this  
   //for (int pa=0; pa<BFTrans->optIndexMap.size(); ++pa)
   //for (int pa=0; pa<BFTrans->numParams; ++pa)
   {
      ValueType dpsia=0.0;
      Gtemp=0.0;
      //ValueType dLa=0.0;
      La1=La2=La3=0.0;
      GradType temp;
      ValueType temp2;
      int num = P.getTotalNum();
      for(int i=0; i<NumPtcls; i++) 
        for(int j=0; j<NumPtcls; j++) {
          GradType f_a=0.0;
          GradType& cj = BFTrans->Cmat(pa,FirstIndex+j);
          for(int k=0; k<NumPtcls; k++) {
             f_a += (psiMinv(i,k)*dot(grad_grad_psiM(j,k),cj)
                   -  dpsiM_temp(k,j)*dot(BFTrans->Cmat(pa,FirstIndex+k),dpsiM_temp(i,k)));
          }
          dFa(i,j)=f_a;
        }
      for(int i=0; i<num; i++) {
        temp=0.0;
        for(int j=0; j<NumPtcls; j++) 
          temp += (dot(BFTrans->Xmat(pa,i,FirstIndex+j),dpsiM_temp(j,j)) 
                    + dot(BFTrans->Amat(i,FirstIndex+j),dFa(j,j)));
        Gtemp(i) += temp;
      }
      for(int j=0; j<NumPtcls; j++) {
        GradType B_j=0.0; 
        for(int i=0; i<num; i++) B_j += BFTrans->Bmat_full(i,FirstIndex+j); 
        La1 += (dot(dpsiM_temp(j,j),BFTrans->Ymat(pa,FirstIndex+j)) +
                  dot(B_j,dFa(j,j)));
        dpsia += dot(dpsiM_temp(j,j),BFTrans->Cmat(pa,FirstIndex+j));
      }

     for(int j=0; j<NumPtcls; j++) {

      HessType q_j = 0.0;
      for(int k=0; k<NumPtcls; k++)  q_j += psiMinv(j,k)*grad_grad_psiM(j,k); 
// define later the dot product with a transpose tensor
      HessType a_j = 0.0;
      for(int i=0; i<num; i++) a_j += dot(transpose(BFTrans->Amat(i,FirstIndex+j)),BFTrans->Amat(i,FirstIndex+j));
  
      HessType a_j_prime = 0.0;
      for(int i=0; i<num; i++) a_j_prime += ( dot(transpose(BFTrans->Xmat(pa,i,FirstIndex+j)),BFTrans->Amat(i,FirstIndex+j)) + dot(transpose(BFTrans->Amat(i,FirstIndex+j)),BFTrans->Xmat(pa,i,FirstIndex+j)) );

      HessType q_j_prime = 0.0, tmp;
      GradType& cj = BFTrans->Cmat(pa,FirstIndex+j);
      for(int k=0; k<NumPtcls; k++)  {
        tmp=0.0;
        for(int n=0; n<NumPtcls; n++) tmp+=psiMinv(k,n)*grad_grad_psiM(j,n);
        tmp *= dot(BFTrans->Cmat(pa,FirstIndex+k),dpsiM_temp(j,k));
      
        q_j_prime += ( psiMinv(j,k)*(cj[0]*grad_grad_grad_psiM(j,k)[0] 
                       + cj[1]*grad_grad_grad_psiM(j,k)[1]  
                       + cj[2]*grad_grad_grad_psiM(j,k)[2]) - tmp); 
      }
       
      La2 += (traceAtB(a_j_prime,q_j) + traceAtB(a_j,q_j_prime)); 
     }

     for(int j=0; j<NumPtcls; j++) {
      for(int k=0; k<NumPtcls; k++) {
          
        HessType a_jk = 0.0;
        for(int i=0; i<num; i++) a_jk += dot(transpose(BFTrans->Amat(i,FirstIndex+j)),BFTrans->Amat(i,FirstIndex+k));

        HessType a_jk_prime = 0.0;
        for(int i=0; i<num; i++) a_jk_prime += ( dot(transpose(BFTrans->Xmat(pa,i,FirstIndex+j)),BFTrans->Amat(i,FirstIndex+k)) + dot(transpose(BFTrans->Amat(i,FirstIndex+j)),BFTrans->Xmat(pa,i,FirstIndex+k)) );        
 
        La3 -= (traceAtB(a_jk_prime, outerProduct(dpsiM_temp(k,j),dpsiM_temp(j,k))) 
               + traceAtB(a_jk, outerProduct(dFa(k,j),dpsiM_temp(j,k)) 
               + outerProduct(dpsiM_temp(k,j),dFa(j,k)) )); 

      }  // k
     }   // j

     int kk = pa; //BFTrans->optIndexMap[pa];
     dlogpsi[kk]+=dpsia;
     dhpsioverpsi[kk] -= (0.5*(La1+La2+La3)+Dot(P.G,Gtemp));
     *G0 += Gtemp;
     (*L0)[0] += La1+La2+La3;
   }

  }

  OrbitalBasePtr DiracDeterminantWithBackflow::makeClone(ParticleSet& tqp) const
  {
    APP_ABORT(" Illegal action. Cannot use DiracDeterminantWithBackflow::makeClone");
    return 0;
  }

  DiracDeterminantWithBackflow* DiracDeterminantWithBackflow::makeCopy(SPOSetBasePtr spo) const
  {
    BackflowTransformation *BF = BFTrans->makeClone(); 
    DiracDeterminantWithBackflow* dclone= new DiracDeterminantWithBackflow(spo,BF);
    dclone->set(FirstIndex,LastIndex-FirstIndex);
    return dclone;
  }

  DiracDeterminantWithBackflow::DiracDeterminantWithBackflow(const DiracDeterminantWithBackflow& s): 
    DiracDeterminantBase(s),BFTrans(s.BFTrans)
  {
    registerTimers();
    this->resize(s.NumPtcls,s.NumOrbitals);
  }

  //SPOSetBasePtr  DiracDeterminantWithBackflow::clonePhi() const
  //{
  //  return Phi->makelone();
  //}

  void DiracDeterminantWithBackflow::testGGG(ParticleSet& P)
  {
    ParticleSet::ParticlePos_t qp_0;
    qp_0.resize(BFTrans->QP.getTotalNum());

    ValueMatrix_t psiM_1,psiM_2;
    GradMatrix_t dpsiM_1,dpsiM_2;
    HessMatrix_t ggM_1, ggM_2;
    psiM_1.resize(NumPtcls,NumOrbitals);
    psiM_2.resize(NumPtcls,NumOrbitals);
    dpsiM_1.resize(NumPtcls,NumOrbitals);
    dpsiM_2.resize(NumPtcls,NumOrbitals);
    ggM_1.resize(NumPtcls,NumOrbitals);
    ggM_2.resize(NumPtcls,NumOrbitals);
    GGGMatrix_t  ggg_psiM1,ggg_psiM2;
    ggg_psiM1.resize(NumPtcls,NumOrbitals); 
    ggg_psiM2.resize(NumPtcls,NumOrbitals); 

    double dh = 0.000001;
    for(int i=0; i<BFTrans->QP.getTotalNum(); i++) qp_0[i] = BFTrans->QP.R[i];
    Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM,dpsiM,grad_grad_psiM,grad_grad_grad_psiM);

    app_log() <<"Testing GGGType calculation: " <<endl;
    for(int lc=0; lc<3; lc++) { 

      for(int i=0; i<BFTrans->QP.getTotalNum(); i++) BFTrans->QP.R[i] = qp_0[i];
      for(int i=0; i<BFTrans->QP.getTotalNum(); i++) BFTrans->QP.R[i][lc] = qp_0[i][lc] + dh;
      BFTrans->QP.update();
      Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM_1,dpsiM_1,ggM_1,ggg_psiM1);

      for(int i=0; i<BFTrans->QP.getTotalNum(); i++) BFTrans->QP.R[i][lc] = qp_0[i][lc] - dh;
      BFTrans->QP.update();
      Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM_2,dpsiM_2,ggM_2,ggg_psiM2);

      ValueType cnt=0.0,av=0.0,maxD=0.0;
      for(int i=0; i<NumPtcls; i++)
       for(int j=0; j<NumOrbitals; j++)  {
          HessType dG = (ggM_1(i,j)-ggM_2(i,j))/(2.0*dh)-(grad_grad_grad_psiM(i,j))[lc]; 
          for(int la=0; la<9; la++) {
            cnt++;
            av+=dG[la];
            if( std::fabs(dG[la]) > maxD) maxD = std::fabs(dG[la]); 
          }
          app_log() <<i <<"  " <<j <<"\n"
                    <<"dG : \n" <<dG <<endl
                    <<"GGG: \n" <<(grad_grad_grad_psiM(i,j))[lc] <<endl;
       }
      app_log() <<"lc, av, max: " <<lc <<"  " <<av/cnt <<"  "
                <<maxD <<endl;
    }

    for(int i=0; i<BFTrans->QP.getTotalNum(); i++) BFTrans->QP.R[i] = qp_0[i];
    BFTrans->QP.update();
    
  }

  void DiracDeterminantWithBackflow::testDerivFjj(ParticleSet& P, int pa)
  {

       app_log() <<" Testing derivatives of the F matrix, prm: " <<pa <<endl;
       opt_variables_type wfVars,wfvar_prime;
       BFTrans->checkInVariables(wfVars);
       BFTrans->checkOutVariables(wfVars);
       int Nvars= wfVars.size();
       wfvar_prime= wfVars;
       GradMatrix_t dpsiM_1, dpsiM_2, dpsiM_0;
       dpsiM_0.resize(NumPtcls,NumOrbitals); 
       dpsiM_1.resize(NumPtcls,NumOrbitals); 
       dpsiM_2.resize(NumPtcls,NumOrbitals); 
       double dh=0.00001;
       int pr = pa;

       for (int j=0; j<Nvars; j++) wfvar_prime[j]=wfVars[j];
       wfvar_prime[pr] = wfVars[pr]+ dh;   
       BFTrans->resetParameters(wfvar_prime);
       BFTrans->evaluateDerivatives(P);

    Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM,dpsiM,grad_grad_psiM,grad_grad_grad_psiM);

    //std::copy(psiM.begin(),psiM.end(),psiMinv.begin());
    psiMinv=psiM;

    // invert backflow matrix
    InverseTimer.start();
    LogValue=InvertWithLog(psiMinv.data(),NumPtcls,NumOrbitals,WorkSpace.data(),Pivot.data(),PhaseValue);
    InverseTimer.stop();

    // calculate F matrix (gradients wrt bf coordinates)
    // could use dgemv with increments of 3*nCols  
    for(int i=0; i<NumPtcls; i++)
    for(int j=0; j<NumPtcls; j++)
    {
       dpsiM_1(i,j)=dot(psiMinv[i],dpsiM[j],NumOrbitals);
    }

       for (int j=0; j<Nvars; j++) wfvar_prime[j]=wfVars[j];
       wfvar_prime[pr] = wfVars[pr]- dh;  
       BFTrans->resetParameters(wfvar_prime);
       BFTrans->evaluateDerivatives(P);

    Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM,dpsiM,grad_grad_psiM,grad_grad_grad_psiM);

    //std::copy(psiM.begin(),psiM.end(),psiMinv.begin());
    psiMinv=psiM;

    // invert backflow matrix
    InverseTimer.start();
    LogValue=InvertWithLog(psiMinv.data(),NumPtcls,NumOrbitals,WorkSpace.data(),Pivot.data(),PhaseValue);
    InverseTimer.stop();

    // calculate F matrix (gradients wrt bf coordinates)
    // could use dgemv with increments of 3*nCols  
    for(int i=0; i<NumPtcls; i++)
    for(int j=0; j<NumPtcls; j++)
    {
       dpsiM_2(i,j)=dot(psiMinv[i],dpsiM[j],NumOrbitals);
    }

       for (int j=0; j<Nvars; j++) wfvar_prime[j]=wfVars[j];
       BFTrans->resetParameters(wfvar_prime);
       BFTrans->evaluateDerivatives(P);

    Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM,dpsiM,grad_grad_psiM,grad_grad_grad_psiM);

    //std::copy(psiM.begin(),psiM.end(),psiMinv.begin());
    psiMinv=psiM;

    // invert backflow matrix
    InverseTimer.start();
    LogValue=InvertWithLog(psiMinv.data(),NumPtcls,NumOrbitals,WorkSpace.data(),Pivot.data(),PhaseValue);
    InverseTimer.stop();

    // calculate F matrix (gradients wrt bf coordinates)
    // could use dgemv with increments of 3*nCols  
    for(int i=0; i<NumPtcls; i++)
    for(int j=0; j<NumPtcls; j++)
    {
       dpsiM_temp(i,j)=dot(psiMinv[i],dpsiM[j],NumOrbitals);
    }

    double cnt=0,av=0,maxD=0;
    for(int i=0; i<NumPtcls; i++)
    for(int j=0; j<NumPtcls; j++)
    for(int lb=0; lb<3; lb++)
    {
       dpsiM_0(i,j)[lb]=0.0;
       for(int k=0; k<NumPtcls; k++)
       for(int lc=0; lc<3; lc++)  {

           dpsiM_0(i,j)[lb] += (BFTrans->Cmat(pr,FirstIndex+j)[lc]*psiMinv(i,k)*grad_grad_psiM(j,k)(lb,lc) - BFTrans->Cmat(pr,FirstIndex+k)[lc]*dpsiM_temp(i,k)[lc]*dpsiM_temp(k,j)[lb]); 
       }
       cnt++;
       av+=dpsiM_0(i,j)[lb]-( dpsiM_1(i,j)[lb]-dpsiM_2(i,j)[lb] )/(2*dh);
       if( std::fabs(dpsiM_0(i,j)[lb]-( dpsiM_1(i,j)[lb]-dpsiM_2(i,j)[lb] )/(2*dh)) > maxD  ) maxD=dpsiM_0(i,j)[lb]-( dpsiM_1(i,j)[lb]-dpsiM_2(i,j)[lb] )/(2*dh);
    }

    app_log() <<" av,max : " <<av/cnt <<"  " <<maxD <<endl;
   
    //APP_ABORT("testing Fij \n"); 
  }


  void DiracDeterminantWithBackflow::testDerivLi(ParticleSet& P, int pa)
  {

       //app_log() <<"Testing new L(i): \n";
       opt_variables_type wfVars,wfvar_prime;
       BFTrans->checkInVariables(wfVars);
       BFTrans->checkOutVariables(wfVars);
       int Nvars= wfVars.size();
       wfvar_prime= wfVars;
       double dh=0.00001;

       
       //BFTrans->evaluate(P);
       ValueType L1a,L2a,L3a,L0a;
       ValueType L1b,L2b,L3b,L0b;
       ValueType L1c,L2c,L3c,L0c;
       //dummyEvalLi(L1,L2,L3);
       if(myG.size() == 0) myG.resize(P.getTotalNum());
       if(myL.size() == 0) myL.resize(P.getTotalNum());
       myG=0.0;
       myL=0.0;
       //ValueType ps = evaluateLog(P,myG,myL);
       //L0 = Sum(myL);  
       //app_log() <<"L old, L new: " <<L0 <<"  " <<L1+L2+L3 <<endl;

       app_log() <<endl <<" Testing derivatives of L(i) matrix. " <<endl;

       for (int j=0; j<Nvars; j++) wfvar_prime[j]=wfVars[j];
       wfvar_prime[pa] = wfVars[pa]+ dh;
       BFTrans->resetParameters(wfvar_prime);
       BFTrans->evaluate(P);
       dummyEvalLi(L1a,L2a,L3a);

       for (int j=0; j<Nvars; j++) wfvar_prime[j]=wfVars[j];
       wfvar_prime[pa] = wfVars[pa]- dh;
       BFTrans->resetParameters(wfvar_prime);
       BFTrans->evaluate(P);
       dummyEvalLi(L1b,L2b,L3b);

       BFTrans->resetParameters(wfVars);
       BFTrans->evaluateDerivatives(P);

       vector<RealType> dlogpsi;
       vector<RealType> dhpsi;
       dlogpsi.resize(Nvars); 
       dhpsi.resize(Nvars); 
       evaluateDerivatives(P,wfVars,dlogpsi,dhpsi,&myG,&myL,pa); 
       
       app_log() <<"pa: " <<pa <<endl
                 <<"dL Numrical: " 
                 <<(L1a-L1b)/(2*dh) <<"  "  
                 <<(L2a-L2b)/(2*dh) <<"  "  
                 <<(L3a-L3b)/(2*dh) <<"\n"  
                 <<"dL Analitival: " 
                 <<La1 <<"  "  
                 <<La2 <<"  "  
                 <<La3 <<endl
                 <<" dLDiff: "
                 <<(L1a-L1b)/(2*dh)-La1 <<"  "   
                 <<(L2a-L2b)/(2*dh)-La2 <<"  "   
                 <<(L3a-L3b)/(2*dh)-La3 <<endl;

  }


  // evaluate \sum_i L(i) splitted into three pieces
  void DiracDeterminantWithBackflow::dummyEvalLi(ValueType& L1, ValueType& L2, ValueType& L3)
  {
    L1=L2=L3=0.0;

    Phi->evaluate(BFTrans->QP, FirstIndex, LastIndex, psiM,dpsiM,grad_grad_psiM);
    psiMinv=psiM;

    InverseTimer.start();
    LogValue=InvertWithLog(psiMinv.data(),NumPtcls,NumOrbitals,WorkSpace.data(),Pivot.data(),PhaseValue);
    InverseTimer.stop();

    for(int i=0; i<NumPtcls; i++)
    for(int j=0; j<NumPtcls; j++)
    {
       dpsiM_temp(i,j)=dot(psiMinv[i],dpsiM[j],NumOrbitals);
    }
   
    GradType temp;
    ValueType temp2;
    int num = BFTrans->QP.getTotalNum();
    for(int i=0; i<num; i++) {
      for(int j=0; j<NumPtcls; j++) 
        L1 += dot(dpsiM_temp(j,j),BFTrans->Bmat_full(i,FirstIndex+j));
    }


   for(int j=0; j<NumPtcls; j++) {
     HessType q_j = 0.0;
     for(int k=0; k<NumPtcls; k++)  q_j += psiMinv(j,k)*grad_grad_psiM(j,k);    
     HessType a_j = 0.0;
     for(int i=0; i<num; i++) a_j += dot(transpose(BFTrans->Amat(i,FirstIndex+j)),BFTrans->Amat(i,FirstIndex+j));     

     L2 += traceAtB(a_j,q_j); 

     for(int k=0; k<NumPtcls; k++) {
       HessType a_jk = 0.0;
       for(int i=0; i<num; i++) a_jk += dot(transpose(BFTrans->Amat(i,FirstIndex+j)),BFTrans->Amat(i,FirstIndex+k));
       
       L3 -= traceAtB(a_jk, outerProduct(dpsiM_temp(k,j),dpsiM_temp(j,k))); 
     }
   }

  }

}
/***************************************************************************
 * $RCSfile$   $Author: jeongnim.kim $
 * $Revision: 4710 $   $Date: 2010-03-07 18:05:22 -0600 (Sun, 07 Mar 2010) $
 * $Id: DiracDeterminantWithBackflow.cpp 4710 2010-03-08 00:05:22Z jeongnim.kim $ 
 ***************************************************************************/
