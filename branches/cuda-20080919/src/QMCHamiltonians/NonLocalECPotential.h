//////////////////////////////////////////////////////////////////
// (c) Copyright 2005- by Jeongnim Kim and Simone Chiesa
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
#ifndef QMCPLUSPLUS_NONLOCAL_ECPOTENTIAL_H
#define QMCPLUSPLUS_NONLOCAL_ECPOTENTIAL_H
#include "QMCHamiltonians/NonLocalECPComponent.h"

namespace qmcplusplus {

  /** @ingroup hamiltonian
   * \brief Evaluate the semi local potentials
   */
  struct NonLocalECPotential: public QMCHamiltonianBase {

    int NumIons;
    ///the distance table containing electron-nuclei distances  
    DistanceTableData* d_table;
    ///the set of local-potentials (one for each ion)
    vector<NonLocalECPComponent*> PP;
    ///unique NonLocalECPComponent to remove
    vector<NonLocalECPComponent*> PPset;
    ///reference to the center ion
    ParticleSet& IonConfig;
    ///target TrialWaveFunction
    TrialWaveFunction& Psi;

    NonLocalECPotential(ParticleSet& ions, ParticleSet& els, TrialWaveFunction& psi);

    ~NonLocalECPotential();

    void resetTargetParticleSet(ParticleSet& P);

    Return_t evaluate(ParticleSet& P);

    Return_t evaluate(ParticleSet& P, vector<NonLocalData>& Txy);

    /** Do nothing */
    bool put(xmlNodePtr cur) { return true; }

    bool get(std::ostream& os) const {
      os << "NonLocalECPotential: " << IonConfig.getName();
      return true;
    }

    QMCHamiltonianBase* makeClone(ParticleSet& qp, TrialWaveFunction& psi);

    void add(int groupID, NonLocalECPComponent* pp);

    void setRandomGenerator(RandomGenerator_t* rng);

    //////////////////////////////////
    // Vectorized evaluation on GPU //
    //////////////////////////////////
    int NumIonGroups;
    vector<int> IonFirst, IonLast;
    cuda_vector<CUDA_PRECISION> Ions_GPU, L, Linv;
    host_vector<CUDA_PRECISION> R_host;
    cuda_vector<CUDA_PRECISION> R_GPU;
    host_vector<CUDA_PRECISION*> Rlist_host;
    cuda_vector<CUDA_PRECISION*> Rlist_GPU;
    cuda_vector<int2> Pairs_GPU;
    cuda_vector<CUDA_PRECISION> Dist_GPU;
    cuda_vector<int2*> Pairlist_GPU;
    cuda_vector<CUDA_PRECISION*> Distlist_GPU;
    cuda_vector<int> NumPairs_GPU;
    int NumElecs;
    // The maximum number of quadrature points over all the ions species
    int MaxKnots, MaxPairs;
    // These are the positions at which we have to evalate the WF ratios
    // It has size OHMMS_DIM * MaxPairs * MaxKnots * NumWalkers
    cuda_vector<CUDA_PRECISION> RatioPos_GPU;
    cuda_vector<CUDA_PRECISION> Ratios_GPU;
    cuda_vector<CUDA_PRECISION*> RatioPoslist_GPU, Ratiolist_GPU;

    // Quadrature points
    vector<cuda_vector<CUDA_PRECISION> > QuadPoints_GPU;
    vector<host_vector<CUDA_PRECISION> > QuadPoints_host;

    void setupCuda(ParticleSet &elecs);
    void addEnergy(vector<Walker_t*> &walkers, vector<RealType> &LocalEnergy);

  };
}
#endif

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

