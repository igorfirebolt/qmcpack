//////////////////////////////////////////////////////////////////
// (c) Copyright 2008-  by Jeongnim Kim
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
#ifndef QMCPLUSPLUS_DIFFERENTIAL_TWOBODYJASTROW_H
#define QMCPLUSPLUS_DIFFERENTIAL_TWOBODYJASTROW_H
#include "Configuration.h"
#include "QMCWaveFunctions/DiffOrbitalBase.h"
#include "Particle/DistanceTableData.h"
#include "Particle/DistanceTable.h"
#include "ParticleBase/ParticleAttribOps.h"
#include "Utilities/IteratorUtility.h"

namespace qmcplusplus
  {

  /** @ingroup OrbitalComponent
   *  @brief Specialization for two-body Jastrow function using multiple functors
   */
  template<class FT>
  class DiffTwoBodyJastrowOrbital: public DiffOrbitalBase
    {
      ///number of variables this object handles
      int NumVars;
      ///number of target particles
      int NumPtcls;
      ///number of groups, e.g., for the up/down electrons
      int NumGroups;
      ///read-only distance table
      const DistanceTableData* d_table;
      ///variables handled by this orbital
      opt_variables_type myVars;
      ///container for the Jastrow functions  for all tghe pairs
      vector<FT*> F;

      vector<pair<int,int> > OffSet;
      Vector<RealType> dLogPsi;
      vector<GradVectorType*> gradLogPsi;
      vector<ValueVectorType*> lapLogPsi;
      std::map<std::string,FT*> J2Unique;

    public:

      ///constructor
      DiffTwoBodyJastrowOrbital(ParticleSet& p):NumVars(0)
      {
        NumPtcls=p.getTotalNum();
        NumGroups=p.groups();
        d_table=DistanceTable::add(p);
        F.resize(NumGroups*NumGroups,0);
      }

      ~DiffTwoBodyJastrowOrbital()
      {
        delete_iter(gradLogPsi.begin(),gradLogPsi.end());
        delete_iter(lapLogPsi.begin(),lapLogPsi.end());
      }

      void addFunc(int ia, int ib, FT* j)
      {
        if (ia==ib)
          {
            if (ia==0)//first time, assign everything
              {
                int ij=0;
                for (int ig=0; ig<NumGroups; ++ig)
                  for (int jg=0; jg<NumGroups; ++jg, ++ij)
                    if (F[ij]==0) F[ij]=j;
              }
          }
        else
          {
            F[ia*NumGroups+ib]=j;
            if (ia<ib) F[ib*NumGroups+ia]=j;
          }
        stringstream aname;
        aname<<ia<<ib;
        J2Unique[aname.str()]=j;
      }

      ///reset the value of all the unique Two-Body Jastrow functions
      void resetParameters(const opt_variables_type& active)
      {
        typename std::map<std::string,FT*>::iterator it(J2Unique.begin()),it_end(J2Unique.end());
        while (it != it_end)
          {
            (*it++).second->resetParameters(active);
          }
      }

      ///reset the distance table
      void resetTargetParticleSet(ParticleSet& P)
      {
        d_table = DistanceTable::add(P);
      }

      void checkOutVariables(const opt_variables_type& active)
      {
        myVars.clear();
        typename std::map<std::string,FT*>::iterator it(J2Unique.begin()),it_end(J2Unique.end());
        while (it != it_end)
          {
            (*it).second->myVars.getIndex(active);
            myVars.insertFrom((*it).second->myVars);
            ++it;
          }

        myVars.getIndex(active);
        NumVars=myVars.size();

        //myVars.print(cout);

        if (NumVars && dLogPsi.size()==0)
          {
            dLogPsi.resize(NumVars);
            gradLogPsi.resize(NumVars,0);
            lapLogPsi.resize(NumVars,0);
            for (int i=0; i<NumVars; ++i)
              {
                gradLogPsi[i]=new GradVectorType(NumPtcls);
                lapLogPsi[i]=new ValueVectorType(NumPtcls);
              }
            OffSet.resize(F.size());
            int varoffset=myVars.Index[0];
            for (int i=0; i<F.size(); ++i)
              {
                OffSet[i].first=F[i]->myVars.Index.front()-varoffset;
                OffSet[i].second=F[i]->myVars.Index.size()+OffSet[i].first;
              }
          }
      }

      void evaluateDerivatives(ParticleSet& P,
                               const opt_variables_type& active,
                               vector<RealType>& dlogpsi,
                               vector<RealType>& dhpsioverpsi)
      {
        bool recalculate(false);
        vector<bool> rcsingles(myVars.size(),false);
        for (int k=0; k<myVars.size(); ++k)
          {
            int kk=myVars.where(k);
            if (kk<0) continue;
            if (active.recompute(kk)) recalculate=true;
            rcsingles[k]=true;
          }

        if (recalculate)
          {
            dLogPsi=0.0;
            for (int p=0;p<NumVars; ++p)(*gradLogPsi[p])=0.0;
            for (int p=0;p<NumVars; ++p)(*lapLogPsi[p])=0.0;

            vector<TinyVector<RealType,3> > derivs(NumVars);

            for (int i=0; i<d_table->size(SourceIndex); ++i)
              {
                for (int nn=d_table->M[i]; nn<d_table->M[i+1]; ++nn)
                  {
                    int ptype=d_table->PairID[nn];
                    bool recalcFunc(false);
                    for (int rcs=OffSet[ptype].first;rcs<OffSet[ptype].second;rcs++) if (rcsingles[rcs]==true) recalcFunc=true;
                    if (recalcFunc)
                      {
                        std::fill(derivs.begin(),derivs.end(),0.0);
                        if (!F[ptype]->evaluateDerivatives(d_table->r(nn),derivs)) continue;
                        int j = d_table->J[nn];
                        RealType rinv(d_table->rinv(nn));
                        PosType dr(d_table->dr(nn));

                        for (int p=OffSet[ptype].first, ip=0; p<OffSet[ptype].second; ++p,++ip)
                          {
                            RealType dudr(rinv*derivs[ip][1]);
                            RealType lap(derivs[ip][2]+2.0*dudr);
                            PosType gr(dudr*dr);

                            dLogPsi[p]-=derivs[ip][0];
                            (*gradLogPsi[p])[i] += gr;
                            (*gradLogPsi[p])[j] -= gr;
                            (*lapLogPsi[p])[i] -=lap;
                            (*lapLogPsi[p])[j] -=lap;
                          }
                      }
                  }
              }

            for (int k=0; k<myVars.size(); ++k)
              {
                int kk=myVars.where(k);
                if (kk<0) continue;
                if (rcsingles[k])
                  {
                    dlogpsi[kk]=dLogPsi[k];
                    dhpsioverpsi[kk]=-0.5*Sum(*lapLogPsi[k])-Dot(P.G,*gradLogPsi[k]);
                  }
                //optVars.setDeriv(p,dLogPsi[ip],-0.5*Sum(*lapLogPsi[ip])-Dot(P.G,*gradLogPsi[ip]));
              }
          }
      }

      DiffOrbitalBasePtr makeClone(ParticleSet& tqp) const
        {
          DiffTwoBodyJastrowOrbital<FT>* j2copy=new DiffTwoBodyJastrowOrbital<FT>(tqp);
          map<const FT*,FT*> fcmap;
          for (int ig=0; ig<NumGroups; ++ig)
            for (int jg=ig; jg<NumGroups; ++jg)
              {
                int ij=ig*NumGroups+jg;
                if (F[ij]==0) continue;
                typename map<const FT*,FT*>::iterator fit=fcmap.find(F[ij]);
                if (fit == fcmap.end())
                  {
                    FT* fc=new FT(*F[ij]);
                    j2copy->addFunc(ig,jg,fc);
                    fcmap[F[ij]]=fc;
                  }
              }

          j2copy->myVars.clear();
          j2copy->myVars.insertFrom(myVars);
          j2copy->NumVars=NumVars;
          j2copy->NumPtcls=NumPtcls;
          j2copy->NumGroups=NumGroups;
          j2copy->dLogPsi.resize(NumVars);
          j2copy->gradLogPsi.resize(NumVars,0);
          j2copy->lapLogPsi.resize(NumVars,0);
          for (int i=0; i<NumVars; ++i)
            {
              j2copy->gradLogPsi[i]=new GradVectorType(NumPtcls);
              j2copy->lapLogPsi[i]=new ValueVectorType(NumPtcls);
            }
          j2copy->OffSet=OffSet;


          return j2copy;
        }

    };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1761 $   $Date: 2007-02-17 17:11:59 -0600 (Sat, 17 Feb 2007) $
 * $Id: TwoBodyJastrowOrbital.h 1761 2007-02-17 23:11:59Z jnkim $
 ***************************************************************************/

