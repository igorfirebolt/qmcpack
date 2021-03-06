#ifndef QMCPLUSPLUS_LRBREAKUP_H
#define QMCPLUSPLUS_LRBREAKUP_H

#include "Configuration.h"
#include "Particle/ParticleSet.h"
#include "LongRange/KContainer.h"
#include "Numerics/Blasf.h"
#include <cassert>

namespace qmcplusplus {

  template<class BreakupBasis>
    class LRBreakup: public QMCTraits {

  private:
    //Typedef for the lattice-type. We don't need the full particle-set.
    typedef ParticleSet::ParticleLayout_t ParticleLayout_t;

    //We use an internal k-list with degeneracies to do the breakup.
    //We do this because the number of vectors is much larger than we'd
    //use elsewhere.
    void AddKToList(RealType k, RealType degeneracy=1.0);

  public:
    //The basis to be used for breakup.
    BreakupBasis& Basis;

    vector<TinyVector<RealType,2> > KList;
    //kc = k-space cutoff for long-range sums
    //kmax = largest k used for performing the breakup
    //kcont = k at which approximate (spherical shell) degeneracies are used.
    void SetupKVecs(RealType kc, RealType kcont, RealType kmax);

    //Fk is FT of F_full(r) up to kmax
    //adjust is used for constraining values in the breakup

/* REPLACED SO WE CAN USE TYPES OTHER THAN STL VECTOR.
    RealType DoBreakup(const vector<RealType> &Fk, vector<RealType> &t, 
		       const vector<bool> &adjust);
    RealType DoBreakup(const vector<RealType> &Fk, vector<RealType> &t);   
*/
    RealType DoBreakup(RealType *Fk, RealType *t, RealType *adjust);

    RealType DoBreakup(RealType *Fk, RealType *t) {
      const RealType tolerance = 1.0e-16;


      //t must be allocated up to Basis.NumBasisElem();
      //Fk must be allocated and filled up to KList.size();

      //  assert(t.size()==Basis.NumBasisElem());

      Matrix<RealType> A;
      vector<RealType> b;
      Matrix<RealType> cnk;

      int numElem = Basis.NumBasisElem(); //t.size();
      A.resize(numElem, numElem);
      b.resize(numElem,0.0);
      cnk.resize(numElem,KList.size());

      // Fill in cnk.
      for (int n=0; n<numElem; n++) {
        for (int ki=0; ki<KList.size(); ki++) {
          RealType k = KList[ki][0];
          cnk(n,ki) = Basis.c(n,k);
        }
      }

      // Now, fill in A and b
      A = 0.0;
      for (int l=0; l<numElem; l++) {
        for (int ki=0; ki<KList.size(); ki++) {
          b[l] += KList[ki][1]*Fk[ki] * cnk(l, ki);
          for (int n=0; n<numElem; n++) 
            A(l,n) += KList[ki][1]*cnk(l,ki)*cnk(n,ki);
        }
      }

      //////////////////////////
      //Do the SVD:
      // Matrix<RealType> U(numElem, numElem), V(numElem, numElem);
      // vector<RealType> S(numElem), Sinv(numElem);
      //////////////////////////
      //  SVdecomp(A, U, S, V);
      //////////////////////////
      int M = A.rows();
      int N = A.cols();
      Matrix<RealType> Atrans(N,M);
      Matrix<RealType> U, V;
      U.resize(std::min(M,N),M);
      V.resize(N,std::min(M,N));
      vector<RealType> S, Sinv;
      S.resize(std::min(N,M));
      //Do the transpose
      for(int i=0; i<M; i++){
        for(int j=0;j<N;j++)
          Atrans(j,i) = A(i,j);
      }

      char JOBU = 'S';
      char JOBVT = 'S';
      int LDA = M; int LDU = M;
      int LDVT = std::min(M,N);
      int LWORK = 10*std::max(3*std::min(N,M)+std::max(M,N),5*std::min(M,N));
      std::vector<RealType> WORK(LWORK);
      int INFO;
      dgesvd(&JOBU,&JOBVT,&M,&N,Atrans.data(),&LDA,&S[0],U.data(),
          &LDU,V.data(),&LDVT,&WORK[0],&LWORK,&INFO);
      assert(INFO==0);

      int ur = U.rows();
      int uc = U.cols();
      Matrix<RealType> Utrans(uc,ur);
      for(int i=0; i<ur; i++){
        for(int j=0;j<uc;j++)
          Utrans(j,i) = U(i,j);
      }

      U.resize(uc,ur);
      U=Utrans;

      ///////////////////////////////////
      // Zero out near-singular values
      RealType Smax=S[0];
      for (int i=1; i<S.size(); i++)
        Smax = std::max(S[i],Smax);

      Sinv.resize(S.size());

      for (int i=0; i<S.size(); i++)
        Sinv[i] = (S[i] < (tolerance*Smax)) ? 0.0 : (1.0/S[i]);
      int numSingular = 0;

      for (int i=0; i<Sinv.size(); i++)
        if (Sinv[i] == 0.0) numSingular++;
      if (numSingular > 0)
        cout << "There were " << numSingular << " singular values in breakup.\n";
      for(int i=0; i<numElem; i++) 
        t[i] = 0.0;
      // Compute t_n, removing singular values
      for (int i=0; i<numElem; i++) {
        RealType coef = 0.0;
        for (int j=0; j<numElem; j++)
          coef += U(j,i) * b[j];
        coef *= Sinv[i];
        for (int k=0; k<numElem; k++)
          t[k] += coef * V(k,i);
      }

      // Calculate chi-squared
      RealType Yk, chi2;
      chi2 = 0.0;
      for (int ki=0; ki<KList.size(); ki++) {
        Yk = Fk[ki];
        for (int n=0; n<numElem; n++) {
          Yk -= cnk(n,ki)*t[n];
        }
        chi2 += KList[ki][1]*Yk*Yk;
      }
      return (chi2);
  }


    //The constructor. Call the constructor of basis...
    //set up the basis parameters too.
    LRBreakup (BreakupBasis& bref) : Basis(bref) 
      {/*Do Nothing*/}
  };

template<class BreakupBasis>
void 
LRBreakup<BreakupBasis>::AddKToList(RealType k, 
				     RealType degeneracy /* =1.0 */) {
  //Search for this k already in list
  int ki=0;
  while((ki < KList.size()) && (fabs(k-KList[ki][0]) > 1.0e-12))
    ki++;

  if(ki==KList.size()) {
    TinyVector<RealType,2> temp(k,degeneracy);
    KList.push_back(temp);
  }
  else
    KList[ki][1] += degeneracy;
}


template<class BreakupBasis>
void
LRBreakup<BreakupBasis>::SetupKVecs(RealType kc, RealType kcont, 
				     RealType kmax) {
  //Add low |k| ( < kcont) k-points with exact degeneracy
  KContainer kexact(Basis.get_Lattice());
  kexact.UpdateKLists(Basis.get_Lattice(),kcont);

  RealType numk = 0.0;

  //Add these vectors to the internal list
  RealType modk2;
  for(int ki=0; ki<kexact.numk; ki++) {
    modk2 = dot(kexact.kpts_cart[ki],kexact.kpts_cart[ki]);
    if(modk2 > (kc*kc)) { //Breakup needs kc < k < kcont.
      AddKToList(sqrt(modk2));
      numk++;
    } 
  }

  //Add high |k| ( >kcont, <kmax) k-points with approximate degeneracy
  //Volume of 1 K-point is (2pi)^3/(a1.a2^a3)
  RealType kelemvol = 8*M_PI*M_PI*M_PI/Basis.get_CellVolume();

  //Generate 4000 shells:
  const int N=4000;
  RealType deltak = (kmax-kcont)/N;
  for(int i=0; i<N; i++) {
    RealType k1 = kcont + deltak*i;
    RealType k2 = k1 + deltak;
    RealType kmid = 0.5*(k1+k2);
    RealType shellvol = 4.0*M_PI*(k2*k2*k2-k1*k1*k1)/3.0;
    RealType degeneracy = shellvol/kelemvol;
    AddKToList(kmid,degeneracy);
    numk += degeneracy;
  }

  //numk now contains the total number of vectors. 
  //this->klist.size() contains the number of unique vectors.
}


//IBM compiler cannot handle the template member function outside of
//the class definition
////Do the breakup
//template<class BreakupBasis>
//typename LRBreakup<BreakupBasis>::RealType
//LRBreakup<BreakupBasis>::DoBreakup(RealType *Fk, RealType *t) {
//  const RealType tolerance = 1.0e-16;
//
//
//  //t must be allocated up to Basis.NumBasisElem();
//  //Fk must be allocated and filled up to KList.size();
//
//  //  assert(t.size()==Basis.NumBasisElem());
//
//  Matrix<RealType> A;
//  vector<RealType> b;
//  Matrix<RealType> cnk;
//
//  int numElem = Basis.NumBasisElem(); //t.size();
//  A.resize(numElem, numElem);
//  b.resize(numElem,0.0);
//  cnk.resize(numElem,KList.size());
//
//  // Fill in cnk.
//  for (int n=0; n<numElem; n++) {
//    for (int ki=0; ki<KList.size(); ki++) {
//      RealType k = KList[ki][0];
//      cnk(n,ki) = Basis.c(n,k);
//    }
//  }
//
//  // Now, fill in A and b
//  A = 0.0;
//  for (int l=0; l<numElem; l++) {
//    for (int ki=0; ki<KList.size(); ki++) {
//      b[l] += KList[ki][1]*Fk[ki] * cnk(l, ki);
//      for (int n=0; n<numElem; n++) 
//	A(l,n) += KList[ki][1]*cnk(l,ki)*cnk(n,ki);
//    }
//  }
//
//  //////////////////////////
//  //Do the SVD:
//  // Matrix<RealType> U(numElem, numElem), V(numElem, numElem);
//  // vector<RealType> S(numElem), Sinv(numElem);
//  //////////////////////////
//  //  SVdecomp(A, U, S, V);
//  //////////////////////////
//  int M = A.rows();
//  int N = A.cols();
//  Matrix<RealType> Atrans(N,M);
//  Matrix<RealType> U, V;
//  U.resize(std::min(M,N),M);
//  V.resize(N,std::min(M,N));
//  vector<RealType> S, Sinv;
//  S.resize(std::min(N,M));
//  //Do the transpose
//  for(int i=0; i<M; i++){
//    for(int j=0;j<N;j++)
//      Atrans(j,i) = A(i,j);
//  }
//
//  char JOBU = 'S';
//  char JOBVT = 'S';
//  int LDA = M; int LDU = M;
//  int LDVT = std::min(M,N);
//  int LWORK = 10*std::max(3*std::min(N,M)+std::max(M,N),5*std::min(M,N));
//  std::vector<RealType> WORK(LWORK);
//  int INFO;
//  dgesvd(&JOBU,&JOBVT,&M,&N,Atrans.data(),&LDA,&S[0],U.data(),
//	 &LDU,V.data(),&LDVT,&WORK[0],&LWORK,&INFO);
//  assert(INFO==0);
//
//  int ur = U.rows();
//  int uc = U.cols();
//  Matrix<RealType> Utrans(uc,ur);
//  for(int i=0; i<ur; i++){
//    for(int j=0;j<uc;j++)
//      Utrans(j,i) = U(i,j);
//  }
//
//  U.resize(uc,ur);
//  U=Utrans;
//
//  ///////////////////////////////////
//  // Zero out near-singular values
//  RealType Smax=S[0];
//  for (int i=1; i<S.size(); i++)
//    Smax = std::max(S[i],Smax);
//
//  Sinv.resize(S.size());
//
//  for (int i=0; i<S.size(); i++)
//    Sinv[i] = (S[i] < (tolerance*Smax)) ? 0.0 : (1.0/S[i]);
//  int numSingular = 0;
//
//  for (int i=0; i<Sinv.size(); i++)
//    if (Sinv[i] == 0.0) numSingular++;
//  if (numSingular > 0)
//    cout << "There were " << numSingular << " singular values in breakup.\n";
//  for(int i=0; i<numElem; i++) 
//    t[i] = 0.0;
//  // Compute t_n, removing singular values
//  for (int i=0; i<numElem; i++) {
//    RealType coef = 0.0;
//    for (int j=0; j<numElem; j++)
//      coef += U(j,i) * b[j];
//    coef *= Sinv[i];
//    for (int k=0; k<numElem; k++)
//      t[k] += coef * V(k,i);
//  }
//
//  // Calculate chi-squared
//  RealType Yk, chi2;
//  chi2 = 0.0;
//  for (int ki=0; ki<KList.size(); ki++) {
//    Yk = Fk[ki];
//    for (int n=0; n<numElem; n++) {
//      Yk -= cnk(n,ki)*t[n];
//    }
//    chi2 += KList[ki][1]*Yk*Yk;
//  }
//  return (chi2);
//}


//Do the constrained breakup
template<class BreakupBasis>
typename LRBreakup<BreakupBasis>::RealType
LRBreakup<BreakupBasis>::DoBreakup(RealType *Fk, 
				   RealType *t,
				   RealType *adjust) {
  const RealType tolerance = 1.0e-16;

  //t and adjust must be allocated up to Basis.NumBasisElem();
  //Fk must be allocated and filled up to KList.size();

  //  assert(t.size()==adjust.size());
  //  assert(t.size()==Basis.NumBasisElem());

  Matrix<RealType> A;
  vector<RealType> b;
  Matrix<RealType> cnk;
  int N = Basis.NumBasisElem(); //t.size();
  A.resize(N,N);
  b.resize(N,0.0);
  cnk.resize(N,KList.size());

  //Fill in cnk.
  for (int n=0; n<N; n++) {
    for (int ki=0; ki<KList.size(); ki++) {
      RealType k = KList[ki][0];
      cnk(n,ki) = Basis.c(n,k);
    }
  }

  //Fill in A and b
  A = 0.0;
  for (int l=0; l<N; l++) {
    for (int ki=0; ki<KList.size(); ki++) {
      b[l] += KList[ki][1]*Fk[ki] * cnk(l, ki);
      for (int n=0; n<N; n++) 
	A(l,n) += KList[ki][1]*cnk(l,ki)*cnk(n,ki);
    }
  }

  //Reduce for constraints
  int M = N;
  for (int i=0; i<N; i++) 
    if (!adjust[i])
      M--;

  //The c is for "constrained"
  Matrix<RealType> Ac;
  Ac.resize(M,M);
  vector<RealType> bc(M,0.0), tc(M,0.0);

  //Build constrained Ac and bc
  int j=0;
  for (int col=0; col<N; col++) {
    if (adjust[col]) {
      // Copy column a A to Ac
      int i=0;
      for (int row=0; row<N; row++) 
	if (adjust[row]) {
	  Ac(i,j) = A(row,col);
	  i++;
	}
      j++;
    }
    else {
      // Otherwise, subtract t(col)*A(:,col) from bc
      for (int row=0; row<N; row++)
      	b[row] -= A(row,col)*t[col];
    }
  }
  j=0;
  for (int row=0; row<N; row++)
    if (adjust[row]) {
      bc[j] = b[row];
      j++;
    }

  // Do SVD:
  // -------
  // Matrix<RealType> U(M, M), V(M, M);
  // vector<RealType> S(M), Sinv(M);
  // SVdecomp(Ac, U, S, V);
  ////////////////////////////////
  int m = Ac.rows();
  int n = Ac.cols();
  Matrix<RealType> Atrans(n,m);
  Matrix<RealType> U, V;
  U.resize(std::min(m,n),m);
  V.resize(n,std::min(m,n));
  vector<RealType> S, Sinv;
  S.resize(std::min(n,m));
  //do the transpose
  for(int i=0; i<m; i++){
    for(int j=0;j<n;j++)
      Atrans(j,i) = Ac(i,j);
  }

  char JOBU = 'S';
  char JOBVT = 'S';
  int LDA = m; int LDU = m;
  int LDVT = std::min(m,n);
  int LWORK = 10*std::max(3*std::min(n,m)+std::max(m,n),5*std::min(m,n));
  vector<RealType> WORK(LWORK);
  int INFO;
  dgesvd(&JOBU,&JOBVT,&m,&n,Atrans.data(),&LDA,&S[0],U.data(),
	 &LDU,V.data(),&LDVT,&WORK[0],&LWORK,&INFO);
  assert(INFO==0);

  int ur = U.rows();
  int uc = U.cols();
  Matrix<RealType> Utrans(uc,ur);
  for(int i=0; i<ur; i++){
    for(int j=0;j<uc;j++)
      Utrans(j,i) = U(i,j);
  }

  U.resize(uc,ur);
  U=Utrans;

  //////////////////////////////////
  // Zero out near-singular values
  RealType Smax=S[0];
  for (int i=1; i<M; i++)
    Smax = std::max(S[i],Smax);

  for (int i=0; i<M; i++)
    if (S[i] < 0.0)
      cout << "negative singlar value.\n";

  //  perr << "Smax = " << Smax << endl;
  Sinv.resize(S.size());
  for (int i=0; i<M; i++)
    Sinv[i] = (S[i] < (tolerance*Smax)) ? 0.0 : (1.0/S[i]);
  int numSingular = 0;
  for (int i=0; i<Sinv.size(); i++)
    if (Sinv[i] == 0.0)
      numSingular++;
  if (numSingular > 0)
    cout << "There were " << numSingular << " singular values in breakup.\n";
  // Compute t_n, removing singular values
  for (int i=0; i<M; i++) {
    RealType coef = 0.0;
    for (int j=0; j<M; j++)
      coef += U(j,i) * bc[j];
    coef *= Sinv[i];
    for (int k=0; k<M; k++)
      tc[k] += coef * V(k,i);
  }

  // Now copy tc values into t
  j=0;
  for (int i=0; i<N; i++)
    if (adjust[i]) {
      t[i] = tc[j];
      j++;
    }

  // Calculate chi-squared
  RealType Yk, chi2;
  chi2 = 0.0;
  for (int ki=0; ki<KList.size(); ki++) {
    Yk = Fk[ki];
    for (int n=0; n<N; n++) {
      Yk -= cnk(n,ki)*t[n];
    }
    chi2 += KList[ki][1]*Yk*Yk;
  }
  return (chi2);
}
}

#endif
