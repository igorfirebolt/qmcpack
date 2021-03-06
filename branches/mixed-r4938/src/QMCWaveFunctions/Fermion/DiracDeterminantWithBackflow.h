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
/**@file DiracDeterminantWithBackflowBase.h
 * @brief Declaration of DiracDeterminantWithBackflow with a S(ingle)P(article)O(rbital)SetBase
 */
#ifndef QMCPLUSPLUS_DIRACDETERMINANTWITHBACKFLOW_H
#define QMCPLUSPLUS_DIRACDETERMINANTWITHBACKFLOW_H
#include "QMCWaveFunctions/OrbitalBase.h"
#include "QMCWaveFunctions/SPOSetBase.h"
#include "Utilities/NewTimer.h"
#include "QMCWaveFunctions/Fermion/BackflowTransformation.h"
#include "QMCWaveFunctions/Fermion/DiracDeterminantBase.h"
#include "OhmmsPETE/OhmmsArray.h"

namespace qmcplusplus
  {

  /** class to handle determinants with backflow
   */
  class DiracDeterminantWithBackflow: public DiracDeterminantBase 
    {
    public:

      typedef SPOSetBase::IndexVector_t IndexVector_t;
      typedef SPOSetBase::ValueVector_t ValueVector_t;
      typedef SPOSetBase::ValueMatrix_t ValueMatrix_t;
      typedef SPOSetBase::GradVector_t  GradVector_t;
      typedef SPOSetBase::GradMatrix_t  GradMatrix_t;
      typedef SPOSetBase::HessMatrix_t  HessMatrix_t;
      typedef OrbitalSetTraits<ValueType>::HessVector_t  HessVector_t;
      typedef SPOSetBase::HessType      HessType;
      typedef TinyVector<HessType, 3>   GGGType;
      typedef Vector<GGGType>           GGGVector_t;           
      typedef Matrix<GGGType>           GGGMatrix_t;           
      typedef Array<HessType,3>         HessArray_t;
      //typedef Array<GradType,3>       GradArray_t;
      //typedef Array<PosType,3>        PosArray_t;

      /** constructor
       *@param spos the single-particle orbital set
       *@param first index of the first particle
       */
      DiracDeterminantWithBackflow(SPOSetBasePtr const &spos, BackflowTransformation * BF, int first=0);

      ///default destructor
      ~DiracDeterminantWithBackflow();

      /**copy constructor
       * @param s existing DiracDeterminantWithBackflow
       *
       * This constructor makes a shallow copy of Phi.
       * Other data members are allocated properly.
       */
      DiracDeterminantWithBackflow(const DiracDeterminantWithBackflow& s);

      DiracDeterminantWithBackflow& operator=(const DiracDeterminantWithBackflow& s);

      ///** return a clone of Phi
      // */
      //SPOSetBasePtr clonePhi() const;

      // in general, assume that P is the quasiparticle set
      void evaluateDerivatives(ParticleSet& P,
				       const opt_variables_type& active,
				       vector<RealType>& dlogpsi,
				       vector<RealType>& dhpsioverpsi);

      void evaluateDerivatives(ParticleSet& P,
                                       const opt_variables_type& active,
                                       vector<RealType>& dlogpsi,
                                       vector<RealType>& dhpsioverpsi,
                                       ParticleSet::ParticleGradient_t* G0,
                                       ParticleSet::ParticleLaplacian_t* L0,
                                       int k);

      ///reset the size: with the number of particles and number of orbtials
      void resize(int nel, int morb);

      RealType registerData(ParticleSet& P, PooledData<RealType>& buf);

      RealType updateBuffer(ParticleSet& P, PooledData<RealType>& buf, bool fromscratch=false);

      void copyFromBuffer(ParticleSet& P, PooledData<RealType>& buf);

      /** return the ratio only for the  iat-th partcle move
       * @param P current configuration
       * @param iat the particle thas is being moved
       */
      ValueType ratio(ParticleSet& P, int iat);

      void get_ratios(ParticleSet& P, vector<ValueType>& ratios);

      ValueType alternateRatio(ParticleSet& P)
      {
        return 1.0;
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
      ValueType ratio(ParticleSet& P, int iat,
                              ParticleSet::ParticleGradient_t& dG,
                              ParticleSet::ParticleLaplacian_t& dL);

      ValueType ratioGrad(ParticleSet& P, int iat, GradType& grad_iat);
      GradType evalGrad(ParticleSet& P, int iat);
      GradType evalGradSource(ParticleSet &P, ParticleSet &source,
                                      int iat);

      GradType evalGradSource
      (ParticleSet& P, ParticleSet& source, int iat,
       TinyVector<ParticleSet::ParticleGradient_t, OHMMS_DIM> &grad_grad,
       TinyVector<ParticleSet::ParticleLaplacian_t,OHMMS_DIM> &lapl_grad);

      GradType evalGradSourcep
      (ParticleSet& P, ParticleSet& source, int iat,
       TinyVector<ParticleSet::ParticleGradient_t, OHMMS_DIM> &grad_grad,
       TinyVector<ParticleSet::ParticleLaplacian_t,OHMMS_DIM> &lapl_grad);

      ValueType logRatio(ParticleSet& P, int iat,
                                 ParticleSet::ParticleGradient_t& dG,
                                 ParticleSet::ParticleLaplacian_t& dL);

      /** move was accepted, update the real container
       */
      void acceptMove(ParticleSet& P, int iat);

      /** move was rejected. copy the real container to the temporary to move on
       */
      void restore(int iat);

      void update(ParticleSet& P,
                          ParticleSet::ParticleGradient_t& dG,
                          ParticleSet::ParticleLaplacian_t& dL,
                          int iat);

      RealType evaluateLog(ParticleSet& P, PooledData<RealType>& buf);


      RealType
      evaluateLog(ParticleSet& P,
                  ParticleSet::ParticleGradient_t& G,
                  ParticleSet::ParticleLaplacian_t& L) ;

      ValueType
      evaluate(ParticleSet& P,
               ParticleSet::ParticleGradient_t& G,
               ParticleSet::ParticleLaplacian_t& L);

      OrbitalBasePtr makeClone(ParticleSet& tqp) const;

      /** cloning function 
       * @param tqp target particleset
       * @param spo spo set
       *
       * This interface is exposed only to SlaterDet and its derived classes
       * can overwrite to clone itself correctly.
       */
      DiracDeterminantWithBackflow* makeCopy(SPOSetBase* spo) const;

      inline void setLogEpsilon(ValueType x) { }

      GradMatrix_t dFa; 
      HessMatrix_t grad_grad_psiM; 
      HessVector_t grad_gradV; 
      HessMatrix_t grad_grad_psiM_temp; 
      GGGMatrix_t  grad_grad_grad_psiM; 
      BackflowTransformation *BFTrans;
      ParticleSet::ParticleGradient_t Gtemp;
      ValueType La1,La2,La3;
      HessMatrix_t Ajk_sum,Qmat;
      GradMatrix_t Fmat;
      GradVector_t Fmatdiag;
      GradVector_t Fmatdiag_temp;

      ValueMatrix_t psiMinv_temp;
      ValueType *FirstAddressOfGGG;
      ValueType *LastAddressOfGGG;
      ValueType *FirstAddressOfFm;
      ValueType *LastAddressOfFm;

      void testDerivFjj(ParticleSet& P, int pa);
      void testGGG(ParticleSet& P);
      void testDerivLi(ParticleSet& P, int pa);
      void dummyEvalLi(ValueType& L1, ValueType& L2, ValueType& L3);

    };



}
#endif
