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
#ifndef OHMMS_QMC_DMC_MOLECU_H
#define OHMMS_QMC_DMC_MOLECU_H

#include "QMCDrivers/QMCDriver.h" 

namespace ohmmsqmc {

  /** Implements the DMC algorithm. */
  class MolecuDMC: public QMCDriver {

  public:
    /// Constructor.
    MolecuDMC(MCWalkerConfiguration& w, TrialWaveFunction& psi, QMCHamiltonian& h);

    template<class BRANCHER>
    void advanceWalkerByWalker(BRANCHER& Branch);

    bool run();
    bool put(xmlNodePtr q);

    void setBranchInfo(const string& fname);

  private:

    ///hdf5 file name for Branch conditions
    std::string BranchInfo;
    /// Copy Constructor (disabled)
    MolecuDMC(const MolecuDMC& a): QMCDriver(a) { }
    /// Copy operator (disabled).
    MolecuDMC& operator=(const MolecuDMC&) { return *this;}
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
