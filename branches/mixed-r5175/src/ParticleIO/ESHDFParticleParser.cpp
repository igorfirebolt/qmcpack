//////////////////////////////////////////////////////////////////
// (c) Copyright 2009- by Jeongnim Kim
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
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "ParticleIO/ESHDFParticleParser.h"
#include "ParticleIO/ParticleIOUtility.h"
//#include "ParticleBase/ParticleUtility.h"
#include "ParticleIO/HDFParticleAttrib.h"
#include "Numerics/HDFNumericAttrib.h"
#include "OhmmsData/HDFStringAttrib.h"
#include "Message/Communicate.h"
#include "Message/CommOperators.h"
#include "Utilities/SimpleParser.h"
#include "Utilities/IteratorUtility.h"

namespace qmcplusplus {

  ESHDFElectronsParser::ESHDFElectronsParser(ParticleSet& aptcl
      ,hid_t h, Communicate* c)
    :ref_(aptcl),hfile_id(h),myComm(c) { }

  bool ESHDFElectronsParser::put(xmlNodePtr cur) 
  {
    return true;
  }

  ESHDFIonsParser::ESHDFIonsParser(ParticleSet& aptcl, hid_t h, Communicate* c)
    :ref_(aptcl),hfile_id(h),myComm(c)
     ,atomic_number_tag("atomic_number")
     ,charge_tag("valence_charge")
     ,mass_tag("mass")
  { }

  bool ESHDFIonsParser::put(xmlNodePtr cur) 
  {
    //add basic attributes of the speciesset
    SpeciesSet& tspecies(ref_.getSpeciesSet());
    int icharge= tspecies.addAttribute("charge");//charge_tag);
    int iatnumber= tspecies.addAttribute(atomic_number_tag);
    int membersize= tspecies.addAttribute("membersize");
    int massind= tspecies.addAttribute(mass_tag);

    if(myComm->rank()==0 && hfile_id>=-1) readESHDF();

    if(myComm->size()==1) return true;

    int nspecies=tspecies.getTotalNum();
    int natoms=ref_.getTotalNum();

    ostringstream o;
    if(myComm->rank()==0)
    {
      int i=0;
      for(; i<nspecies-1;++i) o<<tspecies.speciesName[i]<<",";
      o<<tspecies.speciesName[i];
    }

    TinyVector<int,3> bsizes(nspecies,natoms,o.str().size()+1);
    myComm->bcast(bsizes);

    //send the names: UGLY!!!!
    nspecies=bsizes[0];
    char *species_names=new char[bsizes[2]];
    if(myComm->rank()==0) snprintf(species_names, bsizes[2], "%s",o.str().c_str());
    myComm->bcast(species_names,bsizes[2]);
    if(myComm->rank())
    {
      vector<string> vlist;
      parsewords(species_names,vlist);
      for(int i=0; i<vlist.size();++i) tspecies.addSpecies(vlist[i]);
      //create natoms particles
      ref_.create(bsizes[1]);
    }
    delete [] species_names;

    ParticleSet::Tensor_t lat(ref_.Lattice.R);
    ParticleSet::Buffer_t pbuffer;
    for(int i=0; i<tspecies.numAttributes();++i)
      pbuffer.add(tspecies.d_attrib[i]->begin(),tspecies.d_attrib[i]->end());
    pbuffer.add(lat.begin(),lat.end());
    pbuffer.add(get_first_address(ref_.R),get_last_address(ref_.R));
    pbuffer.add(ref_.GroupID.begin(),ref_.GroupID.end());
    myComm->bcast(pbuffer);


    ref_.R.InUnit=PosUnit::CartesianUnit;
    if(myComm->rank())
    {
      pbuffer.rewind();
      for(int i=0; i<tspecies.numAttributes();++i)
        pbuffer.get(tspecies.d_attrib[i]->begin(),tspecies.d_attrib[i]->end());
      pbuffer.get(lat.begin(),lat.end());
      pbuffer.get(get_first_address(ref_.R),get_last_address(ref_.R));
      pbuffer.get(ref_.GroupID.begin(),ref_.GroupID.end());
      ref_.Lattice.set(lat);
    }

    return true;
  }

  void ESHDFIonsParser::readESHDF()
  {
    int nspecies=0;
    {
      HDFAttribIO<int> a(nspecies);
      a.read(hfile_id,"atoms/number_of_species");
    }

    SpeciesSet& tspecies(ref_.getSpeciesSet());
    int icharge= tspecies.addAttribute("charge");//use charge
    int iatnumber= tspecies.addAttribute(atomic_number_tag);
    int massind= tspecies.addAttribute(mass_tag);
    //add charge
    //atomic_number is optional
    for(int i=0; i<nspecies; ++i)
    {
      ostringstream o;
      o << "atoms/species_"<<i;
      hid_t g=H5Gopen(hfile_id,o.str().c_str());

      string aname;
      HDFAttribIO<string> a(aname);
      a.read(g,"name");
      int ii=tspecies.addSpecies(aname);

      double q=-1;
      HDFAttribIO<double> b(q);
      b.read(g,charge_tag.c_str());
      tspecies(icharge,ii)=q;

      int atnum=0;
      HDFAttribIO<int> c(atnum);
      c.read(g,atomic_number_tag.c_str());
      tspecies(iatnumber,ii)=atnum;

      //close the group
      H5Gclose(g);
    }

    for(int ig=0; ig<nspecies; ++ig) tspecies(massind,ig)=1.0; 
    //just for checking
    // tspecies(icharge,0)=15.0;
    // tspecies(icharge,1)=6.0;

    {//get the unit cell
      Tensor<double,3> alat;
      HDFAttribIO<Tensor<double,3> > a(alat);
      a.read(hfile_id,"supercell/primitive_vectors");
      ref_.Lattice.set(alat);
    }

    {//get the unit cell
      int natoms=0;
      HDFAttribIO<int> a(natoms);
      a.read(hfile_id,"atoms/number_of_atoms");
      ref_.create(natoms);

      ref_.R.InUnit=PosUnit::CartesianUnit;
      HDFAttribIO<ParticleSet::ParticlePos_t> b(ref_.R);
      b.read(hfile_id,"atoms/positions");

      HDFAttribIO<ParticleSet::ParticleIndex_t> c(ref_.GroupID);
      c.read(hfile_id,"atoms/species_ids");
    }
  }

  void ESHDFIonsParser::expand(Tensor<int,3>& tmat)
  {
    expandSuperCell(ref_,tmat);
  //  typedef ParticleSet::SingleParticlePos_t SingleParticlePos_t;
  //  typedef ParticleSet::Tensor_t Tensor_t;

  //  Tensor<int,3> I(1,0,0,0,1,0,0,0,1);
  //  bool identity=true;
  //  int ij=0;
  //  while(identity&& ij<9)
  //  {
  //    identity=(I[ij]==tmat[ij]);
  //    ++ij;
  //  }

  //  if(identity)
  //  {
  //    cout << "  Identity tiling. Nothing to do " << endl;
  //  }
  //  else
  //  {
  //    //convert2unit
  //    ref_.convert2Unit(ref_.R);
  //    ParticleSet::ParticleLayout_t PrimCell(ref_.Lattice);
  //    ref_.Lattice.set(dot(tmat,PrimCell.R));

  //    int natoms=ref_.getTotalNum();
  //    int numCopies = abs(tmat.det());
  //    ParticleSet::ParticlePos_t primPos(ref_.R);
  //    ParticleSet::ParticleIndex_t primTypes(ref_.GroupID);
  //    ref_.resize(natoms*numCopies);
  //    int maxCopies = 10;
  //    int index=0;
  //    //set the unit to the Cartesian
  //    ref_.R.InUnit=PosUnit::CartesianUnit;
  //    for(int ns=0; ns<ref_.getSpeciesSet().getTotalNum();++ns)
  //    {
  //      for (int i0=-maxCopies; i0<=maxCopies; i0++)    
  //        for (int i1=-maxCopies; i1<=maxCopies; i1++)
  //          for (int i2=-maxCopies; i2<=maxCopies; i2++) 
  //            for (int iat=0; iat < primPos.size(); iat++) 
  //            {
  //              if(primTypes[iat]!=ns) continue;
  //              //SingleParticlePos_t r     = primPos[iat];
  //              SingleParticlePos_t uPrim = primPos[iat];
  //              for (int i=0; i<3; i++)   uPrim[i] -= std::floor(uPrim[i]);
  //              SingleParticlePos_t r = PrimCell.toCart(uPrim) + (double)i0*PrimCell.a(0) 
  //                + (double)i1*PrimCell.a(1) + (double)i2*PrimCell.a(2);
  //              SingleParticlePos_t uSuper = ref_.Lattice.toUnit(r);
  //              if ((uSuper[0] >= -1.0e-6) && (uSuper[0] < 0.9999) &&
  //                  (uSuper[1] >= -1.0e-6) && (uSuper[1] < 0.9999) &&
  //                  (uSuper[2] >= -1.0e-6) && (uSuper[2] < 0.9999)) 
  //              {
  //      	  char buff[500];
  //      	  app_log() << "  Reduced coord    Cartesion coord    species.\n";
  //      	  snprintf (buff, 500, "  %10.4f  %10.4f %10.4f   %12.6f %12.6f %12.6f %d\n", 
  //      		    uSuper[0], uSuper[1], uSuper[2], r[0], r[1], r[2], ns);
  //      	  app_log() << buff;
  //                ref_.R[index]= r;
  //                ref_.GroupID[index]= ns;//primTypes[iat];
  //                index++;
  //              }
  //            }
  //    }
  //  }

    ref_.createSK();

    SpeciesSet& tspecies(ref_.getSpeciesSet());
    vector<int> numPerGroup(tspecies.getTotalNum(),0);
    for(int iat=0; iat<ref_.GroupID.size(); iat++) {
      numPerGroup[ref_.GroupID[iat]]++;
    }
    int membersize= tspecies.addAttribute("membersize");
    for(int ig=0; ig<tspecies.getTotalNum(); ++ig) {
      tspecies(membersize,ig)=numPerGroup[ig];
    }


    //char fname[8];
    //sprintf(fname,"test%i",myComm->rank());
    //ofstream fout(fname);
    //SpeciesSet& tspecies(ref_.getSpeciesSet());
    //for(int i=0; i<tspecies.size(); ++i)
    //{
    //  fout <<  tspecies.speciesName[i] ;
    //  for(int j=0; j<tspecies.numAttributes(); ++j) fout << " " << tspecies(j,i) ;
    //  fout << endl;
    //}
    //ref_.Lattice.print(fout);
    //for(int i=0; i<ref_.getTotalNum(); ++i)
    //{
    //  fout << ref_.GroupID[i] << " " << ref_.R[i] << endl;
    //}

    //ref_.convert2Unit(ref_.R);
    //if (myComm->rank() == 0) {
    //   fprintf (stderr, "Supercell ion positions = \n");
    //   for (int i=0; i<ref_.getTotalNum(); i++)
    //     fprintf (stderr, "  %d [%14.12f %14.12f %14.12f]\n"
    //         ,ref_.GroupID[i], ref_.R[i][0], ref_.R[i][1], ref_.R[i][2]);
    //}

  }
}

/***************************************************************************
 * $RCSfile$   $Author: qmc $
 * $Revision: 1048 $   $Date: 2006-05-18 13:49:04 -0500 (Thu, 18 May 2006) $
 * $Id: ESHDFParticleParser.cpp 1048 2006-05-18 18:49:04Z qmc $ 
 ***************************************************************************/
