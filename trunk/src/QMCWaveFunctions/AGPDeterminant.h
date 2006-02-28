//////////////////////////////////////////////////////////////////
// (c) Copyright 2006- by Jeongnim Kim
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
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#ifndef QMCPLUSPLUS_AGP_DIRACDETERMINANT_H
#define QMCPLUSPLUS_AGP_DIRACDETERMINANT_H
#include "QMCWaveFunctions/OrbitalBase.h"
#include "OhmmsPETE/OhmmsMatrix.h"
#include "QMCWaveFunctions/MolecularOrbitals/GridMolecularOrbitals.h"

namespace qmcplusplus {

  class AGPDeterminant: public OrbitalBase {

  public:

    typedef GridMolecularOrbitals::BasisSetType BasisSetType;
    typedef Matrix<ValueType> Determinant_t;
    typedef Matrix<GradType>  Gradient_t;
    typedef Matrix<ValueType> Laplacian_t;

    BasisSetType* BasisSet;

    /** constructor
     *@param spos the single-particle orbital set
     *@param first index of the first particle
     */
    AGPDeterminant(BasisSetType* bs=0);

    ///default destructor
    ~AGPDeterminant();
  
    ///reset the single-particle orbital set
    void reset() { BasisSet->reset(); }
   
    void resetTargetParticleSet(ParticleSet& P) { 
      BasisSet->resetTargetParticleSet(P);
    }

    ///reset the size: with the number of particles and number of orbtials
    inline void resize(int nup, int ndown);

    ValueType registerData(ParticleSet& P, PooledData<RealType>& buf);

    ValueType updateBuffer(ParticleSet& P, PooledData<RealType>& buf);

    void copyFromBuffer(ParticleSet& P, PooledData<RealType>& buf);

    /** dump the inverse to the buffer
     */
    inline void dumpToBuffer(ParticleSet& P, PooledData<RealType>& buf) {
      buf.add(psiM.begin(),psiM.end());
    }

    /** copy the inverse from the buffer
     */
    inline void dumpFromBuffer(ParticleSet& P, PooledData<RealType>& buf) {
      buf.get(psiM.begin(),psiM.end());
    }

    /** return the ratio only for the  iat-th partcle move
     * @param P current configuration
     * @param iat the particle thas is being moved
     */
    ValueType ratio(ParticleSet& P, int iat);

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


    ValueType logRatio(ParticleSet& P, int iat,
		    ParticleSet::ParticleGradient_t& dG, 
		    ParticleSet::ParticleLaplacian_t& dL) {
      ValueType r=ratio(P,iat,dG,dL);
      SignValue = (r<0.0)? -1.0: 1.0;
      return log(abs(r));
    }


    /** move was accepted, update the real container
     */
    void update(ParticleSet& P, int iat);

    /** move was rejected. copy the real container to the temporary to move on
     */
    void restore(int iat);

    
    void update(ParticleSet& P, 
		ParticleSet::ParticleGradient_t& dG, 
		ParticleSet::ParticleLaplacian_t& dL,
		int iat);


    ValueType evaluate(ParticleSet& P, PooledData<RealType>& buf);


    void resizeByWalkers(int nwalkers);

    ///evaluate log of determinant for a particle set: should not be called 
    ValueType
    evaluateLog(ParticleSet& P, 
	        ParticleSet::ParticleGradient_t& G, 
	        ParticleSet::ParticleLaplacian_t& L) {
      ValueType psi=evaluate(P,G,L);
      SignValue = (psi<0.0)?-1.0:1.0;
      return LogValue = log(abs(psi));
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
    ValueType
    evaluate(ParticleSet& P, 
	     ParticleSet::ParticleGradient_t& G, 
	     ParticleSet::ParticleLaplacian_t& L);


    ///Total number of particles
    int NumPtcls;
    ///number of major spins
    int Nup;
    ///number of minor spins
    int Ndown;
    ///size of the basis set
    int BasisSize;

    ///index of the particle (or row) 
    int WorkingIndex;      

    ///Current determinant value
    ValueType CurrentDet;

    ///coefficient of the up/down block
    Determinant_t Coeff;

    ///coefficient of the major block
    Vector<ValueType> CoeffMajor;

    /// psiM(j,i) \f$= \psi_j({\bf r}_i)\f$
    Determinant_t psiM, psiM_temp;

    /**  Transient data for gradient and laplacian evaluation
     *
     * \f$phiU(j,k) = \sum_{j^{'}} \lambda_{j,j^{'}} \phi_k(r_j) \f$
     * j denotes the spin-up index
     */
    Determinant_t phiU;

    /**  Transient data for gradient and laplacian evaluation
     *
     * \f$phiD(j,k) = \sum_{j^{'}} \lambda_{j^{'},j} \phi_k(r_j) \f$
     * j runs over the spin-down index
     */
    Determinant_t phiD;

    /// temporary container for testing
    Determinant_t psiMinv;

    /// dpsiM(i,j) \f$= \nabla_i \psi_j({\bf r}_i)\f$
    Gradient_t    dpsiM, dpsiM_temp;

    /// d2psiM(i,j) \f$= \nabla_i^2 \psi_j({\bf r}_i)\f$
    Laplacian_t   d2psiM, d2psiM_temp;

    /// value of single-particle orbital for particle-by-particle update
    std::vector<ValueType> psiV;
    std::vector<GradType> dpsiV;
    std::vector<ValueType> d2psiV;
    std::vector<ValueType> workV1, workV2;

    ///storages to process many walkers once
    vector<Determinant_t> psiM_v; 
    vector<Gradient_t>    dpsiM_v; 
    vector<Laplacian_t>   d2psiM_v; 

    Vector<ValueType> WorkSpace;
    Vector<IndexType> Pivot;

    ValueType curRatio,cumRatio;
    ValueType *FirstAddressOfG;
    ValueType *LastAddressOfG;
    ValueType *FirstAddressOfdV;
    ValueType *LastAddressOfdV;

    ParticleSet::ParticleGradient_t myG, myG_temp;
    ParticleSet::ParticleLaplacian_t myL, myL_temp;
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
