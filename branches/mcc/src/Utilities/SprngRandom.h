//////////////////////////////////////////////////////////////////
// (c) Copyright 1998-2002 by Jeongnim Kim
//
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
#ifndef OHMMS_SPRNGRANDOM_H
#define OHMMS_SPRNGRANDOM_H
/*! \class template<int rg> SprngRandom
 *\brief
 *  A wrapper class to generate a random number using sprng library.
 *  Templated for the generator type.
 *  Choose 0 - 5: EXAMPLE/gen_types_menu.h
 *  \li{    printf("   lfg     --- 0 \n");}
 *  \li{    printf("   lcg     --- 1 \n");}
 *  \li{    printf("   lcg64   --- 2 \n");}
 *  \li{    printf("   cmrg    --- 3 \n");}
 *  \li{    printf("   mlfg    --- 4 \n");}
 *  \li{    printf("   pmlcg   --- 5 \n");}
 *
 *  Documentation at http://sprng.cs.fsu.edu
 */
#include <math.h>
#include <sprng.h>

#define SRSEED 985456376

template<int rg>
class SprngRandom {

public:
  typedef double Return_t;

  SprngRandom(): thisStreamID(0), nStreams(1),
		 thisStream(NULL),thisSeed(SRSEED)  { }

  SprngRandom(int i, int nstr, int iseed): 
    thisStream(NULL),thisSeed(SRSEED) 
  {
    init(i,nstr,iseed);
  }
 
  void init(int i, int nstr, int iseed) {

    if(!thisStream) {
      thisStreamID = i;
      nStreams = nstr;
      if(iseed < 0) {  // generate a new seed
	thisSeed = make_sprng_seed();
      } else if(iseed > 0) { // use input seed
	thisSeed = iseed;      
      } // if iseed = 0, use SRSEED
#ifdef SPRNG_OLD
      thisStream = init_sprng(rg,thisStreamID,nStreams, thisSeed);
#else
      thisStream = init_sprng(rg,thisStreamID,nStreams, thisSeed, SPRNG_DEFAULT);
#endif
    }
  }

  inline Return_t getRandom() { return sprng(thisStream);}//!< return [0,1)
  inline Return_t operator()() { return getRandom();} //!< return [0,1)
  inline int irand() { return isprng(thisStream);} //!< return random integer

  inline void bivariate(Return_t& g1, Return_t &g2) {
    Return_t v1, v2, r;
    do {
    v1 = 2.0e0*getRandom() - 1.0e0;
    v2 = 2.0e0*getRandom() - 1.0e0;
    r = v1*v1+v2*v2;
    } while(r > 1.0e0);
    Return_t fac = sqrt(-2.0e0*log(r)/r);
    g1 = v1*fac;
    g2 = v2*fac;
  }

private:
  int thisStreamID;
  int nStreams;
  int thisSeed;
  int* thisStream;
};
#endif

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
