//////////////////////////////////////////////////////////////////
// (c) Copyright 1998-2002,2003- by Jeongnim Kim 
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim
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
/** @file NRCOptimization.h
 * @brief Declaration and definition of optimization class using Numerical Recipe
 */
#ifndef QMCPLUSPLUS_NRC_OPTIMIZATION_H
#define QMCPLUSPLUS_NRC_OPTIMIZATION_H

#include "Numerics/MatrixOperators.h"
#include "Numerics/DeterminantOperators.h"

#include <math.h>
#if (__GNUC__ == 2)
#include <algo.h>
#else
#include <algorithm>
#endif
#include <limits>
//template<class T>
//inline void shift (T& a, T& b, T& c, T d) {
//  a = b; b= c; c = d;
//}
//double sign(double a, double b) {
//  return (b > 0.0)? fabs(a): -fabs(a); 
//}

template<class T>
struct sign2 { };

template<>
struct sign2<double> {
   inline static double apply(double a, double b) { return (b > 0.0)? fabs(a): -fabs(a); }
};

template<class T>
struct NRCOptimization {

  typedef T Return_t;

  /** number of line iteration for Brent method */
  int ITMAX;

  /** maximum CG mixture for line minimization 
   * 
   * y'=y + Lambda*cg where Lambda < LambdaMax
   */
  Return_t LambdaMax;

  int current_step;

  NRCOptimization() {
    ITMAX=100;
    ZEPS = 1.0e-10;
    CGOLD = 0.3819660e0;
    GOLD = 1.618034e0;
    TOL = 2.0e-4;
    GLIMIT = 100.0;
    TINY = numeric_limits<T>::epsilon();
    LambdaMax = 0.02;
    current_step = 0;
  }

  virtual ~NRCOptimization() { }

  /** evaluate the value for y+dl*x 
   *
   * Lineminimization uses this function to find the minimum along the x direction
   */
  virtual Return_t Func(Return_t dl) = 0;

  Return_t Lambda;
  Return_t ZEPS, CGOLD, TOL, GLIMIT, TINY, GOLD;

  // Returns the number of real roots
  inline int CubicFormula (double a, double b, double c, double d,
			   double &x1, double &x2, double &x3)
  {
    double A = b/a;
    double B = c/a;
    double C = d/a;
    double Q = (A*A - 3.0*B)/9.0;
    double R = (2.0*A*A*A - 9.0*A*B + 27.0*C)/54.0;
    //cerr << "Q = " << Q << " R = " << R << "\n";
    if ((R*R) < (Q*Q*Q))
      {
	double theta = acos(R/sqrt(Q*Q*Q));
	double twosqrtQ = 2.0*sqrt(Q);
	double third = 1.0/3.0;
	double thirdA = third * A;
	x1 = -twosqrtQ*cos(third*theta) - thirdA;
	x2 = -twosqrtQ*cos(third*(theta + 2.0*M_PI)) - thirdA;
	x3 = -twosqrtQ*cos(third*(theta - 2.0*M_PI)) - thirdA;
	return 3;
      }
    else {
      double D = -Q*Q*Q + R*R;
      double u = cbrt(-R + sqrt(D));
      double v = cbrt(-R - sqrt(D));
      double y1 = u+v;
      x1 = y1 - A/3.0;
      return 1;
    }
  }

  inline Return_t QuarticMinimum (vector<Return_t> &coefs)
  {
    double a, b, c, d;
    a = 4.0*coefs[4];
    b = 3.0*coefs[3];
    c = 2.0*coefs[2];
    d = coefs[1];
    double x1, x2, x3;
    int numroots = CubicFormula (a, b, c, d, x1, x2, x3);
    if (numroots == 1)
      return x1;
    else {
      double v1 = coefs[0] + coefs[1]*x1 + coefs[2]*x1*x1 + coefs[3]*x1*x1*x1
	+ coefs[4]*x1*x1*x1*x1;
      double v2 = coefs[0] + coefs[1]*x2 + coefs[2]*x2*x2 + coefs[3]*x2*x2*x2
	+ coefs[4]*x2*x2*x2*x2;
      double v3 = coefs[0] + coefs[1]*x3 + coefs[2]*x3*x3 + coefs[3]*x3*x3*x3
	+ coefs[4]*x3*x3*x3*x3;
      if (v1 < v2 && v1 < v3)
	return x1;
      if (v2 < v1 && v2 < v3)
	return x2;
      if (v3 < v1 && v3 < v2)
	return x3;
      return x1;
    }
  }
	  


  bool lineoptimization() {
    vector<Return_t> x(5), y(5), coefs(5), deriv(4);
    qmcplusplus::Matrix<Return_t> S(5,5);
    x[0]=-0.02; x[1]=-0.01; x[2]=0.0; x[3]=0.01; x[4]=0.02;
    Return_t start_cost, cost;
    for (int i=0; i<5; i++) {
      y[i] = Func(x[i]);
      for (int j=0; j<5; j++)
	S(i,j) = std::pow(x[i],j);
    }
    start_cost = y[2];

    qmcplusplus::invert_matrix(S, false);
    qmcplusplus::MatrixOperators::product(S, &(y[0]), &(coefs[0]));

    Lambda = QuarticMinimum (coefs);
    if (fabs(Lambda) > 2.0 || isnan(Lambda))
      return lineoptimization2();
    cost = Func(Lambda);
    if (isnan(cost) || cost > start_cost)
      return lineoptimization2();
    //fprintf (stderr, "Minimum found at %1.8f\n", Lambda);
    
    current_step++;
    return true;

    // HACK HACK HACK
//     if (Lambda < 0.0) {
      // char fname[50];
      // snprintf (fname, 50, "line_opt_%d.dat", current_step);
      // FILE *fout = fopen (fname, "w");
      // for (double lam=-0.01; lam<=0.01; lam+=0.0001) {
      //   double val = 0.0;
      //   for (int j=0; j<5; j++) 
      // 	val += coefs[j] * std::pow(lam, j);
      //   fprintf (fout, "%1.8f %1.12e %1.12e\n", lam, Func(lam), val);
      // }
      // fclose(fout);
//     }
    // END HACK HACK HACK



  }    

  bool lineoptimization2() {
    Return_t ax = 0;
    Return_t bx, fa, fx, fb;
    Return_t xx = LambdaMax;

    // HACK HACK HACK
//     char fname[50];
//     snprintf (fname, 50, "line_opt_%d.dat", current_step++);
//     FILE *fout = fopen (fname, "w");
//     for (double lam=-0.01; lam<=0.01; lam+=0.0001)
//       fprintf (fout, "%1.8f %1.12e\n", lam, Func(lam));
//     fclose(fout);
    // END HACK HACK HACK


    fprintf (stderr, "Before:  ax = %1.10f  bx=%1.10f  cx=%1.10f\n",
	     ax, xx, bx);
    mnbrakNRC(ax,xx,bx,fa,fx,fb);
    fprintf (stderr, "After:  ax = %1.10f  bx=%1.10f  cx=%1.10f\n",
	     ax, xx, bx);

    Lambda = 0.0e0;
    Return_t ep = brentNRC(ax,xx,bx,Lambda);
    fprintf (stderr, "Minimum found at lambda = %1.5e\n", Lambda);

    if(std::fabs(Lambda)<TINY) 
      return false;
    else 
      return true;
  }

  inline void shift(Return_t& a, Return_t& b, Return_t& c, Return_t d) {
    a = b; b= c; c = d;
  }

  T brentNRC(Return_t ax, Return_t bx, Return_t cx, Return_t & xmin);

  void mnbrakNRC(Return_t& ax,Return_t& bx,Return_t& cx,
		 Return_t& fa,Return_t& fb,Return_t& fc );

};

template<class T>
T NRCOptimization<T>::brentNRC(Return_t ax, Return_t bx,  Return_t cx, Return_t& xmin) {

  Return_t a,b,d,etemp,fu,fv,fw,fx,p,q,r,tol1,tol2,u,v,w,x,xm;
  Return_t e=0.0e0;

  a=((ax < cx) ? ax : cx);
  b=((ax > cx) ? ax : cx);

  x=w=v=bx;

  fw=fv=fx=Func(x);

  for(int iter=1;iter<=ITMAX;iter++) {
    xm=0.5*(a+b);
    tol2=2.0*(tol1=TOL*fabs(x)+ZEPS);
    if (fabs(x-xm) <= (tol2-0.5*(b-a))) {
      xmin=x;
      return fx;
    }
    if (fabs(e) > tol1) {
      r=(x-w)*(fx-fv);
      q=(x-v)*(fx-fw);
      p=(x-v)*q-(x-w)*r;
      q=2.0*(q-r);
      if (q > 0.0) p = -p;
      q=fabs(q);
      etemp=e;
      e=d;
      if (fabs(p) >= fabs(0.5*q*etemp) || p <= q*(a-x) || p >= q*(b-x))
        d=CGOLD*(e=(x >= xm ? a-x : b-x));
      else {
	d=p/q;
	u=x+d;
	//if (u-a < tol2 || b-u < tol2) d=sign(tol1,xm-x);
	if (u-a < tol2 || b-u < tol2) d=sign2<T>::apply(tol1,xm-x);
      }
    } else {
      d=CGOLD*(e=(x >= xm ? a-x : b-x));
    }
    //u=(fabs(d) >= tol1 ? x+d : x+sign(tol1,d));
    u=(fabs(d) >= tol1 ? x+d : x+sign2<T>::apply(tol1,d));
      fu = Func(u); // fu=(*f)(u);
      if (fu <= fx) {
        if (u >= x) a=x; else b=x;
   	shift(v,w,x,u);
	shift(fv,fw,fx,fu);
      } else {
	if (u < x) a=u; else b=u;
	if (fu <= fw || w == x) {
  	  v=w;
	  w=u;
	  fv=fw;
	  fw=fu;
	} else if (fu <= fv || v == x || v == w) {
  	  v=u;
 	  fv=fu;
	}
      }
  }

  xmin=x;
  return fx;
}

template<class T>
void 
NRCOptimization<T>::mnbrakNRC(Return_t& ax, Return_t& bx, Return_t& cx,
			    Return_t& fa, Return_t& fb, Return_t& fc) {

  Return_t ulim,u,r,q,fu,dum = 0.0e0;

  fa = Func(ax); // *fa=(*func)(*ax);
  fb = Func(bx); // *fb=(*func)(*bx);

  if (fb > fa) {
    shift(dum,ax,bx,dum);
    shift(dum,fb,fa,dum);
  }

  cx=bx+GOLD*(bx-ax);
  fc = Func(cx); // *fc=(*func)(*cx);
  while (fb > fc) {
    r=(bx-ax)*(fb-fc);
    q=(bx-cx)*(fb-fa);
    //u=(bx)-((bx-cx)*q-(bx-ax)*r)/(2.0*sign(max(fabs(q-r),TINY),q-r));
    u=(bx)-((bx-cx)*q-(bx-ax)*r)/(2.0*sign2<T>::apply(max(fabs(q-r),TINY),q-r));
    ulim=(bx)+GLIMIT*(cx-bx);
    if ((bx-u)*(u-cx) > 0.0) {
      fu = Func(u); // fu=(*func)(u);
      if (fu < fc) {
	ax=bx;
        bx=u;
	fa=fb;
	fb=fu;
	return;
      } else if (fu > fb) {
	cx=u;
	fc=fu;
	return;
      }
      u=cx+GOLD*(cx-bx);
      fu = Func(u);//fu=(*func)(u);
    } else if ((cx-u)*(u-ulim) > 0.0) {
      fu = Func(u);//fu=(*func)(u);
      if (fu < fc) {

        shift(bx,cx,u,cx + GOLD*(cx-bx));
        shift(fb,fc,fu,Func(u)) ;
      }
    } else if ((u-ulim)*(ulim-cx) >= 0.0) {
      u=ulim;
      fu = Func(u);//fu=(*func)(u);
    } else {
      u=cx+GOLD*(cx-bx);
      fu = Func(u);//fu=(*func)(u);
    }
    shift(ax,bx,cx,u);
    shift(fa,fb,fc,fu);
  }
}

#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
