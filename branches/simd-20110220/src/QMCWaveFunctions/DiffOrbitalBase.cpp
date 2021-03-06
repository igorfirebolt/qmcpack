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
#include "QMCWaveFunctions/DiffOrbitalBase.h"
#include "ParticleBase/ParticleAttribOps.h"

/**@file DiffOrbitalBase.cpp
 *@brief Definition of NumericalDiffOrbital
 */
namespace qmcplusplus
  {
  DiffOrbitalBase::DiffOrbitalBase(OrbitalBase* orb)
  {
    if (orb) refOrbital.push_back(orb);
  }

  DiffOrbitalBasePtr DiffOrbitalBase::makeClone(ParticleSet& tpq) const
    {
      APP_ABORT("Implement DiffOrbitalBase::makeClone for this orbital");
      return 0;
    }

  void NumericalDiffOrbital::resetTargetParticleSet(ParticleSet& P)
  {
    int nptcls=P.getTotalNum();
    dg_p.resize(nptcls);
    dl_p.resize(nptcls);
    dg_m.resize(nptcls);
    dl_m.resize(nptcls);
    gradLogPsi.resize(nptcls);
    lapLogPsi.resize(nptcls);
  }

  void NumericalDiffOrbital::checkOutVariables(const opt_variables_type& optvars)
  {
    //do nothing
  }
  //@{implementation of NumericalDiffOrbital
  void NumericalDiffOrbital::resetParameters(const opt_variables_type& optvars)
  {
    //do nothing
  }


  void NumericalDiffOrbital::evaluateDerivatives(ParticleSet& P,
      const opt_variables_type& optvars,
      vector<RealType>& dlogpsi,
      vector<RealType>& dhpsioverpsi)
  {
    //
    if (refOrbital.empty()) return;

    opt_variables_type v(optvars);

    //this should be modified later
    const RealType delta=1e-4;

    const std::vector<int>& ind_map(refOrbital[0]->myVars.Index);
    for (int j=0; j<ind_map.size(); ++j)
      {
        int jj=ind_map[j];
        if (jj<0) continue;
        RealType plus=0.0;
        RealType minus=0.0;
        RealType curvar=optvars[jj];
        dg_p=0.0;
        dl_p=0.0;
        dg_m=0.0;
        dl_m=0.0;
        //accumulate plus and minus displacement
        for (int i=0; i<refOrbital.size(); ++i)
          {
            v[jj]=optvars[jj]+delta;
            refOrbital[i]->resetParameters(v);
            plus+=refOrbital[i]->evaluateLog(P,dg_p,dl_p);


            v[jj]=optvars[jj]-delta;
            refOrbital[i]->resetParameters(v);
            minus+=refOrbital[i]->evaluateLog(P,dg_m,dl_m);

            //restore the variable to the original state
            v[jj]=curvar;
            refOrbital[i]->resetParameters(v);
          }

        const RealType dh=1.0/(2.0*delta);
        gradLogPsi=dh*(dg_p-dg_m);
        lapLogPsi=dh*(dl_p-dl_m);

        RealType dLogPsi=dh*(plus-minus);
        dlogpsi[jj]=dLogPsi;
        dhpsioverpsi[jj]=-0.5*Sum(lapLogPsi)-Dot(P.G,gradLogPsi);
      }
  }
  //@}

  //@{implementation of AnalyticDiffOrbital
  void AnalyticDiffOrbital::resetParameters(const opt_variables_type& optvars)
  {
    if (MyIndex<0) return;
    for (int i=0; i<refOrbital.size(); ++i)
      refOrbital[i]->resetParameters(optvars);
  }

  void AnalyticDiffOrbital::resetTargetParticleSet(ParticleSet& P)
  {
    if (MyIndex<0) return;
    for (int i=0; i<refOrbital.size(); ++i) refOrbital[i]->resetTargetParticleSet(P);
    int nptcls=P.getTotalNum();
    if (gradLogPsi.size()!=nptcls)
      {
        gradLogPsi.resize(nptcls);
        lapLogPsi.resize(nptcls);
      }
  }

  void AnalyticDiffOrbital::checkOutVariables(const opt_variables_type& optvars)
  {
    MyIndex=-1;
    if (refOrbital.empty()) return;
    for (int i=0; i<refOrbital.size(); ++i)
      refOrbital[i]->checkOutVariables(optvars);
    MyIndex=refOrbital[0]->myVars.Index[0];
  }

  void AnalyticDiffOrbital::evaluateDerivatives(ParticleSet& P,
      const opt_variables_type& optvars,
      vector<RealType>& dlogpsi,
      vector<RealType>& dhpsioverpsi)
  {
    if (MyIndex<0) return;
    RealType dLogPsi=0.0;
    gradLogPsi=0.0;
    lapLogPsi=0.0;
    for (int i=0; i<refOrbital.size(); ++i)
      dLogPsi+=refOrbital[i]->evaluateLog(P,gradLogPsi,lapLogPsi);

    dlogpsi[MyIndex]=dLogPsi;
    dhpsioverpsi[MyIndex]=-0.5*Sum(lapLogPsi)-Dot(P.G,gradLogPsi);
    //optvars.setDeriv(FirstIndex,dLogPsi,-0.5*Sum(lapLogPsi)-Dot(P.G,gradLogPsi));
    //optvars.setDeriv(FirstIndex,dLogPsi,-0.5*Sum(lapLogPsi)-Dot(P.G,gradLogPsi)-dLogPsi*ke0);
  }
  //@}
}
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1770 $   $Date: 2007-02-17 17:45:38 -0600 (Sat, 17 Feb 2007) $
 * $Id: OrbitalBase.h 1770 2007-02-17 23:45:38Z jnkim $
 ***************************************************************************/

