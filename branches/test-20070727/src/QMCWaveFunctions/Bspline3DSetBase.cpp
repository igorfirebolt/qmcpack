/////////////////////////////////////////////////////////////////
// (c) Copyright 2007-  Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Modified by Jeongnim Kim for qmcpack
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
/** @file Bspline3DSet.cpp
 * @brief Implement Bspline3DBase and its derived classes
 */
#include "QMCWaveFunctions/Bspline3DSetBase.h"

namespace qmcplusplus {

  Bspline3DSetBase::Bspline3DSetBase(): Orthorhombic(true),NumOrbitals(0)
  { 
  }

  Bspline3DSetBase::~Bspline3DSetBase()
  {
  }

  void Bspline3DSetBase::setLattice(const CrystalLattice<RealType,OHMMS_DIM>& lat)
  {
    Lattice.set(lat);
    UnitLattice.set(lat);

    GGt=dot(Lattice.G,transpose(Lattice.G));
  }

  void Bspline3DSetBase::resize(int norbs)
  {
    if(P.empty())
    {
      //remove this
      NumOrbitals=norbs;
      Centers.resize(norbs);
      Origins.resize(norbs);
      P.resize(norbs,0);
      OrbitalSetSize=norbs;
      BasisSetSize=norbs;
      Identity=true;
    }
  }

  void Bspline3DSetBase::setGrid(RealType xi, RealType xf, 
      RealType yi, RealType yf, RealType zi, RealType zf, 
      int nx, int ny, int nz, 
      bool interp, bool periodic,bool openend)
  {
    if(Orthorhombic)
      bKnots.setGrid(xi,xf,yi,yf,zi,zf,nx,ny,nz,interp,periodic,openend);
    else
      bKnots.setGrid(0.0,1.0,0.0,1.0,0.0,1.0,nx,ny,nz,interp,periodic,openend);
  }

  void Bspline3DSetBase::setTwistAngle(const PosType& tangle)
  {
    TwistAngle=tangle;
    mK2=-dot(tangle,tangle);
  }

  void Bspline3DSetBase::resetParameters(VarRegistry<RealType>& vlist)
  {
  }

  void Bspline3DSetBase::setOrbitalSetSize(int norbs) { 
    if(norbs == OrbitalSetSize ) return;
    resize(norbs);
    //OrbitalSetSize=norbs;
    //BasisSetSize=norbs;
  }
  void Bspline3DSetBase::resetTargetParticleSet(ParticleSet& e) { }

  void Bspline3DSetBase::add(int i, const StorageType& data, StorageType* curP)
  {
    bKnots.Init(data,*curP);
    P[i]=curP;
  }

  void Bspline3DSetBase::add(int i, StorageType* curP)
  {
    P[i]=curP;
  }

  void Bspline3DSetBase::tileOrbitals(const TinyVector<int,3>& boxdup)
  {
    TensorType uc(
        Lattice.R(0,0)/static_cast<RealType>(boxdup[0]),
        Lattice.R(0,1)/static_cast<RealType>(boxdup[0]),
        Lattice.R(0,2)/static_cast<RealType>(boxdup[0]),
        Lattice.R(1,0)/static_cast<RealType>(boxdup[1]),
        Lattice.R(1,1)/static_cast<RealType>(boxdup[1]),
        Lattice.R(1,2)/static_cast<RealType>(boxdup[1]),
        Lattice.R(2,0)/static_cast<RealType>(boxdup[2]),
        Lattice.R(2,1)/static_cast<RealType>(boxdup[2]),
        Lattice.R(2,2)/static_cast<RealType>(boxdup[2]));
    UnitLattice.set(uc);

    int norb=OrbitalSetSize/(boxdup[0]*boxdup[1]*boxdup[2]);
    int i=norb;
    for(int ic=0; ic<boxdup[0]; ic++)
      for(int jc=0; jc<boxdup[1]; jc++)
        for(int kc=0; kc<boxdup[2]; kc++)
        {
          if(ic == 0 && jc == 0 && kc == 0) continue;
          PosType c(ic,jc,kc);
          PosType displ=UnitLattice.toCart(c);
          for(int o=0; o<norb; o++, i++)
          {
            P[i]=P[o];
            Centers[i]=Centers[o]+displ;
            Origins[i]=Origins[o]+displ;
          }
        }

    //for(i=0; i<OrbitalSetSize; i++)
    //  app_log() << Centers[i] << endl;
  }

//  void TricubicBsplineSetBuilder::createBsplineBasisSet(xmlNodePtr cur, 
//      PWParameterSet* myParam, hid_t hfileID)
//  {
//
//    int norb(0);
//    int degeneracy(1);
//    OhmmsAttributeSet aAttrib;
//    aAttrib.add(norb,"orbitals"); aAttrib.add(norb,"size");
//    aAttrib.add(degeneracy,"degeneracy");
//    aAttrib.put(cur);
//
//    bool truncate = (myParam->Rcut>0.0);
//
//    vector<int> occSet(norb);
//    for(int i=0; i<norb; i++) occSet[i]=i;
//
//    //set the root name
//    char hroot[128];
//    sprintf(hroot,"/%s/%s%d",myParam->eigTag.c_str(),myParam->twistTag.c_str(),myParam->twistIndex);
//
//    int spinIndex=0;
//    cur=cur->children;
//    while(cur != NULL) {
//      string cname((const char*)(cur->name));
//      if(cname == "occupation") {
//        string occ_mode("ground");
//        OhmmsAttributeSet oAttrib;
//        oAttrib.add(occ_mode,"mode");
//        oAttrib.add(spinIndex,"spindataset");
//        oAttrib.put(cur);
//        //Do nothing if mode == ground
//        if(occ_mode == "excited") {
//          vector<int> occ_in, occRemoved;
//          putContent(occ_in,cur);
//          for(int k=0; k<occ_in.size(); k++) {
//            if(occ_in[k]<0) 
//              occRemoved.push_back(-occ_in[k]-1);
//          }
//          int kpopd=0;
//          for(int k=0; k<occ_in.size(); k++) {
//            if(occ_in[k]>0) 
//              occSet[occRemoved[kpopd++]]=occ_in[k]-1;
//          }
//        }
//        const xmlChar* h5path = xmlGetProp(cur,(const xmlChar*)"h5path");
//        if(h5path != NULL) sprintf(hroot,"%s",(const char*)h5path);
//      }
//      cur=cur->next;
//    }
//
//    //use BoxGrid
//    TinyVector<IndexType,DIM> BoxGrid(myParam->BoxGrid);
//
//    if(myParam->OpenEndGrid)
//      setGrid(myParam->LowerBox[0],myParam->UpperBox[0], 
//          myParam->LowerBox[1],myParam->UpperBox[1],
//          myParam->LowerBox[2],myParam->UpperBox[2],
//          BoxGrid[0],BoxGrid[1],BoxGrid[2]);
//    else//need to offset for closed end
//      setGrid(myParam->LowerBox[0],myParam->UpperBox[0],
//          myParam->LowerBox[1],myParam->UpperBox[1],
//          myParam->LowerBox[2],myParam->UpperBox[2],
//          BoxGrid[0]-1,BoxGrid[1]-1,BoxGrid[2]-1);
//
//    //resize the orbitals
//    resize(norb);
//
//    StorageType inData(BoxGrid[0],BoxGrid[1],BoxGrid[2]);
//    bool complex2real = myParam->hasComplexData(hfileID);
//#if defined(QMC_COMPLEX)
//    if(!complex2real) 
//    {
//      app_error() << "  Real wavefunctions cannot be used with QMC_COMPLEX=1" << endl;
//      abort(); //FIXABORT
//    }
//    complex2real = false;//reset to false
//#endif
//
//    ///add the dummy center
//    PosType center(0.0);
//
//    if(complex2real)
//    {
//      Array<ComplexType,3> inTemp(BoxGrid[0],BoxGrid[1],BoxGrid[2]);
//      for(int iorb=0; iorb<norb; iorb++) 
//      {
//        if(truncate)
//        {
//          string centerName=myParam->getCenterName(hroot,occSet[iorb]);
//          HDFAttribIO<PosType > cdummy(center);
//          cdummy.read(hfileID,centerName.c_str());
//        }
//        ostringstream wnshort;
//        string eigvName=myParam->getEigVectorName(hroot,occSet[iorb]/degeneracy,spinIndex);
//        wnshort<<curH5Fname << "#"<<occSet[iorb]/degeneracy << "#" << spinIndex;
//        map<string,StorageType*>::iterator it(BigDataSet.find(wnshort.str()));
//        if(it == BigDataSet.end()) {
//          if(print_log)
//          {
//            app_log() << "   Reading spline function " << eigvName << " (" << wnshort.str()  << ")"  << endl;
//            if(truncate) app_log() << "     center=" << center << endl;
//          }
//          StorageType* newP =new StorageType;
//          HDFAttribIO<Array<ComplexType,3> > dummy(inTemp);
//          dummy.read(hfileID,eigvName.c_str());
//          BLAS::copy(inTemp.size(),inTemp.data(),inData.data());
//          BigDataSet[wnshort.str()]=newP;
//          abasis->add(iorb,center,inData,newP);
//        } else {
//          if(print_log)
//          {
//            app_log() << "   Reusing spline function " << eigvName << " (" << wnshort.str()  << ")" << endl;
//            if(truncate) app_log() << "     center=" << center << endl;
//          }
//          abasis->add(iorb,center,(*it).second);
//        }
//      } 
//    }
//    else 
//    {
//      for(int iorb=0; iorb<norb; iorb++) {
//        if(truncate)
//        {
//          string centerName=myParam->getCenterName(hroot,occSet[iorb]);
//          HDFAttribIO<PosType > cdummy(center);
//          cdummy.read(hfileID,centerName.c_str());
//        }
//        ostringstream wnshort;
//        string eigvName=myParam->getEigVectorName(hroot,occSet[iorb]/degeneracy,spinIndex);
//        wnshort<<curH5Fname << "#"<<occSet[iorb]/degeneracy << "#" << spinIndex;
//        map<string,StorageType*>::iterator it(BigDataSet.find(wnshort.str()));
//        if(it == BigDataSet.end()) {
//          if(print_log)
//          {
//            app_log() << "   Reading spline function " << eigvName << " (" << wnshort.str()  << ")"  << endl;
//            if(truncate) app_log() << "     center=" << center << endl;
//          }
//          StorageType* newP=new StorageType;
//          HDFAttribIO<StorageType> dummy(inData);
//          dummy.read(hfileID,eigvName.c_str());
//          BigDataSet[wnshort.str()]=newP;
//          abasis->add(iorb,center,inData,newP);
//        } else {
//          if(print_log)
//          {
//            app_log() << "   Reusing spline function " << eigvName << " (" << wnshort.str()  << ")" << endl;
//            if(truncate) app_log() << "     center=" << center << endl;
//          }
//          abasis->add(iorb,center,(*it).second);
//        }
//      } 
//    }
//    if(truncate) {
//      if(print_log)
//        app_log() << "   Truncating orbitals at Rcut= " << myParam->Rcut << endl;
//      abasis->setRcut(myParam->Rcut);
//    }
//  }
}
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 2013 $   $Date: 2007-05-22 16:47:09 -0500 (Tue, 22 May 2007) $
 * $Id: TricubicBsplineSet.h 2013 2007-05-22 21:47:09Z jnkim $
 ***************************************************************************/
