//////////////////////////////////////////////////////////////////
// (c) Copyright 2003  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
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
#ifndef QMCPLUSPLUS_COULOMBPBC_H
#define QMCPLUSPLUS_COULOMBPBC_H
#if (__GNUC__ == 2)
#include <algo.h>
#else
#include <algorithm>
#endif
#include "Particle/ParticleSet.h"
#include "Particle/WalkerSetRef.h"
#include "Particle/DistanceTableData.h"
#include "QMCHamiltonians/QMCHamiltonianBase.h"

//Long-range includes.
#include "LongRange/LRCoulombAA.h"
#include "LongRange/LRCoulombAB.h"
#include "LongRange/LPQHIBasis.h"

namespace qmcplusplus {

  /** @ingroup hamiltonian
   *\brief Calculates the AA Coulomb potential using PBCs
   */

  struct CoulombPBCAA: public QMCHamiltonianBase {

    ParticleSet* PtclRef;
    LRCoulombAA<LPQHIBasis>* AA;
    bool FirstTime;
    
    CoulombPBCAA(ParticleSet& ref): FirstTime(true), PtclRef(&ref), AA(0) {
      LOGMSG("Performing long-range breakup for CoulombAA potential");
      AA = new LRCoulombAA<LPQHIBasis>(*PtclRef);
    }
    
    ~CoulombPBCAA() { 
      if(AA) delete AA;
    }

    void resetTargetParticleSet(ParticleSet& P) {
      //Update the internal particleref
      PtclRef = &P;
      //Update the particleref in AA.
      if(AA)AA->resetTargetParticleSet(P);
    }

    inline Return_t evaluate(ParticleSet& P) {  
      //Ions don't move usually, so P will be electrons.
      //Then we can avoid repeating i-i potential, and do only once, by
      //using FirstTime flag.
      if(FirstTime || PtclRef->tag() == P.tag()){
        Value = AA->evalTotal();
	FirstTime = false;
      }
      return Value;
    }

    /** Do nothing */
    bool put(xmlNodePtr cur) {
      return true;
    }

    bool get(std::ostream& os) const {
      os << "CoulomPBCAA potential: " << PtclRef->getName();
      return true;
    }
  };


  /** @ingroup hamiltonian
   *\brief Calculates the AB Coulomb potential using PBCs
   */

  struct CoulombPBCAB: public QMCHamiltonianBase {

    ParticleSet *PtclIons, *PtclElns;
    LRCoulombAB<LPQHIBasis>* AB;
    
    CoulombPBCAB(ParticleSet& ions,ParticleSet& elns): PtclIons(&ions), PtclElns(&elns), AB(0) {
      LOGMSG("Performing long-range breakup for CoulombAB potential");
      AB = new LRCoulombAB<LPQHIBasis>(*PtclIons,*PtclElns);
    }
    
    ~CoulombPBCAB() { 
      if(AB)delete AB;
    }

    void resetTargetParticleSet(ParticleSet& P) {
      //update the pointer of the target particleset (electrons) 
      PtclElns = &P;
      //Update the particleref in AB.
      if(AB)AB->resetTargetParticleSet(P);
    }

    inline Return_t evaluate(ParticleSet& P) {  
      Value = AB->evalTotal();
      return Value;
    }

    /** Do nothing */
    bool put(xmlNodePtr cur) {
      return true;
    }

    bool get(std::ostream& os) const {
      os << "CoulomPBCAB potential: " 
        << " source = " << PtclIons->getName() 
        << " target = " << PtclElns->getName();
      return true;
    }
  };
}
#endif

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

