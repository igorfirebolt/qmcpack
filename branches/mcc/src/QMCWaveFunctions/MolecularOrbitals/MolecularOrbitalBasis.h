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
/*! \author Jeongnim Kim
 */
#ifndef OHMMS_QMC_MOLECULARORBITALBASIS_H
#define OHMMS_QMC_MOLECULARORBITALBASIS_H

#include "OhmmsPETE/OhmmsMatrix.h"

namespace ohmmsqmc {

  class DistanceTableData;

  /** class for molecular orbital basis
   *
   *The molecular orbital \f$ \psi_i \f$ can be written as a linear
   *combination of basis functions \f$ \{\phi\} \f$ such that
   \f[
   \psi_i ({\bf r}_j) = \sum_I \sum_k C_{ikI} \phi_{ikI}({\bf r}_j-{\bf R}_I).
   \f]
   *This class performs the evaluation of the basis functions and their
   *derivatives for each of the N-particles in a configuration.  All that 
   *is required to generate the actual molecular orbitals is to multiply
   *by the coefficient matrix.
   */
  template<class COT>
  class MolecularOrbitalBasis: public QMCTraits {
  public:

    ///constructor
    MolecularOrbitalBasis(): myTable(NULL){ }
  
    /**
     @param atable the distance table (ion-electron)
     @brief Assign the distance table (ion-electron) and
     *determine the total number of basis states.
    */
    void setTable(DistanceTableData* atable) { 

      myTable = atable;
      //first set up the center list
      const ParticleSet& ptcl_ref = myTable->origin();
      I.resize(myTable->centers());
      for(int ic=0; ic<I.size(); ic++) { 
	I[ic] = ptcl_ref.GroupID[ic];
      }
      //reset the distance table for the atomic orbitals
      for(int i=0; i<AOs.size(); i++) AOs[i]->reset(myTable);
      //evaluate the total basis dimension and offset for each center
      Basis.resize(I.size()+1);
      Basis[0] = 0;
      for(int c=0; c<I.size(); c++){
	Basis[c+1] = Basis[c]+AOs[I[c]]->basis();
      }
      TotalBasis = Basis[I.size()];
    }

    /**
       @param nptcl number of particles
       @brief resize the containers for data 
       for nptcl particles and TotalBasis basis functions
    */
    inline void resize(int nptcl) {
      NumPtcls = nptcl;
      Y.resize(nptcl,TotalBasis);
      dY.resize(nptcl,TotalBasis);
      d2Y.resize(nptcl,TotalBasis);
    }


    inline void resizeByWalkers(int nw) {
      int n = NumPtcls*nw;
      NumWalkers = nw;
      Y.resize(n,TotalBasis);
      dY.resize(n,TotalBasis);
      d2Y.resize(n,TotalBasis);
    }

    /**
       @param P input configuration containing N particles
       @brief For each center, evaluate all the Atomic Orbitals
       belonging to that center.  Fill in the matrices:
       \f[ Y[i,j] =  \phi_j(r_i-R_I) \f]
       \f[ dY[i,j] = {\bf \nabla}_i \phi_j(r_i-R_I) \f]
       \f[ d2Y[i,j] = \nabla^2_i \phi_j(r_i-R_I), \f]
       where \f$ {\bf R_I} \f$ is the correct center for the 
       basis function \f$ \phi_j \f$
    */
    inline void 
    evaluate(const ParticleSet& P) {
      for(int c=0; c<I.size();c++) {
	AOs[I[c]]->evaluate(c,0,P.getTotalNum(),Basis[c],Y,dY,d2Y);
      }
    }

    /**evaluate \f$ \phi_I^J ({\bf R}_i-{\bf R}_I) \f$
     *
     *calling SphericalOrbitals::evaluateW
     */
    inline void 
    evaluate(const WalkerSetRef& W) {
      for(int c=0; c<I.size();c++) {
	AOs[I[c]]->evaluateW(c,0,W.particles(),Basis[c],
			     W.walkers(),NumPtcls,
			     Y,dY,d2Y);
      }
    }

    /**
     *@param aos a set of Centered Atomic Orbitals
     *@brief add a new set of Centered Atomic Orbitals
     */
    inline void add(COT* aos) {
      AOs.push_back(aos);
    }

    void print(std::ostream& os) {
      for(int i=0; i<Y.rows(); i++) {
        for(int j=0; j<Y.cols(); j++) {
          os << Y(i,j) << " " << dY(i,j) << " " << d2Y(i,j) << std::endl;
        }
      }
    }

    ///row i of matrix Y
    inline const ValueType* restrict y(int i){ return &Y(i,0);}
    ///row i of vector matrix dY
    inline const GradType* restrict dy(int i){ return &dY(i,0);}
    ///row i of matrix d2Y
    inline const ValueType* restrict d2y(int i){ return &d2Y(i,0);}

#ifdef USE_FASTWALKER
    inline const ValueType* restrict y(int iw, int ia){return &Y(iw+NumWalkers*ia,0);}
    inline const GradType* restrict dy(int iw, int ia){return &dY(iw+NumWalkers*ia,0);}
    inline const ValueType* restrict d2y(int iw, int ia){return &d2Y(iw+NumWalkers*ia,0);}
#else
    inline const ValueType* restrict y(int iw, int ia){ return &Y(iw*NumPtcls+ia,0);}
    inline const GradType* restrict dy(int iw, int ia){ return &dY(iw*NumPtcls+ia,0);}
    inline const ValueType* restrict d2y(int iw, int ia){ return &d2Y(iw*NumPtcls+ia,0);}
#endif

    ///the number of particles
    int NumPtcls;
    ///the number of walkers
    int NumWalkers;
    ///total number of basis functions
    int TotalBasis;
    ///container for the id's of the centers (ions),
    ///several centers may share the same id
    vector<int>  I;
    ///container to store the offsets of the basis functions,
    ///the number of basis states for center J is Basis[J+1]-Basis[J]
    vector<int>  Basis;
    ///container for the pointers to the Atomic Orbitals, 
    ///the size of this container being determined by the number 
    ///of unique centers
    vector<COT*> AOs;
    ///matrix to store values \f$ Y[i,j] = \phi_j(r_i) \f$
    Matrix<ValueType> Y;
    ///matrix to store gradients \f$ dY[i,j] = {\bf \nabla}_i \phi_j(r_i) \f$
    Matrix<GradType>  dY;
    ///matrix to store laplacians \f$ d2Y[i,j] = \nabla^2_i \phi_j(r_i) \f$
    Matrix<ValueType> d2Y;
    ///the distance table (ion-electron)
    DistanceTableData* myTable;
  };

}
#endif

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

