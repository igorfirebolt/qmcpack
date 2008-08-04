//////////////////////////////////////////////////////////////////
// (c) Copyright 2005- by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim
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
#ifndef QMCPLUSPLUS_COSTFUNCTIONBASE_H
#define QMCPLUSPLUS_COSTFUNCTIONBASE_H

#include "Configuration.h"
#include "Optimize/OptimizeBase.h"
#include "QMCHamiltonians/QMCHamiltonian.h"
#include "QMCWaveFunctions/TrialWaveFunction.h"
#include "Message/MPIObjectBase.h"
#include <deque>
#include <set>

namespace qmcplusplus {

  class MCWalkerConfiguration;

  /** @ingroup QMCDrivers
   * @brief Implements wave-function optimization
   *
   * Optimization by correlated sampling method with configurations 
   * generated from VMC.
   */

  class QMCCostFunctionBase: public CostFunctionBase<QMCTraits::RealType>, public MPIObjectBase
  {
  public:

    enum FieldIndex_OPT {LOGPSI_FIXED=0, LOGPSI_FREE=1, ENERGY_TOT=2, ENERGY_FIXED=3, ENERGY_NEW=4, REWEIGHT=5};
    enum SumIndex_OPT {SUM_E_BARE=0, SUM_ESQ_BARE, SUM_ABSE_BARE,
      SUM_E_WGT, SUM_ESQ_WGT, SUM_ABSE_WGT, SUM_WGT, SUM_WGTSQ,
      SUM_INDEX_SIZE};


    ///Constructor.
    QMCCostFunctionBase(MCWalkerConfiguration& w, TrialWaveFunction& psi, QMCHamiltonian& h);
    
    ///Destructor
    virtual ~QMCCostFunctionBase();

    ///process xml node
    bool put(xmlNodePtr cur);
    ///assign optimization parameter i
    Return_t& Params(int i) { return OptVariables[i];}
    ///return optimization parameter i
    Return_t Params(int i) const { return OptVariables[i]; }
    ///return the cost value for CGMinimization
    Return_t Cost();
    ///return the number of optimizable parameters
    inline int NumParams() { return OptVariables.size(); }
    ///return the number of optimizable parameters
    inline int getNumSamples() { return NumSamples; }
    ///dump the current parameters and other report
    void Report();
    ///report  parameters at the end
    void reportParameters();

    ///return the counter which keeps track of optimization steps
    inline int getReportCounter() const 
    {
      return ReportCounter;
    }

    void setWaveFunctionNode(xmlNodePtr cur) { m_wfPtr=cur; }

    //void getConfigurations(vector<string>& ConfigFile, int partid, int nparts);

    void setTargetEnergy(Return_t et);

    void setRootName(const string& aroot) { RootName=aroot;}

    void setStream(ostream* os) { msg_stream = os;}

    void addCoefficients(xmlXPathContextPtr acontext, const char* cname);

    /** implement the virtual function 
     * @param x0 current parameters
     * @param gr gradients or conjugate gradients
     * @param dl return the displacelement to minimize the cost function
     * @param val_proj projected cost
     *
     * If successful, any optimization object updates the parameters by x0 + dl*gr
     * and proceeds with a new step.
     */
    bool lineoptimization(const vector<double>& x0, const vector<double>& gr, Return_t val0, 
        Return_t& dl, Return_t& val_proj, Return_t& lambda_max);

    virtual void getConfigurations(const string& aroot)=0;

    virtual void checkConfigurations()=0;

  protected:

    ///walker ensemble
    MCWalkerConfiguration& W;

    ///trial function
    TrialWaveFunction& Psi;

    ///Hamiltonian
    QMCHamiltonian& H;

    ///boolean to turn on/off the psi^2/psi^2_old for correlated sampling
    bool UseWeight;
    ///bollean to turn on/off the use of anonymous buffer
    bool needBuffering;
    ///bollean to turn on/off the use of anonymous buffer for the ratio
    bool hamiltonianNeedRatio;
    /** |E-E_T|^PowerE is used for the cost function
     *
     * default PowerE=1
     */
    int PowerE;
    ///number of times cost function evaluated
    int NumCostCalls;
    ///total number of samples to use in correlated sampling
    int NumSamples;
    ///total number of optimizable variables
    int NumOptimizables;
    ///counter for output
    int ReportCounter;
    ///weights for energy and variance in the cost function
    Return_t w_en, w_var, w_abs;
    ///value of the cost function
    Return_t CostValue;
    ///target energy
    Return_t Etarget;
    ///real target energy with the Correlation Factor
    Return_t EtargetEff;
    ///effective number of walkers 
    Return_t NumWalkersEff;
    ///fraction of the number of walkers below which the costfunction becomes invalid
    Return_t MinNumWalkers;
    ///maximum weight beyond which the weight is set to 1
    Return_t MaxWeight;

    ///current Average
    Return_t curAvg;
    ///current Variance
    Return_t curVar;
    ///current weighted average (correlated sampling) 
    Return_t curAvg_w;
    ///current weighted variance (correlated sampling)
    Return_t curVar_w;
    ///current variance of SUM_ABSE_WGT/SUM_WGT
    Return_t curVar_abs;

    /** Rescaling factor to correct the target energy Etarget=(1+CorrelationFactor)*Etarget
     *
     * default CorrelationFactor=0.0;
     */
    Return_t CorrelationFactor;
    ///list of optimizables
    opt_variables_type OptVariables;
    /** full list of optimizables
     *
     * The size of OptVariablesForPsi is equal to or larger than
     * that of OptVariables due to the dependent variables.
     * This is used for TrialWaveFunction::resetParameters and
     * is normally the same as OptVariables.
     */
    opt_variables_type OptVariablesForPsi;
    /** index mapping for <equal> constraints
     *
     * - equalVarMap[i][0] : index in OptVariablesForPsi
     * - equalVarMap[i][1] : index in OptVariables
     */
    std::vector<TinyVector<int,2> > equalVarMap;
    /** index mapping for <negate> constraints
     *
     * - negateVarMap[i][0] : index in OptVariablesForPsi
     * - negateVarMap[i][1] : index in OptVariables
     */
    ///index mapping for <negative> constraints
    std::vector<TinyVector<int,2> > negateVarMap;
    ///stream to which progress is sent
    ostream* msg_stream;
    ///xml node to be dumped
    xmlNodePtr m_wfPtr;
    ///document node to be dumped
    xmlDocPtr m_doc_out;
    ///parameters to be updated
    std::map<string,xmlNodePtr> paramNodes;
    ///coefficients to be updated
    std::map<string,xmlNodePtr> coeffNodes;
    ///attributes to be updated
    std::map<string,pair<xmlNodePtr,string> > attribNodes;
    ///string for the file root
    string RootName;
    ///Hamiltonians that depend on the optimization: KE
    QMCHamiltonian H_KE;

    /** Sum of energies and weights for averages
     *
     * SumValues[k] where k is one of SumIndex_opt
     */
    std::vector<Return_t> SumValue;
    /** Saved properties of all the walkers
     *
     * Records(iw,field_id) returns the field_id value of the iw-th walker
     * field_id is one of FieldIndex_opt
     */
    Matrix<Return_t> Records;
    /** Fixed  Gradients , \f$\nabla\ln\Psi\f$, components */
    ParticleSet::ParticleGradient_t dG;
    /** Fixed  Laplacian , \f$\nabla^2\ln\Psi\f$, components */
    ParticleSet::ParticleLaplacian_t dL;
    ///stream for debug
    ostream* debug_stream;

    bool checkParameters();
    void updateXmlNodes();

    virtual void resetPsi()=0;
    virtual Return_t correlatedSampling()=0;
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1792 $   $Date: 2007-02-21 17:44:40 -0600 (Wed, 21 Feb 2007) $
 * $Id: QMCCostFunctionBase.h 1792 2007-02-21 23:44:40Z jnkim $ 
 ***************************************************************************/