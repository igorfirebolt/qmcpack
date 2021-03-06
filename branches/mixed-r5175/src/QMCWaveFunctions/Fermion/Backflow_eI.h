//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
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
#ifndef QMCPLUSPLUS_BACKFLOW_ELEC_ION_H
#define QMCPLUSPLUS_BACKFLOW_ELEC_ION_H
#include "QMCWaveFunctions/OrbitalSetTraits.h"
#include "QMCWaveFunctions/Fermion/BackflowFunctionBase.h"
#include "Particle/DistanceTable.h"
#include <cmath>
#include <vector>

namespace qmcplusplus
{
  template<class FT>
  class Backflow_eI: public BackflowFunctionBase 
  {

    public:

    vector<FT*> RadFun;
    vector<FT*> uniqueRadFun;
    vector<int> offsetPrms;

    Backflow_eI(ParticleSet& ions, ParticleSet& els): BackflowFunctionBase(ions,els)
    {
      myTable = DistanceTable::add(ions,els); 
      resize(NumTargets,NumCenters);
    }

    //  build RadFun manually from builder class
    Backflow_eI(ParticleSet& ions, ParticleSet& els, FT* RF): BackflowFunctionBase(ions,els)
    {
      myTable = DistanceTable::add(ions,els);
      // same radial function for all centers by default
      uniqueRadFun.push_back(RF);
      for(int i=0; i<NumCenters; i++) RadFun.push_back(RF);
    }

    ~Backflow_eI() {}; 
    
    void resize(int NT, int NC)
    {
      NumTargets=NT; NumCenters=NC;
      UIJ.resize(NumTargets,NumCenters);
      AIJ.resize(NumTargets,NumCenters);
      BIJ.resize(NumTargets,NumCenters);
      UIJ_temp.resize(NumCenters);
      AIJ_temp.resize(NumCenters);
      BIJ_temp.resize(NumCenters);
    }
 
    void resetParameters(const opt_variables_type& active)
    {
      for(int i=0; i<uniqueRadFun.size(); i++) uniqueRadFun[i]->resetParameters(active);
    }

    void checkInVariables(opt_variables_type& active)
    {
      for(int i=0; i<uniqueRadFun.size(); i++) uniqueRadFun[i]->checkInVariables(active);
    }

    void checkOutVariables(const opt_variables_type& active)
    {
      for(int i=0; i<uniqueRadFun.size(); i++) uniqueRadFun[i]->checkOutVariables(active);
    }

    BackflowFunctionBase* makeClone()
    {
       Backflow_eI* clone = new Backflow_eI(*this);
       return clone; 
    }

    inline int 
    indexOffset() 
    {
       return RadFun[0]->myVars.where(0);
    }

    inline void
    acceptMove(int iat, int UpdateMode)
    {
      int num;
      switch(UpdateMode)
      {
        case ORB_PBYP_RATIO:
          num = UIJ.rows();
          for(int i=0; i<num; i++) 
            UIJ(iat,i) = UIJ_temp(i);
          break;
        case ORB_PBYP_PARTIAL:
          num = UIJ.rows();
          for(int i=0; i<num; i++) 
            UIJ(iat,i) = UIJ_temp(i);
          num = AIJ.rows();
          for(int i=0; i<num; i++) 
            AIJ(iat,i) = AIJ_temp(i);
          break;
        case ORB_PBYP_ALL:
          num = UIJ.rows();
          for(int i=0; i<num; i++) 
            UIJ(iat,i) = UIJ_temp(i);
          num = AIJ.rows();
          for(int i=0; i<num; i++) 
            AIJ(iat,i) = AIJ_temp(i);
          num = BIJ.rows();
          for(int i=0; i<num; i++) 
            BIJ(iat,i) = BIJ_temp(i);
          break;
        default:
          num = UIJ.rows();
          for(int i=0; i<num; i++) 
            UIJ(iat,i) = UIJ_temp(i);
          num = AIJ.rows();
          for(int i=0; i<num; i++) 
            AIJ(iat,i) = AIJ_temp(i);
          num = BIJ.rows();
          for(int i=0; i<num; i++) 
            BIJ(iat,i) = BIJ_temp(i);
          break;
      }
      UIJ_temp=0.0;
      AIJ_temp=0.0;
      BIJ_temp=0.0;
    }

    inline void
    restore(int iat, int UpdateType)
    {
      UIJ_temp=0.0;
      AIJ_temp=0.0;
      BIJ_temp=0.0;
    }

    /** calculate quasi-particle coordinates only
     */
    inline void 
    evaluate(const ParticleSet& P, ParticleSet& QP)
    {
      ValueType du,d2u; 
      for(int i=0; i<myTable->size(SourceIndex); i++) {
        for(int nn=myTable->M[i]; nn<myTable->M[i+1]; nn++) {
          int j = myTable->J[nn];
          ValueType uij = RadFun[i]->evaluate(myTable->r(nn),du,d2u);
          QP.R[j] += (UIJ(j,i) = uij*myTable->dr(nn));  // dr(ij) = r_j-r_i 
        }
      }
    }


    inline void
    evaluate(const ParticleSet& P, ParticleSet& QP, GradVector_t& Bmat, HessMatrix_t& Amat)
    {
      ValueType du,d2u,temp;
      for(int i=0; i<myTable->size(SourceIndex); i++) {
        for(int nn=myTable->M[i]; nn<myTable->M[i+1]; nn++) {
          int j = myTable->J[nn];
          ValueType uij = RadFun[i]->evaluate(myTable->r(nn),du,d2u);
          //PosType u = uij*myTable->dr(nn);
          du *= myTable->rinv(nn);
          QP.R[j] += (UIJ(j,i) = uij*myTable->dr(nn));

          HessType& hess = AIJ(j,i);
          hess = du*outerProduct(myTable->dr(nn),myTable->dr(nn));
          hess[0] += uij;
          hess[4] += uij;
          hess[8] += uij;
          Amat(j,j) += hess;

          //u = (d2u+4.0*du)*myTable->dr(nn);
          Bmat(j) += (BIJ(j,i)=(d2u+4.0*du)*myTable->dr(nn));
        }
      }
    }

    
    /** calculate quasi-particle coordinates, Bmat and Amat 
     */
    inline void
    evaluate(const ParticleSet& P, ParticleSet& QP, GradMatrix_t& Bmat_full, HessMatrix_t& Amat)
    {
      RealType du,d2u,temp;
      for(int i=0; i<myTable->size(SourceIndex); i++) {
        for(int nn=myTable->M[i]; nn<myTable->M[i+1]; nn++) {
          int j = myTable->J[nn];
          ValueType uij = RadFun[i]->evaluate(myTable->r(nn),du,d2u);
          du *= myTable->rinv(nn);
          //PosType u = uij*myTable->dr(nn); 
          QP.R[j] += (UIJ(j,i) = uij*myTable->dr(nn));

          HessType& hess = AIJ(j,i);
          hess = du*outerProduct(myTable->dr(nn),myTable->dr(nn));
          hess[0] += uij;
          hess[4] += uij;
          hess[8] += uij;
          Amat(j,j) += hess;

// this will create problems with QMC_COMPLEX, because Bmat is ValueType and dr is RealType
          //u = (d2u+4.0*du)*myTable->dr(nn); 
          Bmat_full(j,j) += (BIJ(j,i)=(d2u+4.0*du)*myTable->dr(nn));  
        }
      }
    }

     /** calculate quasi-particle coordinates after pbyp move  
      */
    inline void
    evaluatePbyP(const ParticleSet& P, ParticleSet::ParticlePos_t& newQP
               ,const vector<int>& index)
    {
      RealType du,d2u;
      int maxI = myTable->size(SourceIndex);
      int iat = index[0];
      
      for(int j=0; j<maxI; j++) {
        ValueType uij = RadFun[j]->evaluate(myTable->Temp[j].r1,du,d2u);
        PosType u = (UIJ_temp(j)=uij*myTable->Temp[j].dr1)-UIJ(iat,j);
        newQP[iat] += u;
      }
    }

    inline void
    evaluatePbyP(const ParticleSet& P, ParticleSet::ParticlePos_t& newQP
               ,const vector<int>& index, HessMatrix_t& Amat)
    {
      RealType du,d2u;
      int maxI = myTable->size(SourceIndex);
      int iat = index[0];
      
      for(int j=0; j<maxI; j++) {
        ValueType uij = RadFun[j]->evaluate(myTable->Temp[j].r1,du,d2u);
        PosType u = (UIJ_temp(j)=uij*myTable->Temp[j].dr1)-UIJ(iat,j);
        newQP[iat] += u;

        HessType& hess = AIJ_temp(j);
        hess = (du*myTable->Temp[j].rinv1)*outerProduct(myTable->Temp[j].dr1,myTable->Temp[j].dr1);
        hess[0] += uij;
        hess[4] += uij;
        hess[8] += uij;
// should I expand this??? Is the compiler smart enough???
        Amat(iat,iat) += (hess - AIJ(iat,j));
      }
    }

    inline void
    evaluatePbyP(const ParticleSet& P, ParticleSet::ParticlePos_t& newQP
               ,const vector<int>& index, GradMatrix_t& Bmat_full, HessMatrix_t& Amat)
    {
      RealType du,d2u;
      int maxI = myTable->size(SourceIndex);
      int iat = index[0];

      for(int j=0; j<maxI; j++) {
        ValueType uij = RadFun[j]->evaluate(myTable->Temp[j].r1,du,d2u);
        PosType u = (UIJ_temp(j)=uij*myTable->Temp[j].dr1)-UIJ(iat,j);
        newQP[iat] += u;
        du *= myTable->Temp[j].rinv1;

        HessType& hess = AIJ_temp(j);
        hess = du*outerProduct(myTable->Temp[j].dr1,myTable->Temp[j].dr1);
        hess[0] += uij;
        hess[4] += uij;
        hess[8] += uij;
// should I expand this??? Is the compiler smart enough???
        Amat(iat,iat) += (hess - AIJ(iat,j));
        
        BIJ_temp(j)=(d2u+4.0*du)*myTable->Temp[j].dr1;
        Bmat_full(iat,iat) += (BIJ_temp(j)-BIJ(iat,j));
      }
    }


    /** calculate only Bmat
     *  This is used in pbyp moves, in updateBuffer()  
     */
    inline void
    evaluateBmatOnly(const ParticleSet& P,GradMatrix_t& Bmat_full)
    {
      RealType du,d2u;
      for(int i=0; i<myTable->size(SourceIndex); i++) {
        for(int nn=myTable->M[i]; nn<myTable->M[i+1]; nn++) {
          int j = myTable->J[nn];
          ValueType uij = RadFun[i]->evaluate(myTable->r(nn),du,d2u);
          Bmat_full(j,j) += (BIJ(j,i)=(d2u+4.0*du*myTable->rinv(nn))*myTable->dr(nn));
        }
      }
    }

    /** calculate quasi-particle coordinates, Bmat and Amat 
     *  calculate derivatives wrt to variational parameters
     */
    inline void
    evaluateWithDerivatives(const ParticleSet& P, ParticleSet& QP, GradMatrix_t& Bmat_full, HessMatrix_t& Amat, GradMatrix_t& Cmat, GradMatrix_t& Ymat, HessArray_t& Xmat)
    {
      RealType du,d2u,temp;
      for(int i=0; i<myTable->size(SourceIndex); i++) {
        for(int nn=myTable->M[i]; nn<myTable->M[i+1]; nn++) {
          int j = myTable->J[nn];
          ValueType uij = RadFun[i]->evaluate(myTable->r(nn),du,d2u);
          std::fill(derivs.begin(),derivs.end(),0.0);
          RadFun[i]->evaluateDerivatives(myTable->r(nn),derivs);
          du *= myTable->rinv(nn);
          //PosType u = uij*myTable->dr(nn); 
          QP.R[j] += (UIJ(j,i) = uij*myTable->dr(nn)); 

          HessType op = outerProduct(myTable->dr(nn),myTable->dr(nn));
          HessType& hess = AIJ(j,i);
          hess = du*op;
          hess[0] += uij;
          hess[4] += uij;
          hess[8] += uij;
          Amat(j,j) += hess;

// this will create problems with QMC_COMPLEX, because Bmat is ValueType and dr is RealType
          //u = (d2u+4.0*du)*myTable->dr(nn); 
          Bmat_full(j,j) += (BIJ(j,i)=(d2u+4.0*du)*myTable->dr(nn));

          int NPrms = RadFun[i]->NumParams; 
          for(int prm=0,la=indexOfFirstParam+offsetPrms[i]; prm<NPrms; prm++,la++) {
            Cmat(la,j) += myTable->dr(nn)*derivs[prm][0];

            Xmat(la,j,j) += (derivs[prm][1]*myTable->rinv(nn))*op;
            Xmat(la,j,j)[0] += derivs[prm][0];
            Xmat(la,j,j)[4] += derivs[prm][0];
            Xmat(la,j,j)[8] += derivs[prm][0];

            Ymat(la,j) += (derivs[prm][2]+4.0*derivs[prm][1]*myTable->rinv(nn))*myTable->dr(nn);

          }


        }
      }
    }

  };
}

#endif
