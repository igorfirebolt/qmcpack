/////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "QMCDrivers/QMCDriver.h"
#include "Utilities/OhmmsInfo.h"
#include "Particle/MCWalkerConfiguration.h"
#include "Particle/HDFWalkerIO.h"
#include "ParticleBase/ParticleUtility.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "OhmmsData/AttributeSet.h"
#include "QMCDrivers/DriftOperators.h"
#include "QMCWaveFunctions/OrbitalTraits.h"
#include "Message/Communicate.h"
#include "Message/CommOperators.h"
#include "OhmmsApp/RandomNumberControl.h"
#include "HDFVersion.h"
#include <limits>

namespace qmcplusplus {

  QMCDriver::QMCDriver(MCWalkerConfiguration& w, TrialWaveFunction& psi, QMCHamiltonian& h):
    branchEngine(0), ResetRandom(false), AppendRun(false),
    MyCounter(0), RollBackBlocks(0),
    Period4CheckPoint(1), Period4WalkerDump(0),
    CurrentStep(0), nBlocks(100), nSteps(10), 
    nAccept(0), nReject(0), nTargetWalkers(0),
    Tau(0.01), qmcNode(NULL),
    QMCType("invalid"), 
    qmcComm(0), wOut(0),
    W(w), Psi(psi), H(h), Estimators(0)
  { 

    //use maximum double
    MaxCPUSecs=numeric_limits<RealType>::max();

    m_param.add(nSteps,"steps","int");
    m_param.add(nBlocks,"blocks","int");
    m_param.add(nTargetWalkers,"walkers","int");
    m_param.add(CurrentStep,"current","int");
    m_param.add(Tau,"timeStep","AU");
    m_param.add(Tau,"Tau","AU");
    m_param.add(Tau,"timestep","AU");
    m_param.add(RollBackBlocks,"rewind","int");
    m_param.add(Period4WalkerDump,"recordWalkers","int");
    m_param.add(MaxCPUSecs,"maxcpusecs","real");

    //add each QMCHamiltonianBase to W.PropertyList so that averages can be taken
    H.add2WalkerProperty(W);
  }

  void QMCDriver::setCommunicator(Communicate* c) 
  {
    qmcComm = c ? c : OHMMS::Controller;
  }

  QMCDriver::~QMCDriver() 
  { 
  }

  void QMCDriver::add_H_and_Psi(QMCHamiltonian* h, TrialWaveFunction* psi) {
    H1.push_back(h);
    Psi1.push_back(psi);
  }

  /** process a <qmc/> element 
   * @param cur xmlNode with qmc tag
   *
   * This function is called before QMCDriver::run and following actions are taken:
   * - Initialize basic data to execute run function.
   * -- distance tables
   * -- resize deltaR and drift with the number of particles
   * -- assign cur to qmcNode
   * - process input file
   *   -- putQMCInfo: <parameter/> s for generic QMC
   *   -- put : extra data by derived classes
   * - initialize branchEngine to accumulate energies
   * - initialize Estimators
   * - initialize Walkers
   */
  void QMCDriver::process(xmlNodePtr cur) {

    deltaR.resize(W.getTotalNum());
    drift.resize(W.getTotalNum());

    qmcNode=cur;

    //process common parameters
    putQMCInfo(cur);

    //need to initialize properties
    int numCopies= (H1.empty())?1:H1.size();
    W.resetWalkerProperty(numCopies);

    //create branchEngine first
    if(branchEngine==0) 
      branchEngine = new BranchEngineType(Tau,W.getActiveWalkers());

    //execute the put function implemented by the derived classes
    put(cur);

    //create and initialize estimator
    Estimators = branchEngine->getEstimatorManager();
    if(Estimators==0)
    {
      Estimators = new EstimatorManager(qmcComm);
      branchEngine->setEstimatorManager(Estimators);
      if(h5FileRoot.size()) branchEngine->read(h5FileRoot);
    }

    branchEngine->put(cur);
    Estimators->put(W,H,cur);

    if(wOut==0) {
      wOut = new HDFWalkerOutput(W,RootName,qmcComm);
      branchEngine->start(RootName,true);
      branchEngine->write(wOut->FileName,false);
    }
    else
      branchEngine->start(RootName,false);

    //use new random seeds
    if(ResetRandom) {
      app_log() << "  Regenerate random seeds." << endl;
      RandomNumberControl::make_seeds();
      ResetRandom=false;
    }

    //flush the ostreams
    OhmmsInfo::flush();


    //increment QMCCounter of the branch engine
    branchEngine->advanceQMCCounter();
  }

  void QMCDriver::setStatus(const string& aname, const string& h5name, bool append) {
    RootName = aname;
    app_log() << "\n=========================================================" 
              << "\n  Start " << QMCType 
              << "\n  File Root " << RootName;
    if(append) 
      app_log() << " append = yes ";
    else 
      app_log() << " append = no ";
    app_log() << "\n=========================================================" << endl;

    if(h5name.size()) h5FileRoot = h5name;
    AppendRun = append;
  }

  
  /** Read walker configurations from *.config.h5 files
   * @param wset list of xml elements containing mcwalkerset
   */
  void QMCDriver::putWalkers(vector<xmlNodePtr>& wset) {

    if(wset.empty()) return;
    int nfile=wset.size();

    HDFWalkerInputManager W_in(W,qmcComm);
    for(int i=0; i<wset.size(); i++)
      if(W_in.put(wset[i])) h5FileRoot = W_in.getFileRoot();

    //clear the walker set
    wset.clear();
  }

  void QMCDriver::recordBlock(int block) {

    ////first dump the data for restart
    if(wOut ==0)
    {//this does not happen but just make sure there is no memory fault 
      wOut = new HDFWalkerOutput(W,RootName,qmcComm);
      branchEngine->start(RootName,true);
    }

    if(block%Period4CheckPoint == 0)
    {
      wOut->dump(W);
      branchEngine->write(wOut->FileName,false); //save energy_history
    }

    //save positions for optimization
    if(QMCDriverMode[QMC_OPTIMIZE]) W.saveEnsemble();
    //if(Period4WalkerDump>0) wOut->append(W);

    //flush the ostream
    //OhmmsInfo::flush();
  }

  bool QMCDriver::finalize(int block) {

    branchEngine->finalize();

    RandomNumberControl::write(wOut->FileName,qmcComm);

    delete wOut;
    wOut=0;

    //Estimators->finalize();

    //set the target walkers
    nTargetWalkers = W.getActiveWalkers();

    //increment MyCounter
    MyCounter++;

    //flush the ostream
    OhmmsInfo::flush();

    return true;
  }

  /** Add walkers to the end of the ensemble of walkers.  
   * @param nwalkers number of walkers to add
   */
  void 
  QMCDriver::addWalkers(int nwalkers) {

    if(nwalkers>0) {
      //add nwalkers walkers to the end of the ensemble
      int nold = W.getActiveWalkers();

      app_log() << "  Adding " << nwalkers << " walkers to " << nold << " existing sets" << endl;

      W.createWalkers(nwalkers);
      
      ParticleSet::ParticlePos_t rv(W.getTotalNum());
      MCWalkerConfiguration::iterator it(W.begin()), it_end(W.end());
      while(it != it_end) {
	(*it)->R=W.R;
	++it;
      }
    } else if(nwalkers<0) {
      W.destroyWalkers(-nwalkers);
      app_log() << "  Removed " << -nwalkers << " walkers. Current number of walkers =" << W.getActiveWalkers() << endl;
    } else {
      app_log() << "  Using the current " << W.getActiveWalkers() << " walkers." <<  endl;
    }

    //update the global number of walkers
    //int nw=W.getActiveWalkers();
    //qmcComm->allreduce(nw);
    vector<int> nw(qmcComm->size(),0),nwoff(qmcComm->size()+1,0);
    nw[qmcComm->rank()]=W.getActiveWalkers();
    qmcComm->allreduce(nw);

    for(int ip=0; ip<qmcComm->size(); ip++) nwoff[ip+1]=nwoff[ip]+nw[ip];
    W.setGlobalNumWalkers(nwoff[qmcComm->size()]);
    W.setWalkerOffsets(nwoff);

    app_log() << "  Total number of walkers: " << W.EnsembleProperty.NumSamples  <<  endl;
    app_log() << "  Total weight: " << W.EnsembleProperty.Weight  <<  endl;
  }

  
  /** Parses the xml input file for parameter definitions for a single qmc simulation.
   */
  bool 
  QMCDriver::putQMCInfo(xmlNodePtr cur) {
    
    int defaultw = 100;
    int targetw = 0;

    //these options are reset for each block
    Period4WalkerDump=0;
    Period4CheckPoint=1;

    OhmmsAttributeSet aAttrib;
    aAttrib.add(Period4CheckPoint,"checkpoint");
    aAttrib.put(cur);
     
    if(cur != NULL) {
      //initialize the parameter set
      m_param.put(cur);
      xmlNodePtr tcur=cur->children;
      //determine how often to print walkers to hdf5 file
      while(tcur != NULL) {
	string cname((const char*)(tcur->name));
	if(cname == "record") {
          //construct a set of attributes
          OhmmsAttributeSet rAttrib;
          rAttrib.add(Period4WalkerDump,"stride");
          rAttrib.add(Period4WalkerDump,"period");
          rAttrib.put(tcur);
	} else if(cname == "checkpoint") {
          OhmmsAttributeSet rAttrib;
          rAttrib.add(Period4CheckPoint,"stride");
          rAttrib.add(Period4CheckPoint,"period");
          rAttrib.put(tcur);
        } else if(cname == "random") {
          ResetRandom = true;
        }
	tcur=tcur->next;
      }
    }
    
    //reset CurrentStep to zero if qmc/@continue='no'
    if(!AppendRun) CurrentStep=0;

    app_log() << "  timestep = " << Tau << endl;
    app_log() << "  blocks = " << nBlocks << endl;
    app_log() << "  steps = " << nSteps << endl;
    app_log() << "  current = " << CurrentStep << endl;

    //Need MPI-IO
    app_log() << "  walkers = " << W.getActiveWalkers() << endl;

    /*check to see if the target population is different 
      from the current population.*/ 
    int nw  = W.getActiveWalkers();
    int ndiff = 0;
    if(nw) { // walkers exist
      // nTargetWalkers == 0, if it is not set by the input file
      ndiff = (nTargetWalkers)? nTargetWalkers-nw: 0;
    } else {
      ndiff= (nTargetWalkers)? nTargetWalkers:defaultw;
    }

    addWalkers(ndiff);

    //always true
    return (W.getActiveWalkers()>0);
  }

  xmlNodePtr QMCDriver::getQMCNode() {

    xmlNodePtr newqmc = xmlCopyNode(qmcNode,1);
    //update current
    xmlNodePtr current_ptr=NULL;
    xmlNodePtr cur=newqmc->children;
    while(cur != NULL && current_ptr == NULL) {
      string cname((const char*)(cur->name)); 
      if(cname == "parameter") {
        const xmlChar* aptr= xmlGetProp(cur, (const xmlChar *) "name");
        if(aptr) {
          if(xmlStrEqual(aptr,(const xmlChar*)"current")) current_ptr=cur;
        }
      }
      cur=cur->next;
    }
    if(current_ptr == NULL) {
      current_ptr = xmlNewTextChild(newqmc,NULL,(const xmlChar*)"parameter",(const xmlChar*)"0");
      xmlNewProp(current_ptr,(const xmlChar*)"name",(const xmlChar*)"current");
    } 
    getContent(CurrentStep,current_ptr);
    return newqmc;
  }

}


/***************************************************************************
 * $RCSfile: QMCDriver.cpp,v $   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
