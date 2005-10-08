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
/***************************************************************************
 *
 * \class TinyVector
 * \brief Pooma/AppyTypes/Vecktor is modified to work with PETE.
 *
 * The POOMA Framework
 * 
 * This program was prepared by the Regents of the University of
 * California at Los Alamos National Laboratory (the University) under
 * Contract No.  W-7405-ENG-36 with the U.S. Department of Energy (DOE).
 * The University has certain rights in the program pursuant to the
 * contract and the program should not be copied or distributed outside
 * your organization.  All rights in the program are reserved by the DOE
 * and the University.  Neither the U.S.  Government nor the University
 * makes any warranty, express or implied, or assumes any liability or
 * responsibility for the use of this software
 *
 * Visit http://www.acl.lanl.gov/POOMA for more details
 *
 ***************************************************************************/

#ifndef OHMMS_TINYVECTOR_H
#define OHMMS_TINYVECTOR_H


// include files
#include <iomanip>
#include "PETE/PETE.h"
#include "OhmmsPETE/OhmmsTinyMeta.h"

//////////////////////////////////////////////////////////////////////
//
// Definition of class TinyVector.
//
//////////////////////////////////////////////////////////////////////
template<class T, unsigned D>
class TinyVector
{
public:

  typedef T Type_t;
  enum { Size = D };

  // Default Constructor initializes to zero.
  TinyVector() { 
    OTAssign<TinyVector<T,D>, T, OpAssign>::apply(*this,T(0), OpAssign());
  }

  // A noninitializing ctor.
  class DontInitialize {};
  TinyVector(DontInitialize) {}

  // Copy Constructor 
  TinyVector(const TinyVector<T,D> &rhs) {
    OTAssign< TinyVector<T,D> , TinyVector<T,D> ,OpAssign>::apply(*this,rhs, OpAssign());
  }

  // Templated TinyVector constructor.
  template<class T1, unsigned D1>
  TinyVector(const TinyVector<T1,D1> &rhs) {
    for (unsigned d=0; d<D; ++d)
      X[d] = (d < D1) ? rhs[d] : T1(0);
  }

  // Constructor from a single T
  TinyVector(const T& x00) {
    OTAssign<TinyVector<T,D>,T,OpAssign>::apply(*this,x00,OpAssign());
  }

  // Constructors for fixed dimension
  TinyVector(const T& x00, const T& x01) {
    X[0] = x00;
    X[1] = x01;
  }
  TinyVector(const T& x00, const T& x01, const T& x02) {
    X[0] = x00;
    X[1] = x01;
    X[2] = x02;
  }
  TinyVector(const T& x00, const T& x01, const T& x02, const T& x03) {
    X[0] = x00;
    X[1] = x01;
    X[2] = x02;
    X[3] = x03;
   }

  // Destructor 
  ~TinyVector() { }

  inline int size() const { return D;}

  inline int byteSize() const { return D*sizeof(T);}

  inline TinyVector<T,D>& operator=(const TinyVector<T,D> &rhs)
  {
    OTAssign<TinyVector<T,D>,TinyVector<T,D>,OpAssign>::apply(*this,rhs,OpAssign());
    return *this;
  }
  
  template<class T1>
  inline TinyVector<T,D>& operator=(const TinyVector<T1,D> &rhs)
  {
    OTAssign<TinyVector<T,D>,TinyVector<T1,D>,OpAssign>::apply(*this,rhs,OpAssign());
    return *this;
  }

  inline TinyVector<T,D>& operator=(const T& rhs)
  {
    OTAssign<TinyVector<T,D>,T,OpAssign>::apply(*this, rhs, OpAssign());
    return *this;
  }

  // Get and Set Operations
  inline Type_t& operator[](unsigned int i)
  { 
    return X[i];
  }
  inline Type_t operator[](unsigned int i) const
  { 
    return X[i];
  }
  inline Type_t& operator()(unsigned int i)
  { 
    return X[i];
  }
  inline Type_t operator()( unsigned int i) const
  { 
    return X[i];
  }

  inline Type_t* begin(){ return  X; }
  inline const Type_t* begin() const { return  X; }
  inline Type_t* end(){ return  X+D; }
  inline const Type_t* end() const { return  X+D; }

  // Comparison operators.
  //bool operator==(const TinyVector<T,D>& that) const {
  //  return MetaCompareArrays<T,T,D>::apply(X,that.X);
  //}
  //bool operator!=(const TinyVector<T,D>& that) const {
  //  return !(*this == that);
  //}

  //----------------------------------------------------------------------
  // parallel communication

  //Message& putMessage(Message& m) const {
  //  m.setCopy(true);
  //  ::putMessage(m, X, X + D);
  //    return m;
  //}

  //Message& getMessage(Message& m) {
  //  ::getMessage(m, X, X + D);
  //  return m;
  //}

  template<class Msg> inline Msg& putMessage(Msg& m) {
      m.Pack(X,Size);
      return m;
    }

  template<class Msg> inline Msg& getMessage(Msg& m) {
      m.Unpack(X,Size);
      return m;
    }

private:

  // Just store D elements of type T.
  T X[Size];

};

//OHMMS_TINYVECTOR_ACCUM_OPERATORS(operator+=,OpAddAssign)
//OHMMS_TINYVECTOR_ACCUM_OPERATORS(operator-=,OpSubtractAssign)
//OHMMS_TINYVECTOR_ACCUM_OPERATORS(operator*=,OpMultiplyAssign)
//OHMMS_TINYVECTOR_ACCUM_OPERATORS(operator/=,OpDivideAssign)
// Adding binary operators using macro defined in OhmmsTinyMeta.h
OHMMS_META_ACCUM_OPERATORS(TinyVector,operator+=,OpAddAssign)
OHMMS_META_ACCUM_OPERATORS(TinyVector,operator-=,OpSubtractAssign)
OHMMS_META_ACCUM_OPERATORS(TinyVector,operator*=,OpMultiplyAssign)
OHMMS_META_ACCUM_OPERATORS(TinyVector,operator/=,OpDivideAssign)

OHMMS_META_BINARY_OPERATORS(TinyVector,operator+,OpAdd)
OHMMS_META_BINARY_OPERATORS(TinyVector,operator-,OpSubtract)
OHMMS_META_BINARY_OPERATORS(TinyVector,operator*,OpMultiply)
OHMMS_META_BINARY_OPERATORS(TinyVector,operator/,OpDivide)


//----------------------------------------------------------------------
// dot product
//----------------------------------------------------------------------
template < class T1, class T2, unsigned D >
inline typename BinaryReturn<T1,T2,OpMultiply>::Type_t
dot(const TinyVector<T1,D> &lhs, const TinyVector<T2,D> &rhs) 
{
  return OTDot< TinyVector<T1,D> , TinyVector<T2,D> > :: apply(lhs,rhs);
}

//----------------------------------------------------------------------
// cross product
//----------------------------------------------------------------------

template < class T1, class T2, unsigned D >
inline TinyVector<typename BinaryReturn<T1,T2,OpMultiply>::Type_t,D>
cross(const TinyVector<T1,D> &lhs, const TinyVector<T2,D> &rhs) 
{
  return OTCross< TinyVector<T1,D> , TinyVector<T2,D> > :: apply(lhs,rhs);
}

//----------------------------------------------------------------------
// cross product
//----------------------------------------------------------------------

template < class T1, class T2, unsigned D >
inline Tensor<typename BinaryReturn<T1,T2,OpMultiply>::Type_t,D>
outerProduct(const TinyVector<T1,D> &lhs, const TinyVector<T2,D> &rhs)
{
  return OuterProduct< TinyVector<T1,D> , TinyVector<T2,D> > :: apply(lhs,rhs);
}

//----------------------------------------------------------------------
// I/O
template<class T>
struct printTinyVector {};

// specialized for Vector<TinyVector<T,D> >
template<class T, unsigned D>
struct printTinyVector< TinyVector<T,D>  > {
  inline static void 
  print(std::ostream& os, const TinyVector<T,D>& r) {
    for(int d=0; d<D; d++) 
      os << std::setw(18) << std::setprecision(10) << r[d];
  }
};

// specialized for Vector<TinyVector<T,2> >
template<class T>
struct printTinyVector<TinyVector<T,2> > {
  inline static void 
  print(std::ostream& os, const TinyVector<T,2>& r) {
      os << std::setw(18) << std::setprecision(10) << r[0]
         << std::setw(18) << std::setprecision(10) << r[1];
  }
};

// specialized for Vector<TinyVector<T,3> >
template<class T>
struct printTinyVector<TinyVector<T,3> > {
  inline static void 
  print(std::ostream& os, const TinyVector<T,3>& r) {
      os << std::setw(18) << std::setprecision(10) << r[0]
         << std::setw(18) << std::setprecision(10) << r[1] 
         << std::setw(18) << std::setprecision(10) << r[2];
  }
};


template<class T, unsigned D>
std::ostream& operator<<(std::ostream& out, const TinyVector<T,D>& rhs)
{
  printTinyVector<TinyVector<T,D> >::print(out,rhs);
  return out;
}

template<class T, unsigned D>
std::istream& operator>>(std::istream& is, TinyVector<T,D>& rhs)
{
  //printTinyVector<TinyVector<T,D> >::print(out,rhs);
  for(int i=0; i<D; i++)  is >> rhs[i];
  return is;
}

#endif // VEKTOR_H

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
