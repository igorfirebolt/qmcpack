/////////////////////////////////////////////////////////////////
// (c) Copyright 2005-  by Jeongnim Kim
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
#include "OhmmsPETE/OhmmsMatrix.h"
#include "QMCWaveFunctions/Jastrow/ThreeBodyBlockSparse.h"
#include "Particle/DistanceTable.h"
#include "Particle/DistanceTableData.h"
#include "Numerics/DeterminantOperators.h"
#include "Numerics/OhmmsBlas.h"
#include "Numerics/BlockMatrixFunctions.h"
#include "Numerics/LibxmlNumericIO.h"
#include "Utilities/IteratorUtility.h"
using namespace std;
#include "Numerics/MatrixOperators.h"
#include "OhmmsData/AttributeSet.h"

namespace qmcplusplus {

  ThreeBodyBlockSparse::ThreeBodyBlockSparse(ParticleSet& ions, ParticleSet& els): 
    CenterRef(ions), GeminalBasis(0), IndexOffSet(1), ID_Lambda("j3g"), SameBlocksForGroup(true)
    {
      d_table = DistanceTable::add(ions,els);
      NumPtcls=els.getTotalNum();
    }

  ThreeBodyBlockSparse::~ThreeBodyBlockSparse() 
  {
    delete_iter(LambdaBlocks.begin(), LambdaBlocks.end());
  }

  OrbitalBase::ValueType 
  ThreeBodyBlockSparse::evaluateLog(ParticleSet& P,
		                 ParticleSet::ParticleGradient_t& G, 
		                 ParticleSet::ParticleLaplacian_t& L) {

    GeminalBasis->evaluateForWalkerMove(P);

    //this could be better but it is used only sparsely
    //MatrixOperators::product(GeminalBasis->Y, Lambda, V);
    MatrixOperators::product(GeminalBasis->Y, Lambda, V, BlockOffset);

    Uk=0.0;

    LogValue=ValueType();
    for(int i=0; i< NumPtcls-1; i++) {
      const BasisSetType::RealType* restrict yptr=GeminalBasis->Y[i];
      for(int j=i+1; j<NumPtcls; j++) {
        RealType x= dot(V[j],yptr,BasisSize);
        LogValue += x;
        Uk[i]+= x;
        Uk[j]+= x;
      }
    }

    for(int i=0; i<NumPtcls; i++)  {
      const BasisSetType::GradType* restrict dptr=GeminalBasis->dY[i];
      const BasisSetType::ValueType* restrict d2ptr=GeminalBasis->d2Y[i];
      const BasisSetType::ValueType* restrict vptr=V[0];
      BasisSetType::GradType grad(0.0);
      BasisSetType::ValueType lap(0.0);
      for(int j=0; j<NumPtcls; j++, vptr+=BasisSize) {
        if(j!=i) {
          grad += dot(vptr,dptr,BasisSize);
          lap +=  dot(vptr,d2ptr,BasisSize);
        }
      }
      G(i)+=grad;
      L(i)+=lap;
    }

    return LogValue;
  }

  OrbitalBase::ValueType 
  ThreeBodyBlockSparse::ratio(ParticleSet& P, int iat) {
    GeminalBasis->evaluateForPtclMove(P,iat);
    diffVal=0.0;
    for(int j=0; j<NumPtcls; j++) {
      if(j == iat) continue;
      diffVal+= dot(V[j],GeminalBasis->Phi.data(),BasisSize);
    }
    return std::exp(diffVal-Uk[iat]);
  }

    /** later merge the loop */
  OrbitalBase::ValueType 
  ThreeBodyBlockSparse::ratio(ParticleSet& P, int iat,
		    ParticleSet::ParticleGradient_t& dG,
		    ParticleSet::ParticleLaplacian_t& dL) {

    return std::exp(logRatio(P,iat,dG,dL));
  }

    /** later merge the loop */
  OrbitalBase::ValueType 
  ThreeBodyBlockSparse::logRatio(ParticleSet& P, int iat,
		    ParticleSet::ParticleGradient_t& dG,
		    ParticleSet::ParticleLaplacian_t& dL) {

    GeminalBasis->evaluateAllForPtclMove(P,iat);

    const BasisSetType::ValueType* restrict y_ptr=GeminalBasis->Phi.data();
    const BasisSetType::GradType* restrict  dy_ptr=GeminalBasis->dPhi.data();
    const BasisSetType::ValueType* restrict d2y_ptr=GeminalBasis->d2Phi.data();

    //This is the only difference from ThreeBodyGeminal
    RealType* restrict cv_ptr = curV.data();
    for(int b=0; b<Blocks.size(); b++)
    {
      int nb=Blocks[b];
      if(nb)
      {
        GEMV<RealType,0>::apply(LambdaBlocks[b]->data(),y_ptr,cv_ptr,nb,nb);
        for(int ib=0,k=BlockOffset[b];ib<nb; k++,ib++) delV[k] = (*cv_ptr++)-V[iat][k];
        y_ptr+=nb;
      }
    }

    diffVal=0.0;
    GradType dg_acc(0.0);
    ValueType dl_acc(0.0);
    const RealType* restrict vptr=V[0];
    for(int j=0; j<NumPtcls; j++, vptr+=BasisSize) {
      if(j == iat) {
        curLap[j]=0.0;
        curGrad[j]=0.0;
        tLap[j]=0.0;
        tGrad[j]=0.0;
      } else {
        diffVal+= (curVal[j]=dot(delV.data(),Y[j],BasisSize));
        dG[j] += (tGrad[j]=dot(delV.data(),dY[j],BasisSize));
        dL[j] += (tLap[j]=dot(delV.data(),d2Y[j],BasisSize));

        curGrad[j]= dot(vptr,dy_ptr,BasisSize);
        curLap[j] = dot(vptr,d2y_ptr,BasisSize);

        dg_acc += curGrad[j]-dUk(iat,j);
        dl_acc += curLap[j]-d2Uk(iat,j);
      }
    }
    
    dG[iat] += dg_acc;
    dL[iat] += dl_acc;

    curVal[iat]=diffVal;

    return diffVal;
  }

  void ThreeBodyBlockSparse::restore(int iat) {
    //nothing to do here
  }

  void ThreeBodyBlockSparse::acceptMove(ParticleSet& P, int iat) {

    //add the differential
    LogValue += diffVal;
    Uk+=curVal; //accumulate the differences

    dUk.replaceRow(curGrad.begin(),iat);
    d2Uk.replaceRow(curLap.begin(),iat);

    dUk.add2Column(tGrad.begin(),iat);
    d2Uk.add2Column(tLap.begin(),iat);

    //Y.replaceRow(GeminalBasis->y(0),iat);
    //dY.replaceRow(GeminalBasis->dy(0),iat);
    //d2Y.replaceRow(GeminalBasis->d2y(0),iat);
    Y.replaceRow(GeminalBasis->Phi.data(),iat);
    dY.replaceRow(GeminalBasis->dPhi.data(),iat);
    d2Y.replaceRow(GeminalBasis->d2Phi.data(),iat);
    V.replaceRow(curV.begin(),iat);

  }

  void ThreeBodyBlockSparse::update(ParticleSet& P, 		
		       ParticleSet::ParticleGradient_t& dG, 
		       ParticleSet::ParticleLaplacian_t& dL,
		       int iat) {
    cout << "****  This is to be removed " << endl;
    //dG[iat]+=curGrad-dUk[iat]; 
    //dL[iat]+=curLap-d2Uk[iat]; 
    acceptMove(P,iat);
  }

  OrbitalBase::ValueType 
  ThreeBodyBlockSparse::registerData(ParticleSet& P, PooledData<RealType>& buf) {

    evaluateLogAndStore(P);
    FirstAddressOfdY=&(dY(0,0)[0]);
    LastAddressOfdY=FirstAddressOfdY+NumPtcls*BasisSize*DIM;

    FirstAddressOfgU=&(dUk(0,0)[0]);
    LastAddressOfgU = FirstAddressOfgU + NumPtcls*NumPtcls*DIM;

    buf.add(LogValue);
    buf.add(V.begin(), V.end());

    buf.add(Y.begin(), Y.end());
    buf.add(FirstAddressOfdY,LastAddressOfdY);
    buf.add(d2Y.begin(),d2Y.end());

    buf.add(Uk.begin(), Uk.end());
    buf.add(FirstAddressOfgU,LastAddressOfgU);
    buf.add(d2Uk.begin(), d2Uk.end());

    return LogValue;
  }

  void 
  ThreeBodyBlockSparse::evaluateLogAndStore(ParticleSet& P) {
    GeminalBasis->evaluateForWalkerMove(P);

    //this could be better but it is used only sparsely
    //MatrixOperators::product(GeminalBasis->Y, Lambda, V);
    MatrixOperators::product(GeminalBasis->Y, Lambda, V, BlockOffset);
    
    Y=GeminalBasis->Y;
    dY=GeminalBasis->dY;
    d2Y=GeminalBasis->d2Y;

    Uk=0.0;
    LogValue=ValueType();
    for(int i=0; i< NumPtcls-1; i++) {
      const RealType* restrict yptr=GeminalBasis->Y[i];
      for(int j=i+1; j<NumPtcls; j++) {
        RealType x= dot(V[j],yptr,BasisSize);
        LogValue += x;
        Uk[i]+= x;
        Uk[j]+= x;
      }
    }

    for(int i=0; i<NumPtcls; i++)  {
      const BasisSetType::GradType* restrict dptr=GeminalBasis->dY[i];
      const BasisSetType::ValueType* restrict d2ptr=GeminalBasis->d2Y[i];
      const RealType* restrict vptr=V[0];
      BasisSetType::GradType grad(0.0);
      BasisSetType::ValueType lap(0.0);
      for(int j=0; j<NumPtcls; j++, vptr+=BasisSize) {
        if(j==i) {
          dUk(i,j) = 0.0;
          d2Uk(i,j)= 0.0;
        } else {
          grad+= (dUk(i,j) = dot(vptr,dptr,BasisSize));
          lap += (d2Uk(i,j)= dot(vptr,d2ptr,BasisSize));
        }
      }
      P.G(i)+=grad;
      P.L(i)+=lap;
    }
  }

  void 
  ThreeBodyBlockSparse::copyFromBuffer(ParticleSet& P, PooledData<RealType>& buf) {
    buf.get(LogValue);
    buf.get(V.begin(), V.end());

    buf.get(Y.begin(), Y.end());
    buf.get(FirstAddressOfdY,LastAddressOfdY);
    buf.get(d2Y.begin(),d2Y.end());

    buf.get(Uk.begin(), Uk.end());
    buf.get(FirstAddressOfgU,LastAddressOfgU);
    buf.get(d2Uk.begin(), d2Uk.end());
  }

  OrbitalBase::ValueType 
  ThreeBodyBlockSparse::evaluate(ParticleSet& P, PooledData<RealType>& buf) {
    buf.put(LogValue);
    buf.put(V.begin(), V.end());

    buf.put(Y.begin(), Y.end());
    buf.put(FirstAddressOfdY,LastAddressOfdY);
    buf.put(d2Y.begin(),d2Y.end());

    buf.put(Uk.begin(), Uk.end());
    buf.put(FirstAddressOfgU,LastAddressOfgU);
    buf.put(d2Uk.begin(), d2Uk.end());

    return std::exp(LogValue);
  }

  OrbitalBase::ValueType 
  ThreeBodyBlockSparse::updateBuffer(ParticleSet& P, PooledData<RealType>& buf,
      bool fromscratch) {
    evaluateLogAndStore(P);
    buf.put(LogValue);
    buf.put(V.begin(), V.end());

    buf.put(Y.begin(), Y.end());
    buf.put(FirstAddressOfdY,LastAddressOfdY);
    buf.put(d2Y.begin(),d2Y.end());

    buf.put(Uk.begin(), Uk.end());
    buf.put(FirstAddressOfgU,LastAddressOfgU);
    buf.put(d2Uk.begin(), d2Uk.end());
    return LogValue;
  }
    
  void ThreeBodyBlockSparse::resetTargetParticleSet(ParticleSet& P) 
  {
    d_table = DistanceTable::add(CenterRef,P);
    GeminalBasis->resetTargetParticleSet(P);
  }

  ///reset the value of all the Two-Body Jastrow functions
  void ThreeBodyBlockSparse::resetParameters(OptimizableSetType& optVariables) {
    char coeffname[16];
    for(int b=0; b<Blocks.size(); b++)
    {
      Matrix<RealType>& m(*LambdaBlocks[b]);
      int firstK=BlockOffset[b];
      int lastK=BlockOffset[b+1];
      for(int k=firstK,ib=0; k<lastK; k++,ib++)
      {
        for(int kp=k,jb=ib; kp<lastK; kp++,jb++)
        {
          sprintf(coeffname,"%s_%d_%d",ID_Lambda.c_str(),k+IndexOffSet,kp+IndexOffSet);
          OptimizableSetType::iterator it(optVariables.find(coeffname));
          if(it != optVariables.end()) 
          {
            m(ib,jb)=Lambda(k,kp)=(*it).second;
            if(k!=kp) m(jb,ib)=Lambda(kp,k)=(*it).second;
          }
        }
      }
    }

    if(SameBlocksForGroup) checkLambda();

    GeminalBasis->resetParameters(optVariables);
  }


  bool ThreeBodyBlockSparse::put(xmlNodePtr cur, OptimizableSetType& varlist) 
  {

    //BasisSize = GeminalBasis->TotalBasis;
    BasisSize = GeminalBasis->getBasisSetSize();

    app_log() << "  The number of Geminal functions "
      <<"for Three-body Jastrow " << BasisSize << endl;
    app_log() << "  The number of particles " << NumPtcls << endl;

    Lambda.resize(BasisSize,BasisSize);

    //identity is the default
    for(int ib=0; ib<BasisSize; ib++) Lambda(ib,ib)=1.0;

    if(cur == NULL) 
    { 
      addOptimizables(varlist);
    }
    else 
    {//read from an input nodes
      char coeffname[16];
      string aname("j3g");
      string datatype("lambda");
      string sameblocks("yes");
      IndexOffSet=1;
      OhmmsAttributeSet attrib;
      attrib.add(aname,"id");
      attrib.add(aname,"name");
      attrib.add(datatype,"type");
      attrib.add(IndexOffSet,"offset");
      attrib.add(sameblocks,"sameBlocksForGroup");
      attrib.put(cur);

      SameBlocksForGroup = (sameblocks == "yes");
      ID_Lambda=aname;

      if(datatype.find("rray")<datatype.size())
      {
        putContent(Lambda,cur);
        addOptimizables(varlist);
        //symmetrize it
        for(int ib=0; ib<BasisSize; ib++) {
          sprintf(coeffname,"%s_%d_%d",aname.c_str(),ib+IndexOffSet,ib+IndexOffSet);
          varlist[coeffname]=Lambda(ib,ib);
          for(int jb=ib+1; jb<BasisSize; jb++) {
            sprintf(coeffname,"%s_%d_%d",aname.c_str(),ib+IndexOffSet,jb+IndexOffSet);
            Lambda(jb,ib) = Lambda(ib,jb);
            varlist[coeffname]=Lambda(ib,jb);
          }
        }
      }
      else 
      {
        xmlNodePtr tcur=cur->xmlChildrenNode;
        while(tcur != NULL) {
          if(xmlStrEqual(tcur->name,(const xmlChar*)"lambda")) {
            int iIn=atoi((const char*)(xmlGetProp(tcur,(const xmlChar*)"i")));
            int jIn=atoi((const char*)(xmlGetProp(tcur,(const xmlChar*)"j")));
            int i=iIn-IndexOffSet;
            int j=jIn-IndexOffSet;
            double c=atof((const char*)(xmlGetProp(tcur,(const xmlChar*)"c")));
            Lambda(i,j)=c;
            if(i != j) Lambda(j,i)=c;
            sprintf(coeffname,"%s_%d_%d",aname.c_str(),iIn,jIn);
            varlist[coeffname]=c;
          }
          tcur=tcur->next;
        }
      }
    }

    V.resize(NumPtcls,BasisSize);
    Y.resize(NumPtcls,BasisSize);
    dY.resize(NumPtcls,BasisSize);
    d2Y.resize(NumPtcls,BasisSize);

    curGrad.resize(NumPtcls);
    curLap.resize(NumPtcls);
    curVal.resize(NumPtcls);

    tGrad.resize(NumPtcls);
    tLap.resize(NumPtcls);
    curV.resize(BasisSize);
    delV.resize(BasisSize);

    Uk.resize(NumPtcls);
    dUk.resize(NumPtcls,NumPtcls);
    d2Uk.resize(NumPtcls,NumPtcls);

    return true;
  }


  void ThreeBodyBlockSparse::setBlocks(const vector<int>& blockspergroup) 
  {
    //test with three blocks
    Blocks.resize(CenterRef.getTotalNum());
    for(int i=0; i<CenterRef.getTotalNum(); i++)
      Blocks[i]=blockspergroup[CenterRef.GroupID[i]];

    BlockOffset.resize(Blocks.size()+1,0);
    for(int i=0; i<Blocks.size(); i++)
      BlockOffset[i+1]=BlockOffset[i]+Blocks[i];

    if(LambdaBlocks.empty())
    {
      for(int b=0; b<Blocks.size(); b++)
      {
        if(Blocks[b] ==0) continue;
        LambdaBlocks.push_back(new Matrix<RealType>(Blocks[b],Blocks[b]));
        Matrix<RealType>& m(*LambdaBlocks[b]);
        for(int k=BlockOffset[b],ib=0; k<BlockOffset[b+1]; k++,ib++)
        {
          for(int kp=BlockOffset[b],jb=0; kp<BlockOffset[b+1]; kp++,jb++)
          {
            m(ib,jb)=Lambda(k,kp);
          }
        }
      }
    }

    if(SameBlocksForGroup) checkLambda();
  }

  /** set dependent Lambda
   */
  void ThreeBodyBlockSparse::checkLambda() 
  {
    vector<int> need2copy(CenterRef.getSpeciesSet().getTotalNum(),-1);
    for(int i=0; i<CenterRef.getTotalNum(); i++)
    {
      if(Blocks[i] == 0) continue;
      int gid=need2copy[CenterRef.GroupID[i]];
      if(gid<0)//assign the current block index 
        need2copy[CenterRef.GroupID[i]] = i;
      else
        *(LambdaBlocks[i])=*(LambdaBlocks[gid]);

      const Matrix<RealType>& m(*LambdaBlocks[i]);
      for(int k=BlockOffset[i],ib=0; k<BlockOffset[i+1]; k++,ib++)
      {
        for(int kp=BlockOffset[i],jb=0; kp<BlockOffset[i+1]; kp++,jb++)
        {
          Lambda(k,kp)=m(ib,jb);
        }
      }
    }
  }

  void ThreeBodyBlockSparse::addOptimizables(OptimizableSetType& varlist) 
  {
    char coeffname[16];
    for(int ib=0; ib<BasisSize; ib++) {
      sprintf(coeffname,"%s_%d_%d",ID_Lambda.c_str(),ib+IndexOffSet,ib+IndexOffSet);
      varlist[coeffname]=Lambda(ib,ib);
      for(int jb=ib+1; jb<BasisSize; jb++) {
        sprintf(coeffname,"%s_%d_%d",ID_Lambda.c_str(),ib+IndexOffSet,jb+IndexOffSet);
        Lambda(jb,ib) = Lambda(ib,jb);
        varlist[coeffname]=Lambda(ib,jb);
      }
    }
  }
}
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1796 $   $Date: 2007-02-22 11:40:21 -0600 (Thu, 22 Feb 2007) $
 * $Id: ThreeBodyBlockSparse.cpp 1796 2007-02-22 17:40:21Z jnkim $ 
 ***************************************************************************/

