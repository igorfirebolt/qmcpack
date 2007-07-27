#ifndef ATOMICHF_FILLSHELLS_H
#define ATOMICHF_FILLSHELLS_H
#include "AtomicHF/HFAtomicOrbitals.h"

using namespace ohmmshf;

inline void FillShellsNucPot(HFAtomicOrbitals& mo, 
			     int nmax) { 
			     //			     vector<int>& cs, 
			     //			     vector<double>& cratio){
    int up=1;
    int dn=-1;

    switch(nmax) {
    case(1):
      mo.add(1,0,0,up,1.0);
      mo.add(1,0,0,dn,1.0);
      //     cs[1] = 2;
      //     cratio[1] = 0.3;
      break;

    case(2):
      mo.add(1,0, 0,up,1.0);
      mo.add(1,0, 0,dn,1.0);

      mo.add(2,0, 0,up,1.0);
      mo.add(2,0, 0,dn,1.0);
      mo.add(2,1,-1,up,1.0);
      mo.add(2,1,-1,dn,1.0);
      mo.add(2,1, 0,up,1.0);
      mo.add(2,1, 0,dn,1.0);
      mo.add(2,1, 1,up,1.0);
      mo.add(2,1, 1,dn,1.0);

 //      cs[1] = 2;
//       cratio[1] = 0.3;
//       cs[2] = 10;
//       cratio[2] = 0.35;
      break;

    case(3):
      mo.add(1,0, 0,up,1.0);
      mo.add(1,0, 0,dn,1.0);

      mo.add(2,0, 0,up,1.0);
      mo.add(2,0, 0,dn,1.0);
      mo.add(2,1,-1,up,1.0);
      mo.add(2,1,-1,dn,1.0);
      mo.add(2,1, 0,up,1.0);
      mo.add(2,1, 0,dn,1.0);
      mo.add(2,1, 1,up,1.0);
      mo.add(2,1, 1,dn,1.0);

      mo.add(3,0, 0,up,1.0);
      mo.add(3,0, 0,dn,1.0);
      mo.add(3,1,-1,up,1.0);
      mo.add(3,1,-1,dn,1.0);
      mo.add(3,1, 0,up,1.0);
      mo.add(3,1, 0,dn,1.0);
      mo.add(3,1, 1,up,1.0);
      mo.add(3,1, 1,dn,1.0);

  //     cs[1] = 2;
//       cratio[1] = 0.3;
//       cs[2] = 10;
//       cratio[2] = 0.35;
//       cs[3]=18;
//       cratio[2] = 0.4;
      break;
    


    case(4):
      mo.add(1,0, 0,up,1.0);
      mo.add(1,0, 0,dn,1.0);

      mo.add(2,0, 0,up,1.0);
      mo.add(2,0, 0,dn,1.0);
      mo.add(2,1,-1,up,1.0);
      mo.add(2,1,-1,dn,1.0);
      mo.add(2,1, 0,up,1.0);
      mo.add(2,1, 0,dn,1.0);
      mo.add(2,1, 1,up,1.0);
      mo.add(2,1, 1,dn,1.0);

      mo.add(3,0, 0,up,1.0);
      mo.add(3,0, 0,dn,1.0);
      mo.add(3,1,-1,up,1.0);
      mo.add(3,1,-1,dn,1.0);
      mo.add(3,1, 0,up,1.0);
      mo.add(3,1, 0,dn,1.0);
      mo.add(3,1, 1,up,1.0);
      mo.add(3,1, 1,dn,1.0);

      mo.add(3,2,-2,up,1.0);
      mo.add(3,2,-2,dn,1.0);
      mo.add(3,2,-1,up,1.0);
      mo.add(3,2,-1,dn,1.0);
      mo.add(3,2, 0,up,1.0);
      mo.add(3,2, 0,dn,1.0);
      mo.add(3,2, 1,up,1.0);
      mo.add(3,2, 1,dn,1.0);
      mo.add(3,2, 2,up,1.0);
      mo.add(3,2, 2,dn,1.0);
      mo.add(4,0, 0,up,1.0);
      mo.add(4,0, 0,dn,1.0);
      mo.add(4,1,-1,up,1.0);
      mo.add(4,1,-1,dn,1.0);
      mo.add(4,1, 0,up,1.0);
      mo.add(4,1, 0,dn,1.0);
      mo.add(4,1, 1,up,1.0);
      mo.add(4,1, 1,dn,1.0);

    //   cs[1] = 2;
//       cratio[1] = 0.3;
//       cs[2] = 10;
//       cratio[2] = 0.35;
//       cs[3]=18;
//       cratio[2] = 0.4;
//       cs[4] = 36;
//       cratio[4] = 0.45;
      break;
    

    case(5):
      mo.add(1,0, 0,up,1.0);
      mo.add(1,0, 0,dn,1.0);

      mo.add(2,0, 0,up,1.0);
      mo.add(2,0, 0,dn,1.0);
      mo.add(2,1,-1,up,1.0);
      mo.add(2,1,-1,dn,1.0);
      mo.add(2,1, 0,up,1.0);
      mo.add(2,1, 0,dn,1.0);
      mo.add(2,1, 1,up,1.0);
      mo.add(2,1, 1,dn,1.0);

      mo.add(3,0, 0,up,1.0);
      mo.add(3,0, 0,dn,1.0);
      mo.add(3,1,-1,up,1.0);
      mo.add(3,1,-1,dn,1.0);
      mo.add(3,1, 0,up,1.0);
      mo.add(3,1, 0,dn,1.0);
      mo.add(3,1, 1,up,1.0);
      mo.add(3,1, 1,dn,1.0);

      mo.add(3,2,-2,up,1.0);
      mo.add(3,2,-2,dn,1.0);
      mo.add(3,2,-1,up,1.0);
      mo.add(3,2,-1,dn,1.0);
      mo.add(3,2, 0,up,1.0);
      mo.add(3,2, 0,dn,1.0);
      mo.add(3,2, 1,up,1.0);
      mo.add(3,2, 1,dn,1.0);
      mo.add(3,2, 2,up,1.0);
      mo.add(3,2, 2,dn,1.0);
      mo.add(4,0, 0,up,1.0);
      mo.add(4,0, 0,dn,1.0);
      mo.add(4,1,-1,up,1.0);
      mo.add(4,1,-1,dn,1.0);
      mo.add(4,1, 0,up,1.0);
      mo.add(4,1, 0,dn,1.0);
      mo.add(4,1, 1,up,1.0);
      mo.add(4,1, 1,dn,1.0);

      mo.add(4,2,-2,up,1.0);
      mo.add(4,2,-2,dn,1.0);
      mo.add(4,2,-1,up,1.0);
      mo.add(4,2,-1,dn,1.0);
      mo.add(4,2, 0,up,1.0);
      mo.add(4,2, 0,dn,1.0);
      mo.add(4,2, 1,up,1.0);
      mo.add(4,2, 1,dn,1.0);
      mo.add(4,2, 2,up,1.0);
      mo.add(4,2, 2,dn,1.0);
      mo.add(5,0, 0,up,1.0);
      mo.add(5,0, 0,dn,1.0);
      mo.add(5,1,-1,up,1.0);
      mo.add(5,1,-1,dn,1.0);
      mo.add(5,1, 0,up,1.0);
      mo.add(5,1, 0,dn,1.0);
      mo.add(5,1, 1,up,1.0);
      mo.add(5,1, 1,dn,1.0);

  //     cs[1] = 2;
//       cratio[1] = 0.3;
//       cs[2] = 10;
//       cratio[2] = 0.35;
//       cs[3]=18;
//       cratio[2] = 0.4;
//       cs[4] = 36;
//       cratio[4] = 0.45;
//       cs[5] = 54;
//       cratio[5] = 0.5;
      break;


  case(6):
      mo.add(1,0, 0,up,1.0);
      mo.add(1,0, 0,dn,1.0);

      mo.add(2,0, 0,up,1.0);
      mo.add(2,0, 0,dn,1.0);
      mo.add(2,1,-1,up,1.0);
      mo.add(2,1,-1,dn,1.0);
      mo.add(2,1, 0,up,1.0);
      mo.add(2,1, 0,dn,1.0);
      mo.add(2,1, 1,up,1.0);
      mo.add(2,1, 1,dn,1.0);

      mo.add(3,0, 0,up,1.0);
      mo.add(3,0, 0,dn,1.0);
      mo.add(3,1,-1,up,1.0);
      mo.add(3,1,-1,dn,1.0);
      mo.add(3,1, 0,up,1.0);
      mo.add(3,1, 0,dn,1.0);
      mo.add(3,1, 1,up,1.0);
      mo.add(3,1, 1,dn,1.0);

      mo.add(3,2,-2,up,1.0);
      mo.add(3,2,-2,dn,1.0);
      mo.add(3,2,-1,up,1.0);
      mo.add(3,2,-1,dn,1.0);
      mo.add(3,2, 0,up,1.0);
      mo.add(3,2, 0,dn,1.0);
      mo.add(3,2, 1,up,1.0);
      mo.add(3,2, 1,dn,1.0);
      mo.add(3,2, 2,up,1.0);
      mo.add(3,2, 2,dn,1.0);
      mo.add(4,0, 0,up,1.0);
      mo.add(4,0, 0,dn,1.0);
      mo.add(4,1,-1,up,1.0);
      mo.add(4,1,-1,dn,1.0);
      mo.add(4,1, 0,up,1.0);
      mo.add(4,1, 0,dn,1.0);
      mo.add(4,1, 1,up,1.0);
      mo.add(4,1, 1,dn,1.0);

      mo.add(4,2,-2,up,1.0);
      mo.add(4,2,-2,dn,1.0);
      mo.add(4,2,-1,up,1.0);
      mo.add(4,2,-1,dn,1.0);
      mo.add(4,2, 0,up,1.0);
      mo.add(4,2, 0,dn,1.0);
      mo.add(4,2, 1,up,1.0);
      mo.add(4,2, 1,dn,1.0);
      mo.add(4,2, 2,up,1.0);
      mo.add(4,2, 2,dn,1.0);
      mo.add(5,0, 0,up,1.0);
      mo.add(5,0, 0,dn,1.0);
      mo.add(5,1,-1,up,1.0);
      mo.add(5,1,-1,dn,1.0);
      mo.add(5,1, 0,up,1.0);
      mo.add(5,1, 0,dn,1.0);
      mo.add(5,1, 1,up,1.0);
      mo.add(5,1, 1,dn,1.0);

      mo.add(4,3,-3,up,1.0);
      mo.add(4,3,-3,dn,1.0);
      mo.add(4,3,-2,up,1.0);
      mo.add(4,3,-2,dn,1.0);
      mo.add(4,3,-1,up,1.0);
      mo.add(4,3,-1,dn,1.0);
      mo.add(4,3, 0,up,1.0);
      mo.add(4,3, 0,dn,1.0);
      mo.add(4,3, 1,up,1.0);
      mo.add(4,3, 1,dn,1.0);
      mo.add(4,3, 2,up,1.0);
      mo.add(4,3, 2,dn,1.0);
      mo.add(4,3, 3,up,1.0);
      mo.add(4,3, 3,dn,1.0);

      mo.add(5,2,-2,up,1.0);
      mo.add(5,2,-2,dn,1.0);
      mo.add(5,2,-1,up,1.0);
      mo.add(5,2,-1,dn,1.0);
      mo.add(5,2, 0,up,1.0);
      mo.add(5,2, 0,dn,1.0);
      mo.add(5,2, 1,up,1.0);
      mo.add(5,2, 1,dn,1.0);
      mo.add(5,2, 2,up,1.0);
      mo.add(5,2, 2,dn,1.0);
      mo.add(6,0, 0,up,1.0);
      mo.add(6,0, 0,dn,1.0);
      mo.add(6,1,-1,up,1.0);
      mo.add(6,1,-1,dn,1.0);
      mo.add(6,1, 0,up,1.0);
      mo.add(6,1, 0,dn,1.0);
      mo.add(6,1, 1,up,1.0);
      mo.add(6,1, 1,dn,1.0);

  //     cs[1] = 2;
//       cratio[1] = 0.3;
//       cs[2] = 10;
//       cratio[2] = 0.35;
//       cs[3]=18;
//       cratio[2] = 0.4;
//       cs[4] = 36;
//       cratio[4] = 0.45;
//       cs[5] = 54;
//       cratio[5] = 0.5;
//       cs[6] = 86;
//       cratio[6] = 0.55;
      break;
    }
}

inline void FillShellsHarmPot(HFAtomicOrbitals& mo, 
			      int nmax) {
  //			      vector<int>& cs, 
  //			      vector<double>& cratio){
  
  int up=1;
  int dn=-1;
  
  switch(nmax) {
  case(1):
      mo.add(0,0,0,up,1.0);
      mo.add(0,0,0,dn,1.0);
//  cs[1] = 2;
//      cratio[1] = 0.3;
      break;

    case(2):
      mo.add(0,0,0,up,1.0);
      mo.add(0,0,0,dn,1.0);

      mo.add(0,1,-1,up,1.0);
      mo.add(0,1,-1,dn,1.0);
      mo.add(0,1, 0,up,1.0);
      mo.add(0,1, 0,dn,1.0);
      mo.add(0,1, 1,up,1.0);
      mo.add(0,1, 1,dn,1.0);
     
    //   cs[1] = 2;
//       cratio[1] = 0.3;
//       cs[2] = 10;
//       cratio[2] = 0.35;
      break;

    case(3):
      mo.add(0,0,0,up,1.0);
      mo.add(0,0,0,dn,1.0);

      mo.add(0,1,-1,up,1.0);
      mo.add(0,1,-1,dn,1.0);
      mo.add(0,1, 0,up,1.0);
      mo.add(0,1, 0,dn,1.0);
      mo.add(0,1, 1,up,1.0);
      mo.add(0,1, 1,dn,1.0);

      mo.add(0,2,-2,up,1.0);
      mo.add(0,2,-2,dn,1.0);
      mo.add(0,2,-1,up,1.0);
      mo.add(0,2,-1,dn,1.0);
      mo.add(0,2, 0,up,1.0);
      mo.add(0,2, 0,dn,1.0);
      mo.add(0,2, 1,up,1.0);
      mo.add(0,2, 1,dn,1.0);
      mo.add(0,2, 2,up,1.0);
      mo.add(0,2, 2,dn,1.0);
      mo.add(1,0, 0,up,1.0);
      mo.add(1,0, 0,dn,1.0);
     
    //   cs[1] = 2;
//       cratio[1] = 0.3;
//       cs[2] = 10;
//       cratio[2] = 0.35;
//       cs[3]=18;
//       cratio[2] = 0.4;
      break;
    
    case(4):
      mo.add(0,0,0,up,1.0);
      mo.add(0,0,0,dn,1.0);

      mo.add(0,1,-1,up,1.0);
      mo.add(0,1,-1,dn,1.0);
      mo.add(0,1, 0,up,1.0);
      mo.add(0,1, 0,dn,1.0);
      mo.add(0,1, 1,up,1.0);
      mo.add(0,1, 1,dn,1.0);

      mo.add(0,2,-2,up,1.0);
      mo.add(0,2,-2,dn,1.0);
      mo.add(0,2,-1,up,1.0);
      mo.add(0,2,-1,dn,1.0);
      mo.add(0,2, 0,up,1.0);
      mo.add(0,2, 0,dn,1.0);
      mo.add(0,2, 1,up,1.0);
      mo.add(0,2, 1,dn,1.0);
      mo.add(0,2, 2,up,1.0);
      mo.add(0,2, 2,dn,1.0);
      mo.add(1,0, 0,up,1.0);
      mo.add(1,0, 0,dn,1.0);

      mo.add(0,3,-3,up,1.0);
      mo.add(0,3,-3,dn,1.0);
      mo.add(0,3,-2,up,1.0);
      mo.add(0,3,-2,dn,1.0);
      mo.add(0,3,-1,up,1.0);
      mo.add(0,3,-1,dn,1.0);
      mo.add(0,3, 0,up,1.0);
      mo.add(0,3, 0,dn,1.0);
      mo.add(0,3, 1,up,1.0);
      mo.add(0,3, 1,dn,1.0);
      mo.add(0,3, 2,up,1.0);
      mo.add(0,3, 2,dn,1.0);
      mo.add(0,3, 3,up,1.0);
      mo.add(0,3, 3,dn,1.0);    
      mo.add(1,1,-1,up,1.0);
      mo.add(1,1,-1,dn,1.0);
      mo.add(1,1, 0,up,1.0);
      mo.add(1,1, 0,dn,1.0);
      mo.add(1,1, 1,up,1.0);
      mo.add(1,1, 1,dn,1.0);

 //      cs[1] = 2;
//       cratio[1] = 0.3;
//       cs[2] = 10;
//       cratio[2] = 0.35;
//       cs[3]=18;
//       cratio[2] = 0.4;
//       cs[4] = 36;
//       cratio[4] = 0.45;
      break;
    
    case(5):
      mo.add(0,0,0,up,1.0);
      mo.add(0,0,0,dn,1.0);

      mo.add(0,1,-1,up,1.0);
      mo.add(0,1,-1,dn,1.0);
      mo.add(0,1, 0,up,1.0);
      mo.add(0,1, 0,dn,1.0);
      mo.add(0,1, 1,up,1.0);
      mo.add(0,1, 1,dn,1.0);

      mo.add(0,2,-2,up,1.0);
      mo.add(0,2,-2,dn,1.0);
      mo.add(0,2,-1,up,1.0);
      mo.add(0,2,-1,dn,1.0);
      mo.add(0,2, 0,up,1.0);
      mo.add(0,2, 0,dn,1.0);
      mo.add(0,2, 1,up,1.0);
      mo.add(0,2, 1,dn,1.0);
      mo.add(0,2, 2,up,1.0);
      mo.add(0,2, 2,dn,1.0);
      mo.add(1,0, 0,up,1.0);
      mo.add(1,0, 0,dn,1.0);

      mo.add(0,3,-3,up,1.0);
      mo.add(0,3,-3,dn,1.0);
      mo.add(0,3,-2,up,1.0);
      mo.add(0,3,-2,dn,1.0);
      mo.add(0,3,-1,up,1.0);
      mo.add(0,3,-1,dn,1.0);
      mo.add(0,3, 0,up,1.0);
      mo.add(0,3, 0,dn,1.0);
      mo.add(0,3, 1,up,1.0);
      mo.add(0,3, 1,dn,1.0);
      mo.add(0,3, 2,up,1.0);
      mo.add(0,3, 2,dn,1.0);
      mo.add(0,3, 3,up,1.0);
      mo.add(0,3, 3,dn,1.0);    
      mo.add(1,1,-1,up,1.0);
      mo.add(1,1,-1,dn,1.0);
      mo.add(1,1, 0,up,1.0);
      mo.add(1,1, 0,dn,1.0);
      mo.add(1,1, 1,up,1.0);
      mo.add(1,1, 1,dn,1.0);

      mo.add(0,4,-4,up,1.0);
      mo.add(0,4,-4,dn,1.0);
      mo.add(0,4,-3,up,1.0);
      mo.add(0,4,-3,dn,1.0);
      mo.add(0,4,-2,up,1.0);
      mo.add(0,4,-2,dn,1.0);
      mo.add(0,4,-1,up,1.0);
      mo.add(0,4,-1,dn,1.0);
      mo.add(0,4, 0,up,1.0);
      mo.add(0,4, 0,dn,1.0);
      mo.add(0,4, 1,up,1.0);
      mo.add(0,4, 1,dn,1.0);
      mo.add(0,4, 2,up,1.0);
      mo.add(0,4, 2,dn,1.0);
      mo.add(0,4, 3,up,1.0);
      mo.add(0,4, 3,dn,1.0);
      mo.add(0,4, 4,up,1.0);
      mo.add(0,4, 4,dn,1.0);

      mo.add(2,0, 0,up,1.0);
      mo.add(2,0, 0,dn,1.0);

      mo.add(1,2,-2,up,1.0);
      mo.add(1,2,-2,dn,1.0);
      mo.add(1,2,-1,up,1.0);
      mo.add(1,2,-1,dn,1.0);
      mo.add(1,2, 0,up,1.0);
      mo.add(1,2, 0,dn,1.0);
      mo.add(1,2, 1,up,1.0);
      mo.add(1,2, 1,dn,1.0);
      mo.add(1,2, 2,up,1.0);
      mo.add(1,2, 2,dn,1.0);

  //     cs[1] = 2;
//       cratio[1] = 0.3;
//       cs[2] = 10;
//       cratio[2] = 0.35;
//       cs[3]=18;
//       cratio[2] = 0.4;
//       cs[4] = 36;
//       cratio[4] = 0.45;
//       cs[5] = 54;
//       cratio[5] = 0.5;
      break;

    case(6):
      mo.add(0,0,0,up,1.0);
      mo.add(0,0,0,dn,1.0);

      mo.add(0,1,-1,up,1.0);
      mo.add(0,1,-1,dn,1.0);
      mo.add(0,1, 0,up,1.0);
      mo.add(0,1, 0,dn,1.0);
      mo.add(0,1, 1,up,1.0);
      mo.add(0,1, 1,dn,1.0);

      mo.add(0,2,-2,up,1.0);
      mo.add(0,2,-2,dn,1.0);
      mo.add(0,2,-1,up,1.0);
      mo.add(0,2,-1,dn,1.0);
      mo.add(0,2, 0,up,1.0);
      mo.add(0,2, 0,dn,1.0);
      mo.add(0,2, 1,up,1.0);
      mo.add(0,2, 1,dn,1.0);
      mo.add(0,2, 2,up,1.0);
      mo.add(0,2, 2,dn,1.0);
      mo.add(1,0, 0,up,1.0);
      mo.add(1,0, 0,dn,1.0);

      mo.add(0,3,-3,up,1.0);
      mo.add(0,3,-3,dn,1.0);
      mo.add(0,3,-2,up,1.0);
      mo.add(0,3,-2,dn,1.0);
      mo.add(0,3,-1,up,1.0);
      mo.add(0,3,-1,dn,1.0);
      mo.add(0,3, 0,up,1.0);
      mo.add(0,3, 0,dn,1.0);
      mo.add(0,3, 1,up,1.0);
      mo.add(0,3, 1,dn,1.0);
      mo.add(0,3, 2,up,1.0);
      mo.add(0,3, 2,dn,1.0);
      mo.add(0,3, 3,up,1.0);
      mo.add(0,3, 3,dn,1.0);    
      mo.add(1,1,-1,up,1.0);
      mo.add(1,1,-1,dn,1.0);
      mo.add(1,1, 0,up,1.0);
      mo.add(1,1, 0,dn,1.0);
      mo.add(1,1, 1,up,1.0);
      mo.add(1,1, 1,dn,1.0);

      mo.add(0,4,-4,up,1.0);
      mo.add(0,4,-4,dn,1.0);
      mo.add(0,4,-3,up,1.0);
      mo.add(0,4,-3,dn,1.0);
      mo.add(0,4,-2,up,1.0);
      mo.add(0,4,-2,dn,1.0);
      mo.add(0,4,-1,up,1.0);
      mo.add(0,4,-1,dn,1.0);
      mo.add(0,4, 0,up,1.0);
      mo.add(0,4, 0,dn,1.0);
      mo.add(0,4, 1,up,1.0);
      mo.add(0,4, 1,dn,1.0);
      mo.add(0,4, 2,up,1.0);
      mo.add(0,4, 2,dn,1.0);
      mo.add(0,4, 3,up,1.0);
      mo.add(0,4, 3,dn,1.0);
      mo.add(0,4, 4,up,1.0);
      mo.add(0,4, 4,dn,1.0);

      mo.add(2,0, 0,up,1.0);
      mo.add(2,0, 0,dn,1.0);

      mo.add(1,2,-2,up,1.0);
      mo.add(1,2,-2,dn,1.0);
      mo.add(1,2,-1,up,1.0);
      mo.add(1,2,-1,dn,1.0);
      mo.add(1,2, 0,up,1.0);
      mo.add(1,2, 0,dn,1.0);
      mo.add(1,2, 1,up,1.0);
      mo.add(1,2, 1,dn,1.0);
      mo.add(1,2, 2,up,1.0);
      mo.add(1,2, 2,dn,1.0);

      mo.add(0,5,-5,up,1.0);
      mo.add(0,5,-5,dn,1.0);
      mo.add(0,5,-4,up,1.0);
      mo.add(0,5,-4,dn,1.0);
      mo.add(0,5,-3,up,1.0);
      mo.add(0,5,-3,dn,1.0);
      mo.add(0,5,-2,up,1.0);
      mo.add(0,5,-2,dn,1.0);
      mo.add(0,5,-1,up,1.0);
      mo.add(0,5,-1,dn,1.0);
      mo.add(0,5, 0,up,1.0);
      mo.add(0,5, 0,dn,1.0);
      mo.add(0,5, 1,up,1.0);
      mo.add(0,5, 1,dn,1.0);
      mo.add(0,5, 2,up,1.0);
      mo.add(0,5, 2,dn,1.0);
      mo.add(0,5, 3,up,1.0);
      mo.add(0,5, 3,dn,1.0);
      mo.add(0,5, 4,up,1.0);
      mo.add(0,5, 4,dn,1.0);
      mo.add(0,5, 5,up,1.0);
      mo.add(0,5, 5,dn,1.0);

      mo.add(1,3,-3,up,1.0);
      mo.add(1,3,-3,dn,1.0);
      mo.add(1,3,-2,up,1.0);
      mo.add(1,3,-2,dn,1.0);
      mo.add(1,3,-1,up,1.0);
      mo.add(1,3,-1,dn,1.0);
      mo.add(1,3, 0,up,1.0);
      mo.add(1,3, 0,dn,1.0);
      mo.add(1,3, 1,up,1.0);
      mo.add(1,3, 1,dn,1.0);
      mo.add(1,3, 2,up,1.0);
      mo.add(1,3, 2,dn,1.0);
      mo.add(1,3, 3,up,1.0);
      mo.add(1,3, 3,dn,1.0);    
    
      mo.add(2,1,-1,up,1.0);
      mo.add(2,1,-1,dn,1.0);
      mo.add(2,1, 0,up,1.0);
      mo.add(2,1, 0,dn,1.0);
      mo.add(2,1, 1,up,1.0);
      mo.add(2,1, 1,dn,1.0);

   //    cs[1] = 2;
//       cratio[1] = 0.3;
//       cs[2] = 10;
//       cratio[2] = 0.35;
//       cs[3]=18;
//       cratio[2] = 0.4;
//       cs[4] = 36;
//       cratio[4] = 0.45;
//       cs[5] = 54;
//       cratio[5] = 0.5;
//       cs[6] = 86;
//       cratio[6] = 0.55;
      break;
    }
  }

#endif
