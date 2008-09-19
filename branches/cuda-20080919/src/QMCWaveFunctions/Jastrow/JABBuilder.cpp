//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
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
#include "Particle/DistanceTableData.h"
#include "Particle/DistanceTable.h"
#include "QMCWaveFunctions/Jastrow/JABBuilder.h"
#include "QMCWaveFunctions/Jastrow/BsplineJastrowBuilder.h"
#include "QMCWaveFunctions/Jastrow/PadeFunctors.h"
#include "QMCWaveFunctions/Jastrow/GaussianFunctor.h"
#include "QMCWaveFunctions/Jastrow/ModPadeFunctor.h"
#include "QMCWaveFunctions/Jastrow/BsplineFunctor.h"
#include "QMCWaveFunctions/Jastrow/OneBodyJastrowOrbital.h"

namespace qmcplusplus {

  template<class FN>
  bool JABBuilder::createJAB(xmlNodePtr cur, const string& jname) {

    string corr_tag("correlation");

    vector<FN*> jastrow;
    int ng = 0;
    ParticleSet* sourcePtcl=0;
    const xmlChar* s=xmlGetProp(cur,(const xmlChar*)"source");
    if(s != NULL) {
      map<string,ParticleSet*>::iterator pa_it(ptclPool.find((const char*)s));
      if(pa_it == ptclPool.end()) return false;
      sourcePtcl = (*pa_it).second;
      ng=sourcePtcl->getSpeciesSet().getTotalNum();
      for(int i=0; i<ng; i++) jastrow.push_back(0);
    }

    cur = cur->xmlChildrenNode;
    while(cur != NULL) {
      string cname((const char*)(cur->name));
      if(cname == dtable_tag) {
      	string source_name((const char*)(xmlGetProp(cur,(const xmlChar *)"source")));
        map<string,ParticleSet*>::iterator pa_it(ptclPool.find(source_name));
        if(pa_it == ptclPool.end()) return false;
        sourcePtcl = (*pa_it).second;
        ng=sourcePtcl->getSpeciesSet().getTotalNum();
	XMLReport("Number of sources " << ng)
	for(int i=0; i<ng; i++) jastrow.push_back(0);
      } else if(cname == corr_tag) {
        if(sourcePtcl == 0) return false;
        string spA;
        OhmmsAttributeSet rAttrib;
        rAttrib.add(spA, "speciesA"); rAttrib.add(spA, "elementType");
        rAttrib.put(cur);
	//string speciesB((const char*)(xmlGetProp(cur,(const xmlChar *)"speciesB")));
	int ia = sourcePtcl->getSpeciesSet().findSpecies(spA);
	if(!(jastrow[ia])) {
	  jastrow[ia]= new FN;
	  jastrow[ia]->put(cur);
	  LOGMSG("  Added Jastrow Correlation between " << spA)
	}
      }
      cur = cur->next;
    } // while cur
    
    if(sourcePtcl == 0) {//invalid input: cleanup
      for(int ig=0; ig<ng; ig++)  if(jastrow[ig]) delete jastrow[ig];
      return false;
    }

    typedef OneBodyJastrowOrbital<FN> JneType;
    JneType* J1 = new JneType(*sourcePtcl,targetPtcl);
    for(int ig=0; ig<ng; ig++) {
      J1->addFunc(ig,jastrow[ig]);
    }

    string j1name="J1_"+jname;
    targetPsi.addOrbital(J1,j1name);
    XMLReport("Added a One-Body Jastrow Function")
    return true;
  }

  bool JABBuilder::put(xmlNodePtr cur) {

    string jastfunction("pade");
    OhmmsAttributeSet aAttrib;
    aAttrib.add(jastfunction,"function");
    aAttrib.put(cur);

    bool success=false;
    app_log() << "  One-Body Jastrow Function = " << jastfunction << endl;
    if(jastfunction == "pade") 
    {
      success = createJAB<PadeFunctor<RealType> >(cur,jastfunction);
    } 
    else if(jastfunction == "pade2") 
    {
      success = createJAB<Pade2ndOrderFunctor<RealType> >(cur,jastfunction);
    } 
    else if(jastfunction == "short") 
    {
      success = createJAB<ModPadeFunctor<RealType> >(cur,jastfunction);
    }
    else if(jastfunction == "Gaussian") 
    {
      success = createJAB<GaussianFunctor<RealType> >(cur,jastfunction);
    }
    else if(jastfunction == "shiftedGaussian") 
    {
      success = createJAB<TruncatedShiftedGaussianFunctor<RealType> >(cur,jastfunction);
    }
    else if (jastfunction == "Bspline") 
    {
      success = createJAB<BsplineFunctor<RealType> >(cur,jastfunction);
    }
    else {
      app_error() << "Unknown one body function: "
		  << jastfunction << ".\n";
    }
    return success;
  }
}
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
