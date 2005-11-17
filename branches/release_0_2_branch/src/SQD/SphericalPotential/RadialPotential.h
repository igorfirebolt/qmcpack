//////////////////////////////////////////////////////////////////
// (c) Copyright 2003  by Jeongnim Kim and Jordan Vincent
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
#ifndef OHMMSHF_RADIALPOTENTIALBASE_H
#define OHMMSHF_RADIALPOTENTIALBASE_H

#include "SQD/HFConfiguration.h"
#include "OhmmsData/libxmldefs.h"

class Clebsch_Gordan;
/**
 *@defgroup RadialPotential
 *@brief Classes to define potentials on a radial grid.
 *
 *A radial potential is defined on a radial grid, 
 *OneDimGridBase<T> and RadialPotentialSet are used to represent 
 *the LHS of Radial Schrodinger Equation.
 */
namespace ohmmshf {

  /**
   *@ingroup RadialPotential
   *@class RadialPotentialBase
   *@brief An abstract base class for a Radial Potential.
   *
   *Inherited classes implement the member function evaluate 
   *to calculate matrix elements for a Hamiltonian term.
   */
  struct RadialPotentialBase {

    typedef SphericalOrbitalTraits::BasisSetType       BasisSetType;
    typedef SphericalOrbitalTraits::value_type         value_type;
    typedef SphericalOrbitalTraits::RadialGrid_t       RadialGrid_t;
    typedef SphericalOrbitalTraits::RadialOrbital_t    RadialOrbital_t;
    typedef SphericalOrbitalTraits::RadialOrbitalSet_t RadialOrbitalSet_t;

    ///lower-bound for eigenvalues
    value_type MinEigenValue;

    ///upper-bound for eigenvalues
    value_type MaxEigenValue;

    ///storage for an external potential
    RadialOrbital_t* Vext;

    ///constructor
    RadialPotentialBase():MinEigenValue(0.0), MaxEigenValue(0.0), Vext(NULL) {}

    ///destructor
    virtual ~RadialPotentialBase() { }

    /**
     *@param psi the wavefunction
     *@param V the potential
     *@param norb the number of orbitals
     *@return The sum of the matrix elements of a Radial Potential \f$V(r)\f$
     *@brief Calculates and assigns the values of a Randial Potential for
     *each orbital.
     *
     \f[
     \sum_{k=0}^{N_{orb}} \langle k|V(r)|k \rangle = \sum_{k=0}^{N_{orb}}
     \int_0^{\infty} dr \: \psi_k^*(r)V(r)\psi_k(r) 
     \f]
    */
    virtual 
    value_type evaluate(const BasisSetType& psi, 
			RadialOrbitalSet_t& V, 
			int norb) = 0; 

    /**
     *@param n the principal quantum number
     *@param l the angular quantum number
     *@return the number of radial nodes 
     */
    virtual int getNumOfNodes(int n, int l)=0;

    /**
     *@param RootFileName the name of file root
     *@brief Output the internal storage of the potential.
     *@note Only applies for the Hartree and Exchange potentials.
     */
    virtual void getStorage(const BasisSetType& psi, 
			    const std::string& RootFileName) { return; }

    /**
     *@param ig the grid index
     *@return the value of the external potential
    */
    inline
    value_type getV(int ig) const {
      return (*Vext)(ig);
    }


    /**@return the pointer of the data **/
    inline value_type* data() const {
      if(Vext) 
	return Vext->data();
      else
	return NULL;
    }

    /**
     *@param cur the current xml node to process
     *@return true if the input is valid
     */
    virtual bool put(xmlNodePtr cur) { return true;}

  };

  /**
   *@ingroup RadialPotential
   *@class HartreePotential
   *@brief Implements the Hartree potential.
  */
  struct HartreePotential: public RadialPotentialBase {
    ///the Clebsch Gordan coefficient matrix
    Clebsch_Gordan *CG_coeff;
    /**store the matrix elements 
       \f$\langle \psi_i \psi_j |V| \psi_i \psi_j \rangle \f$ */
    Vector<value_type> storage;
    HartreePotential(Clebsch_Gordan* cg, int norb);
    value_type evaluate(const BasisSetType& psi, 
			RadialOrbitalSet_t& V, int norb);
    void getStorage(const BasisSetType& psi, 
		    const std::string& RootFileName);
    int getNumOfNodes(int n, int l) {return 0;}
  };

  /**
   *@ingroup RadialPotential
   *@class ExchangePotential
   *@brief Implements the exchange potential
   */
  struct ExchangePotential: public RadialPotentialBase {
    ///the Clebsch Gordan coefficient matrix
    Clebsch_Gordan *CG_coeff;
    /**store the matrix elements 
       \f$\langle \psi_i \psi_j |V| \psi_j \psi_i \rangle \f$ */
    Vector<value_type> storage;
    ExchangePotential(Clebsch_Gordan* cg, int norb);
    value_type evaluate(const BasisSetType& psi, 
			RadialOrbitalSet_t& V, int norb);
    void getStorage(const BasisSetType& psi, 
		    const std::string& RootFileName);
    int getNumOfNodes(int n, int l) {return 0;}
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

  
