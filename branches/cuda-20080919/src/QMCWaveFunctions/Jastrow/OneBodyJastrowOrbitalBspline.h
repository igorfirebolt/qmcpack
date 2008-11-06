#ifndef ONE_BODY_JASTROW_ORBITAL_BSPLINE_H
#define ONE_BODY_JASTROW_ORBITAL_BSPLINE_H

#include "Particle/DistanceTableData.h"
#include "Particle/DistanceTable.h"
#include "QMCWaveFunctions/Jastrow/OneBodyJastrowOrbital.h"
#include "QMCWaveFunctions/Jastrow/BsplineFunctor.h"
#include "QMCWaveFunctions/Jastrow/CudaSpline.h"
#include "QMCWaveFunctions/Jastrow/BsplineJastrowCuda.h"
#include "Configuration.h"

namespace qmcplusplus {

  class OneBodyJastrowOrbitalBspline : 
    public OneBodyJastrowOrbital<BsplineFunctor<OrbitalBase::RealType> > 
  {
  private:
    typedef float CudaReal;
    //typedef double CudaReal;

    int ROffset;
    vector<CudaSpline<CudaReal>*> GPUSplines, UniqueSplines;
    ParticleSet &ElecRef;
    cuda_vector<CudaReal> L, Linv;

    // Holds center positions
    cuda_vector<CudaReal> C;

    cuda_vector<CudaReal*> RlistGPU, UpdateListGPU;
    cuda_vector<CudaReal> SumGPU, RnewGPU, GradLaplGPU;

    host_vector<CudaReal*> RlistHost, UpdateListHost;
    host_vector<CudaReal> SumHost, RnewHost, GradLaplHost;
    int NumCenterGroups, NumElecGroups;
    vector<int> CenterFirst, CenterLast;

    host_vector<CudaReal*> NL_SplineCoefsListHost;
    cuda_vector<CudaReal*> NL_SplineCoefsListGPU;
    host_vector<NLjobGPU<CudaReal> > NL_JobListHost;
    cuda_vector<NLjobGPU<CudaReal> > NL_JobListGPU;
    host_vector<int> NL_NumCoefsHost, NL_NumQuadPointsHost;
    cuda_vector<int> NL_NumCoefsGPU,  NL_NumQuadPointsGPU;
    host_vector<CudaReal> NL_rMaxHost, NL_QuadPointsHost, NL_RatiosHost;
    cuda_vector<CudaReal> NL_rMaxGPU,  NL_QuadPointsGPU,  NL_RatiosGPU;

    int N;
  public:
    typedef BsplineFunctor<OrbitalBase::RealType> FT;
    typedef ParticleSet::Walker_t     Walker_t;

    
    void checkInVariables(opt_variables_type& active);
    void addFunc(int ib, FT* j);
    void recompute(vector<Walker_t*> &walkers);
    void reserve (PointerPool<cuda_vector<CudaRealType> > &pool);
    void addLog (vector<Walker_t*> &walkers, vector<RealType> &logPsi);
    void update (vector<Walker_t*> &walkers, int iat);
    void ratio (vector<Walker_t*> &walkers, int iat, vector<PosType> &new_pos,
		vector<ValueType> &psi_ratios,	vector<GradType>  &grad,
		vector<ValueType> &lapl);
    void gradLapl (vector<Walker_t*> &walkers, GradMatrix_t &grads,
		   ValueMatrix_t &lapl);
    void NLratios (vector<Walker_t*> &walkers,  vector<NLjob> &jobList,
		   vector<PosType> &quadPoints, vector<ValueType> &psi_ratios);

    OneBodyJastrowOrbitalBspline(ParticleSet &centers, ParticleSet& elecs) :
      OneBodyJastrowOrbital<BsplineFunctor<OrbitalBase::RealType> > (centers,elecs),
      ElecRef(elecs)
    {
      NumElecGroups = elecs.groups();
      SpeciesSet &sSet = centers.getSpeciesSet();
      NumCenterGroups = sSet.getTotalNum();
      //      NumCenterGroups = centers.groups();
      // cerr << "NumCenterGroups = " << NumCenterGroups << endl;
      GPUSplines.resize(NumCenterGroups,0);
      host_vector<CudaReal> LHost(OHMMS_DIM*OHMMS_DIM), 
	LinvHost(OHMMS_DIM*OHMMS_DIM);
      for (int i=0; i<OHMMS_DIM; i++)
	for (int j=0; j<OHMMS_DIM; j++) {
	  LHost[OHMMS_DIM*i+j]    = (CudaReal)elecs.Lattice.a(i)[j];
	  LinvHost[OHMMS_DIM*i+j] = (CudaReal)elecs.Lattice.b(i)[j];
	}
      L = LHost;
      Linv = LinvHost;
      N = elecs.getTotalNum();

      // Copy center positions to GPU, sorting by GroupID
      host_vector<CudaReal> C_host(OHMMS_DIM*centers.getTotalNum());
      int index=0;
      for (int cgroup=0; cgroup<NumCenterGroups; cgroup++) {
	CenterFirst.push_back(index);
	for (int i=0; i<centers.getTotalNum(); i++) {
	  if (centers.GroupID[i] == cgroup) {
	    for (int dim=0; dim<OHMMS_DIM; dim++) 
	      C_host[OHMMS_DIM*index+dim] = centers.R[i][dim];
	    index++;
	  }
	}
	CenterLast.push_back(index-1);
      }

      // host_vector<CudaReal> C_host(OHMMS_DIM*centers.getTotalNum());
      // for (int i=0; i<centers.getTotalNum(); i++) 
      // 	for (int dim=0; dim<OHMMS_DIM; dim++)
      // 	  C_host[OHMMS_DIM*i+dim] = centers.R[i][dim];
      C = C_host;

    }
  };
}


#endif
