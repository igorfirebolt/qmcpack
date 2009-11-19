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
using namespace std;
#include "Particle/MCWalkerConfiguration.h"
#include "Particle/DistanceTable.h"
#include "ParticleBase/ParticleUtility.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "Message/Communicate.h"
#include "QMCDrivers/WaveFunctionTester.h"
#include "QMCDrivers/DriftOperators.h"
#include "Optimize/VarList.h"
#include "Utilities/OhmmsInform.h"
#include "LongRange/StructFact.h"
#include "OhmmsData/AttributeSet.h"
#include "OhmmsData/ParameterSet.h"
#include "QMCWaveFunctions/SPOSetBase.h"
#include "QMCWaveFunctions/Fermion/SlaterDet.h"
#include "QMCWaveFunctions/OrbitalSetTraits.h"
#include "Numerics/DeterminantOperators.h"
#include "Numerics/SymmetryOperations.h"
#include "Numerics/Blasf.h"

namespace qmcplusplus
  {


  WaveFunctionTester::WaveFunctionTester(MCWalkerConfiguration& w,
                                         TrialWaveFunction& psi,
                                         QMCHamiltonian& h,
                                         ParticleSetPool &ptclPool):
      QMCDriver(w,psi,h),checkRatio("no"),checkClone("no"), checkHamPbyP("no"),
      PtclPool(ptclPool), wftricks("no")
  {
    m_param.add(checkRatio,"ratio","string");
    m_param.add(checkClone,"clone","string");
    m_param.add(checkHamPbyP,"hamiltonianpbyp","string");
    m_param.add(sourceName,"source","string");
    m_param.add(wftricks,"wftricks","string");
  }


  /*!
   * \brief Test the evaluation of the wavefunction, gradient and laplacian
   by comparing to the numerical evaluation.
   *
   Use the finite difference formulas formulas
   \f[
   \nabla_i f({\bf R}) = \frac{f({\bf R+\Delta r_i}) - f({\bf R})}{2\Delta r_i}
   \f]
   and
   \f[
   \nabla_i^2 f({\bf R}) = \sum_{x,y,z} \frac{f({\bf R}+\Delta x_i)
   - 2 f({\bf R}) + f({\bf R}-\Delta x_i)}{2\Delta x_i^2},
   \f]
   where \f$ f = \ln \Psi \f$ and \f$ \Delta r_i \f$ is a
   small displacement for the ith particle.
  */

  bool
  WaveFunctionTester::run()
  {

    app_log() << "Starting a Wavefunction tester" << endl;

    //DistanceTable::create(1);

    put(qmcNode);

    if (checkRatio == "yes")
      {
        runRatioTest();
        runRatioTest2();
      }
    else if (checkClone == "yes")
      runCloneTest();
    else if (sourceName.size() != 0)
      {
        runGradSourceTest();
        runZeroVarianceTest();
      }
    else if (checkRatio =="deriv")
      runDerivTest();
    else if (wftricks !="no")
      runwftricks();
    else
      runBasicTest();

    RealType ene = H.evaluate(W);

    cout << " Energy " << ene << endl;

    return true;
  }

  void WaveFunctionTester::runCloneTest()
  {

    for (int iter=0; iter<4; ++iter)
      {
        ParticleSet* w_clone = new MCWalkerConfiguration(W);
        TrialWaveFunction *psi_clone = Psi.makeClone(*w_clone);
        QMCHamiltonian *h_clone = H.makeClone(*w_clone,*psi_clone);
        h_clone->setPrimary(false);

        IndexType nskipped = 0;
        RealType sig2Enloc=0, sig2Drift=0;
        RealType delta = 0.00001;
        RealType delta2 = 2*delta;
        ValueType c1 = 1.0/delta/2.0;
        ValueType c2 = 1.0/delta/delta;

        int nat = W.getTotalNum();

        ParticleSet::ParticlePos_t deltaR(nat);
        MCWalkerConfiguration::PropertyContainer_t Properties;
        //pick the first walker
        MCWalkerConfiguration::Walker_t* awalker = *(W.begin());

        //copy the properties of the working walker
        Properties = awalker->Properties;

        W.R = awalker->R;
        W.update();
        ValueType logpsi1 = Psi.evaluateLog(W);
        RealType eloc1  = H.evaluate(W);

        w_clone->R=awalker->R;
        w_clone->update();
        ValueType logpsi2 = psi_clone->evaluateLog(*w_clone);
        RealType eloc2  = h_clone->evaluate(*w_clone);

        app_log() << "Testing walker-by-walker functions " << endl;
        app_log() << "log (original) = " << logpsi1 << " energy = " << eloc1 << endl;
        app_log() << "log (clone)    = " << logpsi2 << " energy = " << eloc2 << endl;


        app_log() << "Testing pbyp functions " << endl;
        Walker_t::Buffer_t &wbuffer(awalker->DataSet);
        wbuffer.clear();
        app_log() << "  Walker Buffer State current=" << wbuffer.current() << " size=" << wbuffer.size() << endl;
        //W.registerData(wbuffer);
        logpsi1=Psi.registerData(W,wbuffer);
        eloc1= H.evaluate(W);
        app_log() << "  Walker Buffer State current=" << wbuffer.current() << " size=" << wbuffer.size() << endl;

        wbuffer.clear();
        app_log() << "  Walker Buffer State current=" << wbuffer.current() << " size=" << wbuffer.size() << endl;
        //w_clone->registerData(wbuffer);
        logpsi2=psi_clone->registerData(W,wbuffer);
        eloc2= H.evaluate(*w_clone);
        app_log() << "  Walker Buffer State current=" << wbuffer.current() << " size=" << wbuffer.size() << endl;

        app_log() << "log (original) = " << logpsi1 << " energy = " << eloc1 << endl;
        app_log() << "log (clone)    = " << logpsi2 << " energy = " << eloc2 << endl;

        delete h_clone;
        delete psi_clone;
        delete w_clone;
      }
  }

  void WaveFunctionTester::runBasicTest()
  {
    IndexType nskipped = 0;
    RealType sig2Enloc=0, sig2Drift=0;
    RealType delta = 0.00001;
    RealType delta2 = 2*delta;
    ValueType c1 = 1.0/delta/2.0;
    ValueType c2 = 1.0/delta/delta;

    int nat = W.getTotalNum();

    ParticleSet::ParticlePos_t deltaR(nat);
    MCWalkerConfiguration::PropertyContainer_t Properties;
    //pick the first walker
    MCWalkerConfiguration::Walker_t* awalker = *(W.begin());

    //copy the properties of the working walker
    Properties = awalker->Properties;

    //sample a new walker configuration and copy to ParticleSet::R
    //makeGaussRandom(deltaR);

    W.R = awalker->R;

    //W.R += deltaR;

    W.update();
    //ValueType psi = Psi.evaluate(W);
    ValueType logpsi = Psi.evaluateLog(W);
    RealType eloc=H.evaluate(W);

    app_log() << "  HamTest " << "  Total " <<  eloc << endl;
    for (int i=0; i<H.sizeOfObservables(); i++)
      app_log() << "  HamTest " << H.getObservableName(i) << " " << H.getObservable(i) << endl;

    //RealType psi = Psi.evaluateLog(W);
    ParticleSet::ParticleGradient_t G(nat), G1(nat);
    ParticleSet::ParticleLaplacian_t L(nat), L1(nat);
    G = W.G;
    L = W.L;

//#if defined(QMC_COMPLEX)
//  ValueType logpsi(std::log(psi));
//#else
//  ValueType logpsi(std::log(std::abs(psi)));
//#endif

    for (int iat=0; iat<nat; iat++)
      {
        PosType r0 = W.R[iat];
        GradType g0;
        ValueType lap = 0.0;
        for (int idim=0; idim<3; idim++)
          {

            W.R[iat][idim] = r0[idim]+delta;
            W.update();
            ValueType logpsi_p = Psi.evaluateLog(W);

            W.R[iat][idim] = r0[idim]-delta;
            W.update();
            ValueType logpsi_m = Psi.evaluateLog(W);
            lap += logpsi_m+logpsi_p;
            g0[idim] = logpsi_p-logpsi_m;
//#if defined(QMC_COMPLEX)
//      lap += std::log(psi_m) + std::log(psi_p);
//      g0[idim] = std::log(psi_p)-std::log(psi_m);
//#else
//      lap += std::log(std::abs(psi_m)) + std::log(abs(psi_p));
//      g0[idim] = std::log(std::abs(psi_p/psi_m));
//#endif
            W.R[iat] = r0;
          }
        G1[iat] = c1*g0;
        L1[iat] = c2*(lap-6.0*logpsi);
        cerr << "G1 = " << G1[iat] << endl;
        cerr << "L1 = " << L1[iat] << endl;
      }

    cout.precision(15);
    for (int iat=0; iat<nat; iat++)
      {
        cout.precision(15);
        cout << "For particle #" << iat << " at " << W.R[iat] << endl;
        cout << "Gradient      = " << setw(12) << G[iat] << endl
             << "  Finite diff = " << setw(12) << G1[iat] << endl
             << "  Error       = " << setw(12) << G[iat]-G1[iat] << endl << endl;
        cout << "Laplacian     = " << setw(12) << L[iat] << endl
             << "  Finite diff = " << setw(12) << L1[iat] << endl
             << "  Error       = " << setw(12) << L[iat]-L1[iat] << endl << endl;
      }


    makeGaussRandom(deltaR);
    //testing ratio alone
    for (int iat=0; iat<nat; iat++)
      {
        W.update();
        //ValueType psi_p = log(fabs(Psi.evaluate(W)));
        RealType psi_p = Psi.evaluateLog(W);
        RealType phase_p=Psi.getPhase();

        W.makeMove(iat,deltaR[iat]);
        W.update();
        RealType aratio = Psi.ratio(W,iat);
        W.rejectMove(iat);
        Psi.rejectMove(iat);

        W.R[iat] += deltaR[iat];
        W.update();
        //ValueType psi_m = log(fabs(Psi.evaluate(W)));
        RealType psi_m = Psi.evaluateLog(W);
        RealType phase_m=Psi.getPhase();

        RealType ratDiff=std::exp(psi_m-psi_p)*std::cos(phase_m-phase_p) ;
        cout << iat << " ratio " << aratio/ratDiff << " " << ratDiff << endl;
      }
  }

  void WaveFunctionTester::runRatioTest()
  {

    int nat = W.getTotalNum();
    ParticleSet::ParticleGradient_t Gp(nat), dGp(nat);
    ParticleSet::ParticleLaplacian_t Lp(nat), dLp(nat);

    bool checkHam=(checkHamPbyP == "yes");
    Tau=0.025;
    MCWalkerConfiguration::iterator it(W.begin()), it_end(W.end());
    while (it != it_end)
      {
        makeGaussRandom(deltaR);
        Walker_t::Buffer_t tbuffer;
        W.R = (**it).R+Tau*deltaR;
        (**it).R=W.R;
        W.update();
        RealType logpsi=Psi.registerData(W,tbuffer);
        RealType ene;
        if (checkHam)
          ene = H.registerData(W,tbuffer);
        else
          ene = H.evaluate(W);
        (*it)->DataSet=tbuffer;

        //RealType ene = H.evaluate(W);
        (*it)->resetProperty(logpsi,Psi.getPhase(),ene,0.0,0.0,1.0);
        H.saveProperty((*it)->getPropertyBase());
        ++it;

        app_log() << "  HamTest " << "  Total " <<  ene << endl;
        for (int i=0; i<H.sizeOfObservables(); i++)
          app_log() << "  HamTest " << H.getObservableName(i) << " " << H.getObservable(i) << endl;
      }

    cout << "  Update using drift " << endl;
    bool pbyp_mode=true;
    for (int iter=0; iter<4;++iter)
      {
        int iw=0;
        it=W.begin();
        while (it != it_end)
          {

            cout << "\nStart Walker " << iw++ << endl;
            Walker_t& thisWalker(**it);
            W.loadWalker(thisWalker,pbyp_mode);
            Walker_t::Buffer_t& w_buffer(thisWalker.DataSet);

            Psi.copyFromBuffer(W,w_buffer);
            H.copyFromBuffer(W,w_buffer);

            RealType eold(thisWalker.Properties(LOCALENERGY));
            RealType logpsi(thisWalker.Properties(LOGPSI));
            RealType emixed(eold), enew(eold);

            makeGaussRandom(deltaR);

            //mave a move
            RealType ratio_accum(1.0);
            for (int iat=0; iat<nat; iat++)
              {
                PosType dr(Tau*deltaR[iat]);

                PosType newpos(W.makeMove(iat,dr));

                //RealType ratio=Psi.ratio(W,iat,dGp,dLp);
                RealType ratio=Psi.ratio(W,iat,W.dG,W.dL);
                Gp = W.G + W.dG;
                //RealType enew = H.evaluatePbyP(W,iat);
                if (checkHam) enew = H.evaluatePbyP(W,iat);

                if (ratio > Random())
                  {
                    cout << " Accepting a move for " << iat << endl;
                    cout << " Energy after a move " << enew << endl;
                    W.G += W.dG;
                    W.L += W.dL;
                    W.acceptMove(iat);
                    Psi.acceptMove(W,iat);
                    if (checkHam) H.acceptMove(iat);
                    ratio_accum *= ratio;
                  }
                else
                  {
                    cout << " Rejecting a move for " << iat << endl;
                    W.rejectMove(iat);
                    Psi.rejectMove(iat);
                    //H.rejectMove(iat);
                  }
              }

            cout << " Energy after pbyp = " << H.getLocalEnergy() << endl;
            RealType newlogpsi_up = Psi.evaluateLog(W,w_buffer);
            W.saveWalker(thisWalker);
            RealType ene_up;
            if (checkHam)
              ene_up= H.evaluate(W,w_buffer);
            else
              ene_up = H.evaluate(W);

            Gp=W.G;
            Lp=W.L;
            W.R=thisWalker.R;
            W.update();
            RealType newlogpsi=Psi.evaluateLog(W);
            RealType ene = H.evaluate(W);
            thisWalker.resetProperty(newlogpsi,Psi.getPhase(),ene);
            //thisWalker.resetProperty(std::log(psi),Psi.getPhase(),ene);

            cout << iter << "  Energy by update = "<< ene_up << " " << ene << " "  << ene_up-ene << endl;
            cout << iter << " Ratio " << ratio_accum*ratio_accum
                 << " | " << std::exp(2.0*(newlogpsi-logpsi)) << " "
                 << ratio_accum*ratio_accum/std::exp(2.0*(newlogpsi-logpsi)) << endl
                 << " new log(psi) updated " << newlogpsi_up
                 << " new log(psi) calculated " << newlogpsi
                 << " old log(psi) " << logpsi << endl;

            cout << " Gradients " << endl;
            for (int iat=0; iat<nat; iat++)
              cout << W.G[iat]-Gp[iat] << W.G[iat] << endl; //W.G[iat] << G[iat] << endl;
            cout << " Laplacians " << endl;
            for (int iat=0; iat<nat; iat++)
              cout << W.L[iat]-Lp[iat] << " " << W.L[iat] << endl;

            ++it;
          }
      }

    cout << "  Update without drift : for VMC useDrift=\"no\"" << endl;
    for (int iter=0; iter<4;++iter)
      {
        it=W.begin();
        int iw=0;
        while (it != it_end)
          {

            cout << "\nStart Walker " << iw++ << endl;
            Walker_t& thisWalker(**it);
            W.loadWalker(thisWalker,pbyp_mode);
            Walker_t::Buffer_t& w_buffer(thisWalker.DataSet);
            Psi.copyFromBuffer(W,w_buffer);

            RealType eold(thisWalker.Properties(LOCALENERGY));
            RealType logpsi(thisWalker.Properties(LOGPSI));
            RealType emixed(eold), enew(eold);

            //mave a move
            RealType ratio_accum(1.0);

            for (int substep=0; substep<3; ++substep)
              {
                makeGaussRandom(deltaR);

                for (int iat=0; iat<nat; iat++)
                  {
                    PosType dr(Tau*deltaR[iat]);

                    PosType newpos(W.makeMove(iat,dr));

                    RealType ratio=Psi.ratio(W,iat);
                    RealType prob = ratio*ratio;
                    if (prob > Random())
                      {
                        cout << " Accepting a move for " << iat << endl;
                        W.acceptMove(iat);
                        Psi.acceptMove(W,iat);
                        ratio_accum *= ratio;
                      }
                    else
                      {
                        cout << " Rejecting a move for " << iat << endl;
                        W.rejectMove(iat);
                        Psi.rejectMove(iat);
                      }
                  }

                RealType logpsi_up = Psi.updateBuffer(W,w_buffer,false);
                W.saveWalker(thisWalker);

                RealType ene = H.evaluate(W);
                thisWalker.resetProperty(logpsi_up,Psi.getPhase(),ene);
              }

            Gp=W.G;
            Lp=W.L;

            W.update();
            RealType newlogpsi=Psi.evaluateLog(W);
            cout << iter << " Ratio " << ratio_accum*ratio_accum
                 << " | " << std::exp(2.0*(newlogpsi-logpsi)) << " "
                 << ratio_accum*ratio_accum/std::exp(2.0*(newlogpsi-logpsi)) << endl
                 << " new log(psi) " << newlogpsi
                 << " old log(psi) " << logpsi << endl;

            cout << " Gradients " << endl;
            for (int iat=0; iat<nat; iat++)
              {
                cout << W.G[iat]-Gp[iat] << W.G[iat] << endl; //W.G[iat] << G[iat] << endl;
              }
            cout << " Laplacians " << endl;
            for (int iat=0; iat<nat; iat++)
              {
                cout << W.L[iat]-Lp[iat] << " " << W.L[iat] << endl;
              }
            ++it;
          }
      }

    //for(it=W.begin();it != it_end; ++it)
    //{
    //  Walker_t& thisWalker(**it);
    //  Walker_t::Buffer_t& w_buffer((*it)->DataSet);
    //  w_buffer.rewind();
    //  W.updateBuffer(**it,w_buffer);
    //  RealType logpsi=Psi.updateBuffer(W,w_buffer,true);
    //}


  }

  void WaveFunctionTester::runRatioTest2()
  {

    int nat = W.getTotalNum();
    ParticleSet::ParticleGradient_t Gp(nat), dGp(nat);
    ParticleSet::ParticleLaplacian_t Lp(nat), dLp(nat);

    Tau=0.025;
    MCWalkerConfiguration::iterator it(W.begin()), it_end(W.end());
    for (; it != it_end; ++it)
      {
        makeGaussRandom(deltaR);
        Walker_t::Buffer_t tbuffer;
        (**it).R  +=  Tau*deltaR;
        W.loadWalker(**it,true);
        RealType logpsi=Psi.registerData(W,tbuffer);
        RealType ene = H.evaluate(W);
        (*it)->DataSet=tbuffer;

        //RealType ene = H.evaluate(W);
        (*it)->resetProperty(logpsi,Psi.getPhase(),ene,0.0,0.0,1.0);
        H.saveProperty((*it)->getPropertyBase());

        app_log() << "  HamTest " << "  Total " <<  ene << endl;
        for (int i=0; i<H.sizeOfObservables(); i++)
          app_log() << "  HamTest " << H.getObservableName(i) << " " << H.getObservable(i) << endl;
      }

    for (int iter=0; iter<20;++iter)
      {
        int iw=0;
        it=W.begin();
        //while(it != it_end)
        for (; it != it_end; ++it)
          {
            cout << "\nStart Walker " << iw++ << endl;
            Walker_t& thisWalker(**it);
            W.loadWalker(thisWalker,true);
            Walker_t::Buffer_t& w_buffer(thisWalker.DataSet);
            Psi.copyFromBuffer(W,w_buffer);

            RealType eold(thisWalker.Properties(LOCALENERGY));
            RealType logpsi(thisWalker.Properties(LOGPSI));
            RealType emixed(eold), enew(eold);

            makeGaussRandom(deltaR);

            //mave a move
            RealType ratio_accum(1.0);
            for (int iat=0; iat<nat; iat++)
              {
                GradType grad_now=Psi.evalGrad(W,iat), grad_new;
                PosType dr(Tau*deltaR[iat]);
                PosType newpos(W.makeMove(iat,dr));

                RealType ratio2 = Psi.ratioGrad(W,iat,grad_new);
                W.rejectMove(iat);
                Psi.rejectMove(iat);

                newpos=W.makeMove(iat,dr);
                RealType ratio1 = Psi.ratio(W,iat);
                W.rejectMove(iat);
                cout << " ratio1 = " << ratio1 << " ration2 = " << ratio2 << endl;
              }
          }
      }

    //for(it=W.begin();it != it_end; ++it)
    //{
    //  Walker_t& thisWalker(**it);
    //  Walker_t::Buffer_t& w_buffer((*it)->DataSet);
    //  w_buffer.rewind();
    //  W.updateBuffer(**it,w_buffer);
    //  RealType logpsi=Psi.updateBuffer(W,w_buffer,true);
    //}


  }


  void WaveFunctionTester::runGradSourceTest()
  {
    ParticleSetPool::PoolType::iterator p;
    for (p=PtclPool.getPool().begin(); p != PtclPool.getPool().end(); p++)
      app_log() << "ParticelSet = " << p->first << endl;

    // Find source ParticleSet
    ParticleSetPool::PoolType::iterator pit(PtclPool.getPool().find(sourceName));
    app_log() << pit->first << endl;
    // if(pit == PtclPool.getPool().end())
    //   APP_ABORT("Unknown source \"" + sourceName + "\" WaveFunctionTester.");

    ParticleSet& source = *((*pit).second);

    IndexType nskipped = 0;
    RealType sig2Enloc=0, sig2Drift=0;
    RealType delta = 0.00001;
    RealType delta2 = 2*delta;
    ValueType c1 = 1.0/delta/2.0;
    ValueType c2 = 1.0/delta/delta;

    int nat = W.getTotalNum();

    ParticleSet::ParticlePos_t deltaR(nat);
    MCWalkerConfiguration::PropertyContainer_t Properties;
    //pick the first walker
    MCWalkerConfiguration::Walker_t* awalker = *(W.begin());

    //copy the properties of the working walker
    Properties = awalker->Properties;

    //sample a new walker configuration and copy to ParticleSet::R
    //makeGaussRandom(deltaR);

    W.R = awalker->R;

    //W.R += deltaR;

    W.update();
    //ValueType psi = Psi.evaluate(W);
    ValueType logpsi = Psi.evaluateLog(W);
    RealType eloc=H.evaluate(W);

    app_log() << "  HamTest " << "  Total " <<  eloc << endl;
    for (int i=0; i<H.sizeOfObservables(); i++)
      app_log() << "  HamTest " << H.getObservableName(i) << " " << H.getObservable(i) << endl;

    //RealType psi = Psi.evaluateLog(W);
    ParticleSet::ParticleGradient_t G(nat), G1(nat);
    ParticleSet::ParticleLaplacian_t L(nat), L1(nat);
    G = W.G;
    L = W.L;

    for (int isrc=0; isrc < 1/*source.getTotalNum()*/; isrc++)
      {
        TinyVector<ParticleSet::ParticleGradient_t, OHMMS_DIM> grad_grad;
        TinyVector<ParticleSet::ParticleLaplacian_t,OHMMS_DIM> lapl_grad;
        TinyVector<ParticleSet::ParticleGradient_t, OHMMS_DIM> grad_grad_FD;
        TinyVector<ParticleSet::ParticleLaplacian_t,OHMMS_DIM> lapl_grad_FD;
        for (int dim=0; dim<OHMMS_DIM; dim++)
          {
            grad_grad[dim].resize(nat);
            lapl_grad[dim].resize(nat);
            grad_grad_FD[dim].resize(nat);
            lapl_grad_FD[dim].resize(nat);
          }
        Psi.evaluateLog(W);
        GradType grad_log = Psi.evalGradSource(W, source, isrc, grad_grad, lapl_grad);
        ValueType log = Psi.evaluateLog(W);
        //grad_log = Psi.evalGradSource (W, source, isrc);

        for (int iat=0; iat<nat; iat++)
          {
            PosType r0 = W.R[iat];
            GradType gFD[OHMMS_DIM];
            GradType lapFD = ValueType();
            for (int eldim=0; eldim<3; eldim++)
              {
                W.R[iat][eldim] = r0[eldim]+delta;
                W.update();
                ValueType log_p = Psi.evaluateLog(W);
                GradType gradlogpsi_p =  Psi.evalGradSource(W, source, isrc);
                W.R[iat][eldim] = r0[eldim]-delta;
                W.update();
                ValueType log_m = Psi.evaluateLog(W);
                GradType gradlogpsi_m = Psi.evalGradSource(W, source, isrc);
                lapFD    += gradlogpsi_m + gradlogpsi_p;
                gFD[eldim] = gradlogpsi_p - gradlogpsi_m;
                W.R[iat] = r0;
                W.update();
                //Psi.evaluateLog(W);
              }
            for (int iondim=0; iondim<OHMMS_DIM; iondim++)
              {
                for (int eldim=0; eldim<OHMMS_DIM; eldim++)
                  grad_grad_FD[iondim][iat][eldim] = c1*gFD[eldim][iondim];
                lapl_grad_FD[iondim][iat] = c2*(lapFD[iondim]-6.0*grad_log[iondim]);
              }
          }
        cout.precision(15);
        for (int dimsrc=0; dimsrc<OHMMS_DIM; dimsrc++)
          {
            for (int iat=0; iat<nat; iat++)
              {
                cout.precision(15);
                cout << "For particle #" << iat << " at " << W.R[iat] << endl;
                cout << "Gradient      = " << setw(12) << grad_grad[dimsrc][iat] << endl
                     << "  Finite diff = " << setw(12) << grad_grad_FD[dimsrc][iat] << endl
                     << "  Error       = " << setw(12)
                     <<  grad_grad_FD[dimsrc][iat] - grad_grad[dimsrc][iat] << endl << endl;
                cout << "Laplacian     = " << setw(12) << lapl_grad[dimsrc][iat] << endl
                     << "  Finite diff = " << setw(12) << lapl_grad_FD[dimsrc][iat] << endl
                     << "  Error       = " << setw(12)
                     << lapl_grad_FD[dimsrc][iat] - lapl_grad[dimsrc][iat] << endl << endl;
              }
          }
      }
  }


  void WaveFunctionTester::runZeroVarianceTest()
  {
    ParticleSetPool::PoolType::iterator p;
    for (p=PtclPool.getPool().begin(); p != PtclPool.getPool().end(); p++)
      app_log() << "ParticelSet = " << p->first << endl;

    // Find source ParticleSet
    ParticleSetPool::PoolType::iterator pit(PtclPool.getPool().find(sourceName));
    app_log() << pit->first << endl;
    // if(pit == PtclPool.getPool().end())
    //   APP_ABORT("Unknown source \"" + sourceName + "\" WaveFunctionTester.");

    ParticleSet& source = *((*pit).second);

    int nat = W.getTotalNum();

    ParticleSet::ParticlePos_t deltaR(nat);
    MCWalkerConfiguration::PropertyContainer_t Properties;
    //pick the first walker
    MCWalkerConfiguration::Walker_t* awalker = *(W.begin());

    //copy the properties of the working walker
    Properties = awalker->Properties;

    //sample a new walker configuration and copy to ParticleSet::R
    //makeGaussRandom(deltaR);

    W.R = awalker->R;

    //W.R += deltaR;

    W.update();
    //ValueType psi = Psi.evaluate(W);
    ValueType logpsi = Psi.evaluateLog(W);
    RealType eloc=H.evaluate(W);

    //RealType psi = Psi.evaluateLog(W);
    ParticleSet::ParticleGradient_t G(nat), G1(nat);
    ParticleSet::ParticleLaplacian_t L(nat), L1(nat);
    G = W.G;
    L = W.L;

    PosType r1(5.0, 2.62, 2.55);
    W.R[1] = PosType(4.313, 5.989, 4.699);
    W.R[2] = PosType(5.813, 4.321, 4.893);
    W.R[3] = PosType(4.002, 5.502, 5.381);
    W.R[4] = PosType(5.901, 5.121, 5.311);
    W.R[5] = PosType(5.808, 4.083, 5.021);
    W.R[6] = PosType(4.750, 5.810, 4.732);
    W.R[7] = PosType(4.690, 5.901, 4.989);
    for (int i=1; i<8; i++)
      W.R[i] -= PosType(2.5, 2.5, 2.5);


    FILE *fout = fopen("ZVtest.dat", "w");

    TinyVector<ParticleSet::ParticleGradient_t, OHMMS_DIM> grad_grad;
    TinyVector<ParticleSet::ParticleLaplacian_t,OHMMS_DIM> lapl_grad;
    TinyVector<ParticleSet::ParticleGradient_t, OHMMS_DIM> grad_grad_FD;
    TinyVector<ParticleSet::ParticleLaplacian_t,OHMMS_DIM> lapl_grad_FD;
    for (int dim=0; dim<OHMMS_DIM; dim++)
      {
        grad_grad[dim].resize(nat);
        lapl_grad[dim].resize(nat);
        grad_grad_FD[dim].resize(nat);
        lapl_grad_FD[dim].resize(nat);
      }

    for (r1[0]=0.0; r1[0]<5.0; r1[0]+=1.0e-4)
      {
        W.R[0] = r1;
        fprintf(fout, "%1.8e %1.8e %1.8e ", r1[0], r1[1], r1[2]);
        ValueType log = Psi.evaluateLog(W);
        ValueType psi = std::cos(Psi.getPhase())*std::exp(log);//*W.PropertyList[SIGN];
        double E = H.evaluate(W);
        //double KE = E - W.PropertyList[LOCALPOTENTIAL];
        double KE = -0.5*(Sum(W.L) + Dot(W.G,W.G));
        fprintf(fout, "%16.12e %16.12f ", psi, KE);
        for (int isrc=0; isrc < source.getTotalNum(); isrc++)
          {
            GradType grad_log = Psi.evalGradSource(W, source, isrc, grad_grad, lapl_grad);
            for (int dim=0; dim<OHMMS_DIM; dim++)
              {
                double ZV = 0.5*Sum(lapl_grad[dim]) + Dot(grad_grad[dim], W.G);
                fprintf(fout, "%16.12e %16.12e ", ZV, grad_log[dim]);
              }
          }
        fprintf(fout, "\n");
      }
    fclose(fout);
  }



  bool
  WaveFunctionTester::put(xmlNodePtr q)
  {
    myNode = q;
    return putQMCInfo(q);
  }

  void WaveFunctionTester::runDerivTest()
  {
    app_log()<<" Testing derivatives"<<endl;
    IndexType nskipped = 0;
    RealType sig2Enloc=0, sig2Drift=0;
    RealType delta = 0.00001;
    RealType delta2 = 2*delta;
    ValueType c1 = 1.0/delta/2.0;
    ValueType c2 = 1.0/delta/delta;

    int nat = W.getTotalNum();

    ParticleSet::ParticlePos_t deltaR(nat);
    MCWalkerConfiguration::PropertyContainer_t Properties;
    //pick the first walker
    MCWalkerConfiguration::Walker_t* awalker = *(W.begin());

    //copy the properties of the working walker
    Properties = awalker->Properties;

    //sample a new walker configuration and copy to ParticleSet::R
    //makeGaussRandom(deltaR);

    W.R = awalker->R;

    //W.R += deltaR;

    W.update();
    //ValueType psi = Psi.evaluate(W);
    ValueType logpsi = Psi.evaluateLog(W);
    RealType eloc=H.evaluate(W);

    app_log() << "  HamTest " << "  Total " <<  eloc << endl;
    for (int i=0; i<H.sizeOfObservables(); i++)
      app_log() << "  HamTest " << H.getObservableName(i) << " " << H.getObservable(i) << endl;

    //RealType psi = Psi.evaluateLog(W);
    ParticleSet::ParticleGradient_t G(nat), G1(nat);
    ParticleSet::ParticleLaplacian_t L(nat), L1(nat);
    G = W.G;
    L = W.L;
    cout<<"Gradients"<<endl;
    for (int iat=0;iat<W.R.size();iat++)
      {
        for (int i=0; i<3 ; i++) cout<<W.G[iat][i]<<"  ";
        cout<<endl;
      }

    opt_variables_type wfVars,wfvar_prime;
    Psi.checkInVariables(wfVars);
    Psi.checkOutVariables(wfVars);
    wfvar_prime= wfVars;
    wfVars.print(cout);
    int Nvars= wfVars.size();
    vector<RealType> Dsaved(Nvars);
    vector<RealType> HDsaved(Nvars);
    vector<RealType> PGradient(Nvars);
    vector<RealType> HGradient(Nvars);
    Psi.evaluateDerivatives(W, wfVars, Dsaved, HDsaved);
    RealType FiniteDiff = 1e-5;

    QMCTraits::RealType dh=1.0/(2.0*FiniteDiff);
    for (int i=0; i<Nvars ; i++)
      {
        for (int j=0; j<Nvars; j++) wfvar_prime[j]=wfVars[j];
        wfvar_prime[i] = wfVars[i]+ FiniteDiff;
//     Psi.checkOutVariables(wfvar_prime);
        Psi.resetParameters(wfvar_prime);
        Psi.reset();
        W.update();
        W.G=0;
        W.L=0;
        RealType logpsiPlus = Psi.evaluateLog(W);
        RealType elocPlus=H.evaluate(W);


        wfvar_prime[i] = wfVars[i]- FiniteDiff;
//     Psi.checkOutVariables(wfvar_prime);
        Psi.resetParameters(wfvar_prime);
        Psi.reset();
        W.update();
        W.G=0;
        W.L=0;
        RealType logpsiMinus = Psi.evaluateLog(W);
        RealType elocMinus =H.evaluate(W);

        PGradient[i]= (logpsiPlus-logpsiMinus)*dh;
        HGradient[i]= (elocPlus-elocMinus)*dh;
      }
    cout<<endl<<"Deriv  Numeric Analytic"<<endl;
    for (int i=0; i<Nvars ; i++) cout<<i<<"  "<<PGradient[i]<<"  "<<Dsaved[i] <<endl;
    cout<<endl<<"Hderiv  Numeric Analytic"<<endl;
    for (int i=0; i<Nvars ; i++) cout<<i <<"  "<<HGradient[i]<<"  "<<HDsaved[i]<<endl;

  }



    void WaveFunctionTester::runwftricks()
    {
      vector<OrbitalBase*>& Orbitals=Psi.getOrbitals();
      app_log()<<" Total of "<<Orbitals.size()<<" orbitals."<<endl;
      int SDindex(0);
      for (int i=0;i<Orbitals.size();i++)
        if ("SlaterDet"==Orbitals[i]->OrbitalName) SDindex=i;
      
      SPOSetBasePtr Phi= dynamic_cast<SlaterDet *>(Orbitals[SDindex])->getPhi();
      int NumOrbitals=Phi->getBasisSetSize();
      app_log()<<"Basis set size: "<<NumOrbitals<<endl;
      
      vector<int> SPONumbers(0,0);
      vector<int> irrepRotations(0,0);
      vector<int> Grid(0,0);
      
      xmlNodePtr kids=myNode->children;
      
      string doProj("yes");
      string doRotate("yes");
      string sClass("C2V");
      
      ParameterSet aAttrib;
      aAttrib.add(doProj,"projection","string");
      aAttrib.add(doRotate,"rotate","string");
      aAttrib.add(sClass,"class","string");
      aAttrib.put(myNode);
     
      while(kids != NULL) 
      {
        string cname((const char*)(kids->name));
        if(cname == "orbitals") 
        {
          putContent(SPONumbers,kids);
        }
        else if(cname == "representations") 
        {
          putContent(irrepRotations,kids);
        }
        else if(cname=="grid")
          putContent(Grid,kids);
        
      kids=kids->next;
      }
      
      ParticleSet::ParticlePos_t R_cart(1);
      R_cart.setUnit(PosUnit::CartesianUnit);
      ParticleSet::ParticlePos_t R_unit(1);
      R_unit.setUnit(PosUnit::LatticeUnit);
      
//       app_log()<<" My crystals basis set is:"<<endl;
      vector<vector<RealType> > BasisMatrix(3, vector<RealType>(3,0.0));
      
      for (int i=0;i<3;i++)
      {
        R_unit[0][0]=0;
        R_unit[0][1]=0;
        R_unit[0][2]=0;
        R_unit[0][i]=1;
        W.convert2Cart(R_unit,R_cart);
//         app_log()<<"basis_"<<i<<":  ("<<R_cart[0][0]<<", "<<R_cart[0][1]<<", "<<R_cart[0][2]<<")"<<endl;
        for (int j=0;j<3;j++) BasisMatrix[j][i]=R_cart[0][j];
      }

      int Nrotated(SPONumbers.size());
      app_log()<<" Projected orbitals: ";
      for(int i=0;i<Nrotated;i++) app_log()<< SPONumbers[i] <<" ";
      app_log()<<endl;
      //indexing trick
      for(int i=0;i<Nrotated;i++) SPONumbers[i]-=1;
      
      SymmetryBuilder SO(sClass,myNode);
      SymmetryGroup symOp(*SO.getSymmetryGroup());
      
//       SO.changeBasis(InverseBasisMatrix);
      
      OrbitalSetTraits<ValueType>::ValueVector_t values;
      values.resize(NumOrbitals);
      
      RealType overG0(1.0/Grid[0]);
      RealType overG1(1.0/Grid[1]);
      RealType overG2(1.0/Grid[2]);
      RealType overNpoints=  overG0*overG1*overG2;

      vector<RealType> NormPhi(Nrotated, 0.0);
      
      int totsymops = symOp.getSymmetriesSize();
      Matrix<RealType> SymmetryOrbitalValues;
      SymmetryOrbitalValues.resize(Nrotated,totsymops);
       
      int ctabledim = symOp.getClassesSize();
      Matrix<double> projs(Nrotated,ctabledim);
      Matrix<double> orthoProjs(Nrotated,Nrotated);
      
      vector<RealType> brokenSymmetryCharacter(totsymops);
      for(int k=0;k<Nrotated;k++) for(int l=0;l<totsymops;l++) 
        brokenSymmetryCharacter[l] += double(irrepRotations[k]-1)/double(ctabledim)*symOp.getsymmetryCharacter(l,irrepRotations[k]-1);
      
      if ((doProj=="yes")||(doRotate=="yes"))
      {
        //Loop over grid
      for(int i=0;i<Grid[0];i++)
        for(int j=0;j<Grid[1];j++)
          for(int k=0;k<Grid[2];k++)
          {
            OrbitalSetTraits<ValueType>::ValueVector_t identityValues(values.size());
            //Loop over symmetry classes and small group operators
            for(int l=0;l<totsymops;l++)
            {
                R_unit[0][0]=overG0*RealType(i); R_cart[0][0]=0;
                R_unit[0][1]=overG1*RealType(j); R_cart[0][1]=0;
                R_unit[0][2]=overG2*RealType(k); R_cart[0][2]=0;
                
                for(int a=0; a<3; a++) for(int b=0;b<3;b++) R_cart[0][a]+=BasisMatrix[a][b]*R_unit[0][b];

                
                symOp.TransformSinglePosition(R_cart,l);
                W.R[0]=R_cart[0];
                for(int a=0;a<values.size();a++) values=0.0;
                //evaluate orbitals
                Phi->evaluate(W,0,values);
                
                if (l==0){
                  identityValues=values;
                }
                
                //now we have phi evaluated at the rotated/inverted/whichever coordinates
                for(int n=0;n<Nrotated;n++) 
                {
                  int N=SPONumbers[n];
                  RealType phi2 = values[N]*identityValues[N];
                  SymmetryOrbitalValues(n,l) += phi2;
                  NormPhi[n] += values[N]*values[N];
                }
                
                for(int n=0;n<Nrotated;n++) for(int p=0;p<Nrotated;p++)
                {
                  int N=SPONumbers[n];
                  int P=SPONumbers[p];
                  orthoProjs(n,p) += identityValues[N]*values[P]*brokenSymmetryCharacter[l];
                }
          }
      }
      for(int n=0;n<Nrotated;n++) for(int l=0;l<totsymops;l++) 
        SymmetryOrbitalValues(n,l)/= NormPhi[n];
      
      for(int n=0;n<Nrotated;n++) for(int l=0;l<Nrotated;l++) 
        orthoProjs(n,l) /= std::sqrt(NormPhi[n]*NormPhi[l]);
      
      if (true){
        app_log()<<endl;
        for(int n=0;n<Nrotated;n++) {
          for(int l=0;l<totsymops;l++) app_log()<<SymmetryOrbitalValues(n,l)<<" ";
          app_log()<<endl;
        }
      app_log()<<endl;
      }
      

      
      for(int n=0;n<Nrotated;n++)
      {
        if (false) app_log()<<" orbital #"<<SPONumbers[n]<<endl;
        for(int i=0;i<ctabledim;i++)
        {
          double proj(0);
          for(int j=0;j<totsymops;j++) proj+=symOp.getsymmetryCharacter(j,i)*SymmetryOrbitalValues(n,j);
          if (false) app_log()<<"  Rep "<<i<< ": "<<proj;
          projs(n,i)=proj<1e-4?0:proj;
        }
        if (false) app_log()<<endl;
      }
      
      if (true){
        app_log()<<"Printing Projection Matrix"<<endl;
        for(int n=0;n<Nrotated;n++) {
          for(int l=0;l<ctabledim;l++) app_log()<<projs(n,l)<<" ";
          app_log()<<endl;
        }
      app_log()<<endl;
      }
      if (true){
        app_log()<<"Printing Coefficient Matrix"<<endl;
        for(int n=0;n<Nrotated;n++) {
          for(int l=0;l<ctabledim;l++) app_log()<<std::sqrt(projs(n,l))<<" ";
          app_log()<<endl;
        }
      app_log()<<endl;
      }
      
      if (doRotate=="yes")
      {

        app_log()<<"Printing Broken Symmetry Projection Matrix"<<endl;
          for(int n=0;n<Nrotated;n++) {
            for(int l=0;l<Nrotated;l++) app_log()<<orthoProjs(n,l)<<" ";
            app_log()<<endl;
          } 
        
        char JOBU('A');
        char JOBVT('A');
        int vdim=Nrotated;
        Matrix<RealType> Sigma(vdim,vdim);
        Matrix<RealType> U(vdim,vdim);
        Matrix<RealType> VT(vdim,vdim);
        int lwork=8*Nrotated;
        vector<RealType> work(lwork,0);
        int info(0);
        
        dgesvd(&JOBU, &JOBVT, &vdim, &vdim,
            orthoProjs.data(), &vdim, Sigma.data(), U.data(),
            &vdim, VT.data(), &vdim, &(work[0]),
            &lwork, &info);
        
        app_log()<<"Printing Rotation Matrix"<<endl;
          for(int n=0;n<vdim;n++) {
            for(int l=0;l<vdim;l++) app_log()<<-VT(l,n)<<" ";
            app_log()<<endl;
          }  
          app_log()<<endl<<"Printing Eigenvalues"<<endl;
          for(int n=0;n<vdim;n++) app_log()<<Sigma[0][n]<<" ";
          app_log()<<endl;  
      }      
      }

    }
    
}

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$
 ***************************************************************************/
