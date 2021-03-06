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
#ifndef QMCPLUSPLUS_REPMULTIWARP_H
#define QMCPLUSPLUS_REPMULTIWARP_H

#include "QMCDrivers/QMCDriver.h" 
#include "OhmmsPETE/OhmmsVector.h" 
#include "QMCDrivers/SpaceWarp.h" 

namespace qmcplusplus {

  class Bead;
  class MultiChain;
  class ParticleSetPool;
  class CSPolymerEstimator;

  /** @ingroup QMCDrivers MultiplePsi
   * @brief Implements the RMC algorithm for energy differences
   */
  class RQMCMultiWarp: public QMCDriver {
  
    typedef MCWalkerConfiguration::ParticlePos_t ParticlePos_t;


  public:

    /// Constructor.
    RQMCMultiWarp(MCWalkerConfiguration& w, TrialWaveFunction& psi, QMCHamiltonian& h,
        ParticleSetPool& ptclPool);

    /// Destructor
    ~RQMCMultiWarp();

    bool run();
    bool put(xmlNodePtr q);

  protected:
    ParticleSetPool& PtclPool;

    /** boolean for initialization
     *
     *\if true,
     *use clones for a chain.
     *\else
     *use drift-diffusion to form a chain
     *\endif
     */

    ///The length of polymers
    int ReptileLength;

    ///
    int MinusDirection,PlusDirection,Directionless;
    ///
    int forward,backward,itail,inext;

    ///the number of turns per block
    int NumTurns;

    int nptcl;

    ///the number of H/Psi pairs
    int nPsi;

    //index of the log-jacobian in Properties
    int LOGJACOB;

    string refSetName;

    vector<RealType> Jacobian;

    SpaceWarp PtclWarp;

    ///The Reptile: a chain of beads
    MultiChain* Reptile;

    ///The new bead
    Bead *NewBead;

    //Warped position of head bead
    vector<ParticlePos_t> Warped_R,Warped_deltaR;

    //Auxiliary array to compute properties of freshly warped ptcls
    vector<ParticleSet*> WW;

    ///move polymers
    void moveReptile();

    ///initialize polymers
    void initReptile();

    ///for the first run starting with a point, set reference properties (sign)
    void setReptileProperties();

    ///for the first run starting with a point, set reference properties (sign)
    void checkReptileProperties();

    ///Working arrays
    Vector<RealType> NewGlobalAction,DeltaG;
    Vector<int>NewGlobalSignWgt,WeightSign;
    
    void resizeArrays(int n);

    ///overwrite recordBlock
    void recordBlock(int block);

  private:

    /// Copy Constructor (disabled)
    RQMCMultiWarp(const RQMCMultiWarp& a, ParticleSetPool& ptclPool): 
      QMCDriver(a), PtclPool(ptclPool) { }

    /// Copy operator (disabled).
    RQMCMultiWarp& operator=(const RQMCMultiWarp&) { return *this;}

    ParticleSet::ParticlePos_t gRand;

  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
