//////////////////////////////////////////////////////////////////
// (c) Copyright 2007-  by Jeongnim Kim
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
#ifndef QMCPLUSPLUS_CUBICFUNCTORSFORJASTROW_H
#define QMCPLUSPLUS_CUBICFUNCTORSFORJASTROW_H
#include "Numerics/OneDimGridBase.h"
#include "Numerics/CubicBspline.h"
#include "Optimize/VarList.h"
#include "Numerics/OptimizableFunctorBase.h"

namespace qmcplusplus {

  /** A numerical functor
   *
   * implements interfaces to be used for Jastrow functions
   * - OneBodyJastrow<NumericalJastrow>
   * - TwoBodyJastrow<NumericalJastrow>
   */
  template <typename RT>
    struct CubicBsplineSingle: public OptimizableFunctorBase<RT> {

      ///typedef of the source functor
      typedef OptimizableFunctorBase<RT> FNIN;
      ///typedef for the argument
      typedef typename FNIN::real_type real_type;
      ///typedef for the argument
      typedef CubicBspline<RT,LINEAR_1DGRID,FIRSTDERIV_CONSTRAINTS> FNOUT;
      ///typedef for the grid
      typedef OneDimGridBase<real_type> grid_type;

      FNIN *InFunc;
      FNOUT *OutFunc;
      int NumGridPoints;
      real_type Rmax;
      real_type GridDelta;
      real_type Y;
      real_type dY;
      real_type d2Y;


      ///constructor
      CubicBsplineSingle(): InFunc(0), OutFunc(0) { }
      ///constructor with arguments
      CubicBsplineSingle(FNIN* in_, grid_type* agrid): InFunc(0), OutFunc(0) {
        initialize(in_,agrid);
      }
      ///constructor with arguments
      CubicBsplineSingle(FNIN* in_, real_type rc, int npts):InFunc(0), OutFunc(0){
        initialize(in_,rc,npts);
      }
      ///set the input, analytic function
      void setInFunc(FNIN* in_) { InFunc=in_;}
      ///set the output numerical function
      void setOutFunc(FNOUT* out_) { OutFunc=out_;}
      ///reset the input/output function
      inline void reset() {
        if(!InFunc)
        {
          app_error() << "  CubicSplineJastrow::reset failed due to null input function " << endl;
          OHMMS::Controller->abort();
        }
        if(!OutFunc) OutFunc = new FNOUT;

        InFunc->reset();
        typename FNOUT::container_type datain(NumGridPoints);
        real_type r=0;
        for(int i=0; i<NumGridPoints; i++, r+=GridDelta) 
        {
          datain[i] = InFunc->f(r);
        }
        OutFunc->Init(0.0,Rmax,datain,true,InFunc->df(0.0),0.0);
      }

      /** evaluate everything: value, first and second derivaties
       */
      inline real_type evaluate(real_type r, real_type& dudr, real_type& d2udr2) {
        return OutFunc->splint(r,dudr,d2udr2);
      }

      /** evaluate value only
       */
      inline real_type evaluate(real_type r) {
        return OutFunc->splint(r);
      }
      
      /** evaluate value only
       *
       * Function required for SphericalBasisSet
       */
      inline real_type evaluate(real_type r, real_type rinv) 
      {
        return Y=OutFunc->splint(r);
      }

      /** evaluate everything: value, first and second derivaties
       * 
       * Function required for SphericalBasisSet
       */
      inline real_type evaluateAll(real_type r, real_type rinv) 
      {
        return Y=OutFunc->splint(r,dY,d2Y);
      }

      /** implement the virtual function of OptimizableFunctorBase */
      inline real_type f(real_type r) 
      {
        return OutFunc->splint(r);
      }

      /** implement the virtual function of OptimizableFunctorBase  */
      inline real_type df(real_type r) {
        real_type dudr,d2udr2;
        OutFunc->splint(r,dudr,d2udr2);
        return dudr;
      }

      bool put(xmlNodePtr cur) 
      {
        return InFunc->put(cur);
      }

      void addOptimizables( VarRegistry<real_type>& vlist)
      {
        InFunc->addOptimizables(vlist);
      }

      void print(ostream& os) {
        real_type r=0;
        for(int i=0; i<NumGridPoints; i++, r+=GridDelta) 
          os << r << " " << OutFunc->splint(r) << endl;
      }

      ///set the input, analytic function
      void initialize(FNIN* in_, grid_type* agrid) { 
        initialize(in_,agrid->rmax(),agrid->size());
      }

      void initialize(FNIN* in_, real_type rmax, int npts) 
      { 
        InFunc=in_;
        Rmax=rmax;
        NumGridPoints=npts;
        GridDelta=Rmax/static_cast<real_type>(NumGridPoints-1);
        reset();
      }
    };

  /** A numerical functor
   *
   * implements interfaces to be used for Jastrow functions
   * - OneBodyJastrow<NumericalJastrow>
   * - TwoBodyJastrow<NumericalJastrow>
   */
  template <typename RT>
    struct CubicSplineBasisSet: public OptimizableFunctorBase<RT> {

      ///typedef of the source functor
      typedef OptimizableFunctorBase<RT> FNIN;
      ///typedef for the argument
      typedef typename FNIN::real_type real_type;
      ///typedef for the argument
      typedef CubicBspline<RT,LINEAR_1DGRID,FIRSTDERIV_CONSTRAINTS> FNOUT;
      ///typedef for the grid
      typedef OneDimGridBase<real_type> grid_type;

      FNIN *InFunc;
      FNOUT *OutFunc;
      int NumGridPoints;
      real_type Rmax;
      real_type GridDelta;

      ///constructor
      CubicSplineBasisSet(): InFunc(0), OutFunc(0) { }
      ///constructor with arguments
      CubicSplineBasisSet(FNIN* in_, grid_type* agrid){
        initialize(in_,agrid);
      }
      ///set the input, analytic function
      void setInFunc(FNIN* in_) { InFunc=in_;}
      ///set the output numerical function
      void setOutFunc(FNOUT* out_) { OutFunc=out_;}
      ///reset the input/output function
      inline void reset() {
        if(!InFunc)
        {
          app_error() << "  CubicSplineJastrow::reset failed due to null input function " << endl;
          OHMMS::Controller->abort();
        }
        if(!OutFunc) OutFunc = new FNOUT;

        InFunc->reset();
        typename FNOUT::container_type datain(NumGridPoints);
        real_type r=0;
        for(int i=0; i<NumGridPoints; i++, r+=GridDelta) datain[i] = InFunc->f(r);
        OutFunc->Init(0.0,Rmax,datain,true,InFunc->df(0.0),0.0);
      }

      /** evaluate everything: value, first and second derivaties
      */
      inline real_type evaluate(real_type r, real_type& dudr, real_type& d2udr2) {
        return OutFunc->splint(r,dudr,d2udr2);
      }

      /** evaluate value only
      */
      inline real_type evaluate(real_type r) {
        return OutFunc->splint(r);
      }

      /** implement the virtual function of OptimizableFunctorBase */
      real_type f(real_type r) {
        return OutFunc->splint(r);
      }

      /** implement the virtual function of OptimizableFunctorBase  */
      real_type df(real_type r) {
        real_type dudr,d2udr2;
        OutFunc->splint(r,dudr,d2udr2);
        return dudr;
      }

      bool put(xmlNodePtr cur) 
      {
        return InFunc->put(cur);
      }

      void addOptimizables( VarRegistry<real_type>& vlist)
      {
        InFunc->addOptimizables(vlist);
      }

      void print(ostream& os) {
        real_type r=0;
        for(int i=0; i<NumGridPoints; i++, r+=GridDelta) 
          os << r << " " << OutFunc->splint(r) << endl;
      }

      ///set the input, analytic function
      void initialize(FNIN* in_, grid_type* agrid) { 
        Rmax=agrid->rmax();
        NumGridPoints=agrid->size();
        GridDelta=Rmax/static_cast<real_type>(NumGridPoints-1);
        InFunc=in_;
        reset();
      }
    };

//  /** A numerical functor
//   *
//   * implements interfaces to be used for Jastrow functions
//   * - OneBodyJastrow<NumericalJastrow>
//   * - TwoBodyJastrow<NumericalJastrow>
//   */
//  template <class RT>
//    struct SplineJastrow: public OptimizableFunctorBase<RT> {
//
//      ///typedef of the source functor
//      typedef OptimizableFunctorBase<RT> FNIN;
//      ///typedef for the argument
//      typedef typename FNIN::real_type real_type;
//      ///typedef of the target functor
//      typedef OneDimCubicSpline<real_type,real_type>  FNOUT;
//
//      real_type Rmax;
//      FNIN *InFunc;
//      FNOUT *OutFunc;
//
//      ///constructor
//      SplineJastrow(): InFunc(0), OutFunc(0) { }
//      ///constructor with arguments
//      SplineJastrow(FNIN* in_, typename FNOUT::grid_type* agrid){
//        initialize(in_,agrid);
//      }
//      ///set the input, analytic function
//      void setInFunc(FNIN* in_) { InFunc=in_;}
//      ///set the output numerical function
//      void setOutFunc(FNOUT* out_) { OutFunc=out_;}
//      ///reset the input/output function
//      inline void reset() {
//        InFunc->reset();
//        //reference to the output functions grid
//        const typename FNOUT::grid_type& grid = OutFunc->grid();
//        //set cutoff function
//        int last=grid.size()-1;
//        for(int i=0; i<grid.size(); i++) {
//          (*OutFunc)(i) = InFunc->f(grid(i));
////	  cout << grid(i) << "   " << (*OutFunc)(i) << endl;
//        }
//	(*OutFunc)(last)=0.0;
//        //boundary conditions
//        real_type deriv1=InFunc->df(grid(0));
//        real_type deriv2=0.0;
//        OutFunc->spline(0,deriv1,last,deriv2);
//        Rmax=grid(last);
//      }
//
//      /** evaluate everything: value, first and second derivaties
//      */
//      inline real_type evaluate(real_type r, real_type& dudr, real_type& d2udr2) {
//        return OutFunc->splint(r,dudr,d2udr2);
//      }
//
//      /** evaluate value only
//      */
//      inline real_type evaluate(real_type r) {
//        return OutFunc->splint(r);
//      }
//
//      /** implement the virtual function of OptimizableFunctorBase */
//      real_type f(real_type r) {
//        return OutFunc->splint(r);
//      }
//
//      /** implement the virtual function of OptimizableFunctorBase  */
//      real_type df(real_type r) {
//        real_type dudr,d2udr2;
//        OutFunc->splint(r,dudr,d2udr2);
//        return dudr;
//      }
//
//      bool put(xmlNodePtr cur) 
//      {
//        return InFunc->put(cur);
//      }
//
//      void addOptimizables( VarRegistry<real_type>& vlist)
//      {
//        InFunc->addOptimizables(vlist);
//      }
//      void print(ostream& os) {
//        const typename FNOUT::grid_type& grid = OutFunc->grid();
//        for(int i=0; i<grid.size(); i++) {
//          cout << grid(i) << " " << (*OutFunc)(i) << endl;
//        }
//      }
//
//      ///set the input, analytic function
//      void initialize(FNIN* in_, typename FNOUT::grid_type* agrid) { 
//        InFunc=in_;
//        setOutFunc(new FNOUT(agrid));
//        reset();
//      }
//    };
//
}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1672 $   $Date: 2007-01-30 14:45:16 -0600 (Tue, 30 Jan 2007) $
 * $Id: NumericalJastrowFunctor.h 1672 2007-01-30 20:45:16Z jnkim $ 
 ***************************************************************************/
