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
/**@file HamiltonianPool.h
 * @brief Declaration of HamiltonianPool
 */
#ifndef QMCPLUSPLUS_QMCHAMILTONIANS_H
#define QMCPLUSPLUS_QMCHAMILTONIANS_H

#include "QMCHamiltonians/HamiltonianFactory.h"
#include "OhmmsData/OhmmsElementBase.h"
#include <map>

namespace qmcplusplus {

  class ParticleSet;
  class MCWalkerConfiguration;
  class ParticleSetPool;
  class WaveFunctionPool;
  class Libxml2Document;

  /** @ingroup qmcapp
   * @brief Manage a collection of QMCHamiltonian objects
   *
   * This object handles \<hamiltonian\> elements and
   * functions as a builder class for QMCHamiltonian objects.
   */
  class HamiltonianPool : public OhmmsElementBase {

  public:

    typedef std::map<std::string,HamiltonianFactory*> PoolType;

    HamiltonianPool(const char* aname = "hamiltonian");

    bool get(std::ostream& os) const;
    bool put(std::istream& is);
    bool put(xmlNodePtr cur);
    void reset();

    inline bool empty() const { return myPool.empty();}

    /** return the pointer to the primary QMCHamiltonian
     *
     * The first QMCHamiltonian is assigned to the primaryH.
     * The last QMCHamiltonian with role="primary" will be the primaryH.
     */
    inline QMCHamiltonian* getPrimary() {
      return primaryH;
    }

    /** return the pointer to a QMCHamiltonian with the name 
     * @param pname name of the QMCHamiltonian
     */
    inline QMCHamiltonian* getHamiltonian(const std::string& pname) {
      PoolType::iterator hit(myPool.find(pname));
      if(hit == myPool.end())  {
        if(myPool.empty())
          return 0;
        else
          return (*(myPool.begin())).second->targetH;
      }
      else 
        return (*hit).second->targetH;
    }

    void setDocument(Libxml2Document* doc) {
      curDoc=doc;
    }

    /** assign a pointer to a ParticleSetPool
     */
    inline void setParticleSetPool(ParticleSetPool* pset) { ptclPool=pset;}

    /** assign a pointer to a WaveFunctionPool
     */
    inline void setWaveFunctionPool(WaveFunctionPool* pset) { psiPool=pset;}

    /** clone P/Psi/H 
     * @param qp original ParticleSet to be cloned
     * @param psi original TrialWaveFunction to be cloned
     * @param h original QuantumHamiltonian to be cloned
     * @param plist array containing the original and cloned ParticleSets
     * @param olist array containing the original and cloned TrialWaveFunctions
     * @param hlist array containing the original and cloned QMCHamiltonians
     */
    void clone(const MCWalkerConfiguration& qp, const TrialWaveFunction& psi, const QMCHamiltonian& h,
        vector<MCWalkerConfiguration*>& plist, vector<TrialWaveFunction*>& olist, 
        vector<QMCHamiltonian*>& hlist);


  private:

    /** pointer to the primary QMCHamiltonian
     */
    QMCHamiltonian* primaryH;

    /** pointer to a current QMCHamiltonian to be built.
     */
    QMCHamiltonian* curH;

    /** pointer to ParticleSetPool
     *
     * QMCHamiltonian needs to know which ParticleSet object
     * is used as an input object for the evaluations. 
     * Any number of ParticleSet can be used to describe
     * a QMCHamiltonian.
     */
    ParticleSetPool* ptclPool;

    /** pointer to WaveFunctionPool
     *
     * For those QMCHamiltonianBase that depends on TrialWaveFunction,
     * e.g., NonLocalPPotential.
     */
    WaveFunctionPool* psiPool;


    /** point to the working document */
    Libxml2Document* curDoc;

    /** storage for HamiltonianFactory */
    PoolType myPool;
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
