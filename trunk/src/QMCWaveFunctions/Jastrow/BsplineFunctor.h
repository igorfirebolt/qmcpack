//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
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
#ifndef QMCPLUSPLUS_BSPLINE_FUNCTOR_H
#define QMCPLUSPLUS_BSPLINE_FUNCTOR_H
#include "Numerics/OptimizableFunctorBase.h"
#include "Utilities/ProgressReportEngine.h"
#include "OhmmsData/AttributeSet.h"
#include "Numerics/LinearFit.h"
#include <cstdio>

namespace qmcplusplus
  {

  template<class T>
  struct BsplineFunctor: public OptimizableFunctorBase
    {

      typedef real_type value_type;
      int NumParams;
      int Dummy;
      const TinyVector<real_type,16> A, dA, d2A, d3A;
      //static const real_type A[16], dA[16], d2A[16];
      real_type DeltaR, DeltaRInv;
      real_type CuspValue;
      real_type Y, dY, d2Y;
      std::vector<real_type> SplineCoefs;
      // Stores the derivatives w.r.t. SplineCoefs
      // of the u, du/dr, and d2u/dr2
      std::vector<TinyVector<real_type,3> > SplineDerivs;
      std::vector<real_type> Parameters;
      std::vector<std::string> ParameterNames;
      std::string elementType, pairType;
      int ResetCount;

      ///constructor
      BsplineFunctor(real_type cusp=0.0) :
          NumParams(0),
          A(-1.0/6.0,  3.0/6.0, -3.0/6.0, 1.0/6.0,
             3.0/6.0, -6.0/6.0,  0.0/6.0, 4.0/6.0,
            -3.0/6.0,  3.0/6.0,  3.0/6.0, 1.0/6.0,
             1.0/6.0,  0.0/6.0,  0.0/6.0, 0.0/6.0),
          dA(0.0, -0.5,  1.0, -0.5,
             0.0,  1.5, -2.0,  0.0,
             0.0, -1.5,  1.0,  0.5,
             0.0,  0.5,  0.0,  0.0),
          d2A(0.0, 0.0, -1.0,  1.0,
              0.0, 0.0,  3.0, -2.0,
              0.0, 0.0, -3.0,  1.0,
              0.0, 0.0,  1.0,  0.0),
          d3A(0.0, 0.0,  0.0, -1.0,
              0.0, 0.0,  0.0,  3.0,
              0.0, 0.0,  0.0, -3.0,
              0.0, 0.0,  0.0,  1.0),
          CuspValue(cusp), ResetCount(0)
      {
        cutoff_radius = 0.0;
      }

      OptimizableFunctorBase* makeClone() const
        {
          return new BsplineFunctor(*this);
        }

      void resize(int n)
      {
        NumParams = n;
        int numCoefs = NumParams + 4;
        int numKnots = numCoefs - 2;
        DeltaR = cutoff_radius / (real_type)(numKnots - 1);
        DeltaRInv = 1.0/DeltaR;

        Parameters.resize(n);
        SplineCoefs.resize(numCoefs);
        SplineDerivs.resize(numCoefs);
      }

      void reset()
      {
        int numCoefs = NumParams + 4;
        int numKnots = numCoefs - 2;
        DeltaR = cutoff_radius / (real_type)(numKnots - 1);
        DeltaRInv = 1.0/DeltaR;

        for (int i=0; i<SplineCoefs.size(); i++)
          SplineCoefs[i] = 0.0;

        // Ensure that cusp conditions is satsified at the origin
        SplineCoefs[1] = Parameters[0];
        SplineCoefs[2] = Parameters[1];
        SplineCoefs[0] = Parameters[1] - 2.0*DeltaR * CuspValue;
        for (int i=2; i<Parameters.size(); i++)
          SplineCoefs[i+1] = Parameters[i];
//#if !defined(HAVE_MPI)
//      string fname = (elementType != "") ? elementType : pairType;
//      fname = fname + ".dat";
//      // fprintf (stderr, "Writing %s file.\n", fname.c_str());
//      FILE *fout = fopen (fname.c_str(), "w");
//      for (real_type r=1.0e-5; r<cutoff_radius; r+=0.01) {
//        real_type eps = 1.0e-6;
//        real_type du, d2u, du_FD, d2u_FD;
//        real_type u = evaluate (r, du, d2u);
//        real_type uplus  = evaluate(r+eps);
//        real_type uminus = evaluate(r-eps);
//        du_FD  = (uplus-uminus)/(2.0*eps);
//        d2u_FD = (uplus+uminus-2.0*u)/(eps*eps);
//        fprintf (fout, "%1.10e %1.10e %1.10e %1.10e %1.10e %1.10e\n",
//            r, evaluate(r), du, du_FD, d2u, d2u_FD);
//      }
//      fclose (fout);
//      //cerr << "SplineCoefs = ";
//      //for (int i=0; i<SplineCoefs.size(); i++)
//      //  cerr << SplineCoefs[i] << " ";
//      //cerr << endl;
//#endif
      }

      inline real_type evaluate(real_type r)
      {
        if (r >= cutoff_radius)
          return 0.0;
        r *= DeltaRInv;

        real_type ipart, t;
        t = modf(r, &ipart);
        int i = (int) ipart;

        real_type tp[4];
        tp[0] = t*t*t;
        tp[1] = t*t;
        tp[2] = t;
        tp[3] = 1.0;

        return
          (SplineCoefs[i+0]*(A[ 0]*tp[0] + A[ 1]*tp[1] + A[ 2]*tp[2] + A[ 3]*tp[3])+
           SplineCoefs[i+1]*(A[ 4]*tp[0] + A[ 5]*tp[1] + A[ 6]*tp[2] + A[ 7]*tp[3])+
           SplineCoefs[i+2]*(A[ 8]*tp[0] + A[ 9]*tp[1] + A[10]*tp[2] + A[11]*tp[3])+
           SplineCoefs[i+3]*(A[12]*tp[0] + A[13]*tp[1] + A[14]*tp[2] + A[15]*tp[3]));

      }
      inline real_type evaluate(real_type r, real_type rinv)
      {
        return Y=evaluate(r,dY,d2Y);
      }

      inline void evaluateAll(real_type r, real_type rinv)
      {
        Y=evaluate(r,dY,d2Y);
      }

      inline real_type
      evaluate(real_type r, real_type& dudr, real_type& d2udr2)
      {
        if (r >= cutoff_radius)
          {
            dudr = d2udr2 = 0.0;
            return 0.0;
          }
//       real_type eps = 1.0e-5;
//       real_type dudr_FD = (evaluate(r+eps)-evaluate(r-eps))/(2.0*eps);
//       real_type d2udr2_FD = (evaluate(r+eps)+evaluate(r-eps)-2.0*evaluate(r))/(eps*eps);

        r *= DeltaRInv;
        real_type ipart, t;
        t = modf(r, &ipart);
        int i = (int) ipart;

        real_type tp[4];
        tp[0] = t*t*t;
        tp[1] = t*t;
        tp[2] = t;
        tp[3] = 1.0;

        d2udr2 = DeltaRInv * DeltaRInv *
                 (SplineCoefs[i+0]*(d2A[ 0]*tp[0] + d2A[ 1]*tp[1] + d2A[ 2]*tp[2] + d2A[ 3]*tp[3])+
                  SplineCoefs[i+1]*(d2A[ 4]*tp[0] + d2A[ 5]*tp[1] + d2A[ 6]*tp[2] + d2A[ 7]*tp[3])+
                  SplineCoefs[i+2]*(d2A[ 8]*tp[0] + d2A[ 9]*tp[1] + d2A[10]*tp[2] + d2A[11]*tp[3])+
                  SplineCoefs[i+3]*(d2A[12]*tp[0] + d2A[13]*tp[1] + d2A[14]*tp[2] + d2A[15]*tp[3]));
        dudr = DeltaRInv *
               (SplineCoefs[i+0]*(dA[ 0]*tp[0] + dA[ 1]*tp[1] + dA[ 2]*tp[2] + dA[ 3]*tp[3])+
                SplineCoefs[i+1]*(dA[ 4]*tp[0] + dA[ 5]*tp[1] + dA[ 6]*tp[2] + dA[ 7]*tp[3])+
                SplineCoefs[i+2]*(dA[ 8]*tp[0] + dA[ 9]*tp[1] + dA[10]*tp[2] + dA[11]*tp[3])+
                SplineCoefs[i+3]*(dA[12]*tp[0] + dA[13]*tp[1] + dA[14]*tp[2] + dA[15]*tp[3]));

//       if (std::fabs(dudr_FD-dudr) > 1.0e-8)
//  cerr << "Error in BsplineFunction:  dudr = " << dudr
//       << "  dudr_FD = " << dudr_FD << endl;

//       if (std::fabs(d2udr2_FD-d2udr2) > 1.0e-4)
//  cerr << "Error in BsplineFunction:  r = " << r << "  d2udr2 = " << dudr
//       << "  d2udr2_FD = " << d2udr2_FD << "  rcut = " << cutoff_radius << endl;
        return
          (SplineCoefs[i+0]*(A[ 0]*tp[0] + A[ 1]*tp[1] + A[ 2]*tp[2] + A[ 3]*tp[3])+
           SplineCoefs[i+1]*(A[ 4]*tp[0] + A[ 5]*tp[1] + A[ 6]*tp[2] + A[ 7]*tp[3])+
           SplineCoefs[i+2]*(A[ 8]*tp[0] + A[ 9]*tp[1] + A[10]*tp[2] + A[11]*tp[3])+
           SplineCoefs[i+3]*(A[12]*tp[0] + A[13]*tp[1] + A[14]*tp[2] + A[15]*tp[3]));

      }


      inline real_type
      evaluate(real_type r, real_type& dudr, real_type& d2udr2, real_type &d3udr3)
      {
        if (r >= cutoff_radius)
          {
            dudr = d2udr2 = d3udr3 = 0.0;
            return 0.0;
          }
        // real_type eps = 1.0e-5;
//       real_type dudr_FD = (evaluate(r+eps)-evaluate(r-eps))/(2.0*eps);
//       real_type d2udr2_FD = (evaluate(r+eps)+evaluate(r-eps)-2.0*evaluate(r))/(eps*eps);
        // real_type d3udr3_FD = (-1.0*evaluate(r+1.0*eps)
        //         +2.0*evaluate(r+0.5*eps)
        //         -2.0*evaluate(r-0.5*eps)
        //         +1.0*evaluate(r-1.0*eps))/(eps*eps*eps);

        r *= DeltaRInv;
        real_type ipart, t;
        t = modf(r, &ipart);
        int i = (int) ipart;

        real_type tp[4];
        tp[0] = t*t*t;
        tp[1] = t*t;
        tp[2] = t;
        tp[3] = 1.0;

        d3udr3 = DeltaRInv * DeltaRInv * DeltaRInv *
                 (SplineCoefs[i+0]*(d3A[ 0]*tp[0] + d3A[ 1]*tp[1] + d3A[ 2]*tp[2] + d3A[ 3]*tp[3])+
                  SplineCoefs[i+1]*(d3A[ 4]*tp[0] + d3A[ 5]*tp[1] + d3A[ 6]*tp[2] + d3A[ 7]*tp[3])+
                  SplineCoefs[i+2]*(d3A[ 8]*tp[0] + d3A[ 9]*tp[1] + d3A[10]*tp[2] + d3A[11]*tp[3])+
                  SplineCoefs[i+3]*(d3A[12]*tp[0] + d3A[13]*tp[1] + d3A[14]*tp[2] + d3A[15]*tp[3]));
        d2udr2 = DeltaRInv * DeltaRInv *
                 (SplineCoefs[i+0]*(d2A[ 0]*tp[0] + d2A[ 1]*tp[1] + d2A[ 2]*tp[2] + d2A[ 3]*tp[3])+
                  SplineCoefs[i+1]*(d2A[ 4]*tp[0] + d2A[ 5]*tp[1] + d2A[ 6]*tp[2] + d2A[ 7]*tp[3])+
                  SplineCoefs[i+2]*(d2A[ 8]*tp[0] + d2A[ 9]*tp[1] + d2A[10]*tp[2] + d2A[11]*tp[3])+
                  SplineCoefs[i+3]*(d2A[12]*tp[0] + d2A[13]*tp[1] + d2A[14]*tp[2] + d2A[15]*tp[3]));
        dudr = DeltaRInv *
               (SplineCoefs[i+0]*(dA[ 0]*tp[0] + dA[ 1]*tp[1] + dA[ 2]*tp[2] + dA[ 3]*tp[3])+
                SplineCoefs[i+1]*(dA[ 4]*tp[0] + dA[ 5]*tp[1] + dA[ 6]*tp[2] + dA[ 7]*tp[3])+
                SplineCoefs[i+2]*(dA[ 8]*tp[0] + dA[ 9]*tp[1] + dA[10]*tp[2] + dA[11]*tp[3])+
                SplineCoefs[i+3]*(dA[12]*tp[0] + dA[13]*tp[1] + dA[14]*tp[2] + dA[15]*tp[3]));

//       if (std::fabs(dudr_FD-dudr) > 1.0e-8)
//  cerr << "Error in BsplineFunction:  dudr = " << dudr
//       << "  dudr_FD = " << dudr_FD << endl;

//       if (std::fabs(d2udr2_FD-d2udr2) > 1.0e-4)
//  cerr << "Error in BsplineFunction:  r = " << r << "  d2udr2 = " << dudr
//       << "  d2udr2_FD = " << d2udr2_FD << "  rcut = " << cutoff_radius << endl;

        // if (std::fabs(d3udr3_FD-d3udr3) > 1.0e-4)
        //  cerr << "Error in BsplineFunction:  r = " << r << "  d3udr3 = " << dudr
        //       << "  d3udr3_FD = " << d3udr3_FD << "  rcut = " << cutoff_radius << endl;

        return
          (SplineCoefs[i+0]*(A[ 0]*tp[0] + A[ 1]*tp[1] + A[ 2]*tp[2] + A[ 3]*tp[3])+
           SplineCoefs[i+1]*(A[ 4]*tp[0] + A[ 5]*tp[1] + A[ 6]*tp[2] + A[ 7]*tp[3])+
           SplineCoefs[i+2]*(A[ 8]*tp[0] + A[ 9]*tp[1] + A[10]*tp[2] + A[11]*tp[3])+
           SplineCoefs[i+3]*(A[12]*tp[0] + A[13]*tp[1] + A[14]*tp[2] + A[15]*tp[3]));

      }



      inline bool
      evaluateDerivatives(real_type r, vector<TinyVector<real_type,3> >& derivs)
      {
        if (r >= cutoff_radius)
          return false;

        r *= DeltaRInv;
        real_type ipart, t;
        t = modf(r, &ipart);
        int i = (int) ipart;

        real_type tp[4];
        tp[0] = t*t*t;
        tp[1] = t*t;
        tp[2] = t;
        tp[3] = 1.0;

        SplineDerivs[0] = TinyVector<real_type,3>(0.0);

        // d/dp_i u(r)
        SplineDerivs[i+0][0] = A[ 0]*tp[0] + A[ 1]*tp[1] + A[ 2]*tp[2] + A[ 3]*tp[3];
        SplineDerivs[i+1][0] = A[ 4]*tp[0] + A[ 5]*tp[1] + A[ 6]*tp[2] + A[ 7]*tp[3];
        SplineDerivs[i+2][0] = A[ 8]*tp[0] + A[ 9]*tp[1] + A[10]*tp[2] + A[11]*tp[3];
        SplineDerivs[i+3][0] = A[12]*tp[0] + A[13]*tp[1] + A[14]*tp[2] + A[15]*tp[3];

        // d/dp_i du/dr
        SplineDerivs[i+0][1] = DeltaRInv * (dA[ 1]*tp[1] + dA[ 2]*tp[2] + dA[ 3]*tp[3]);
        SplineDerivs[i+1][1] = DeltaRInv * (dA[ 5]*tp[1] + dA[ 6]*tp[2] + dA[ 7]*tp[3]);
        SplineDerivs[i+2][1] = DeltaRInv * (dA[ 9]*tp[1] + dA[10]*tp[2] + dA[11]*tp[3]);
        SplineDerivs[i+3][1] = DeltaRInv * (dA[13]*tp[1] + dA[14]*tp[2] + dA[15]*tp[3]);

        // d/dp_i d2u/dr2
        SplineDerivs[i+0][2] = DeltaRInv * DeltaRInv * (d2A[ 2]*tp[2] + d2A[ 3]*tp[3]);
        SplineDerivs[i+1][2] = DeltaRInv * DeltaRInv * (d2A[ 6]*tp[2] + d2A[ 7]*tp[3]);
        SplineDerivs[i+2][2] = DeltaRInv * DeltaRInv * (d2A[10]*tp[2] + d2A[11]*tp[3]);
        SplineDerivs[i+3][2] = DeltaRInv * DeltaRInv * (d2A[14]*tp[2] + d2A[15]*tp[3]);

        int imin=std::max(i,1);
        int imax=std::min(i+4,NumParams+1);
        for (int n=imin; n<imax; ++n) derivs[n-1] = SplineDerivs[n];
        derivs[1]+=SplineDerivs[0];

        return true;
      }

      inline real_type f(real_type r)
      {
        if (r>cutoff_radius) return 0.0;
        return evaluate(r);
      }
      inline real_type df(real_type r)
      {
        if (r>cutoff_radius) return 0.0;
        real_type du, d2u;
        evaluate(r, du, d2u);
        return du;
      }

      bool put(xmlNodePtr cur)
      {
        ReportEngine PRE("BsplineFunctor","put(xmlNodePtr)");
        //CuspValue = -1.0e10;
        NumParams = 0;
        //cutoff_radius = 0.0;
        OhmmsAttributeSet rAttrib;
	real_type radius = -1.0;
        rAttrib.add(NumParams,   "size");
        rAttrib.add(radius,      "rcut");
        rAttrib.put(cur);

        if (radius < 0.0) 
          app_log() << "  Jastrow cutoff unspecified.  Setting to Wigner-Seitz radius = "
            << cutoff_radius << ".\n";
        else
          cutoff_radius = radius;
	  
        if (NumParams == 0)
        {
          PRE.error("You must specify a positive number of parameters for the Bspline jastrow function.",true);
        }
        app_log() << " size = " << NumParams << " parameters " << endl;
        app_log() << " cusp = " << CuspValue << endl;
        app_log() << " rcut = " << cutoff_radius << endl;

        resize(NumParams);

        // Now read coefficents
        xmlNodePtr xmlCoefs = cur->xmlChildrenNode;
        while (xmlCoefs != NULL)
          {
            string cname((const char*)xmlCoefs->name);
            if (cname == "coefficients")
              {
                string type("0"), id("0");
                OhmmsAttributeSet cAttrib;
                cAttrib.add(id, "id");
                cAttrib.add(type, "type");
                cAttrib.put(xmlCoefs);

                if (type != "Array")
                  {
                    PRE.error("Unknown correlation type " + type + " in BsplineFunctor." + "Resetting to \"Array\"");
                    xmlNewProp(xmlCoefs, (const xmlChar*) "type", (const xmlChar*) "Array");
                  }

                vector<real_type> params;
                putContent(params, xmlCoefs);
                if (params.size() == NumParams)
                  Parameters = params;
                else
                  {
                    app_log() << "Changing number of Bspline parameters from "
                    << params.size() << " to " << NumParams << ".  Performing fit:\n";
                    // Fit function to new number of parameters
                    const int numPoints = 500;
                    BsplineFunctor<T> tmp_func(CuspValue);
                    tmp_func.cutoff_radius = cutoff_radius;
                    tmp_func.resize(params.size());
                    tmp_func.Parameters = params;
                    tmp_func.reset();
                    vector<real_type> y(numPoints);
                    Matrix<real_type> basis(numPoints,NumParams);
                    vector<TinyVector<real_type,3> > derivs(NumParams);
                    for (int i=0; i<numPoints; i++)
                      {
                        real_type r = (real_type)i / (real_type)numPoints * cutoff_radius;
                        y[i] = tmp_func.evaluate(r);
                        evaluateDerivatives(r, derivs);
                        for (int j=0; j<NumParams; j++)
                          basis(i,j) = derivs[j][0];
                      }
                    resize(NumParams);
                    LinearFit(y, basis, Parameters);
                    app_log() << "New parameters are:\n";
                    for (int i=0; i < Parameters.size(); i++)
                      app_log() << "   " << Parameters[i] << endl;
                  }

                // Setup parameter names
                for (int i=0; i< NumParams; i++)
                  {
                    std::stringstream sstr;
                    sstr << id << "_" << i;
                    myVars.insert(sstr.str(),Parameters[i],true,optimize::LOGLINEAR_P);
                  }

                app_log() << "Parameter     Name      Value\n";
                myVars.print(app_log());

                //for (int i=0; i<ParameterNames.size(); i++)
                //  app_log() << "    " << i << "         " << ParameterNames[i]
                //    << "       " << Parameters[i] << endl;
              }
            xmlCoefs = xmlCoefs->next;
          }

        reset();
        return true;
      }

      void checkInVariables(opt_variables_type& active)
      {
        active.insertFrom(myVars);
      }

      void checkOutVariables(const opt_variables_type& active)
      {
        myVars.getIndex(active);
      }

      void resetParameters(const opt_variables_type& active)
      {
        for (int i=0; i<Parameters.size(); ++i)
          {
            int loc=myVars.where(i);
            if (loc>=0) Parameters[i]=myVars[i]=active[loc];
          }
        if (ResetCount++ == 100)
          {
            ResetCount = 0;
            print();
          }
        reset();
      }


      void print()
      {
        string fname = (elementType != "") ? elementType : pairType;
        fname = fname + ".dat";
        //cerr << "Writing " << fname << " file.\n";
        FILE *fout = fopen(fname.c_str(), "w");
        for (double r=0.0; r<cutoff_radius; r+=0.001)
          fprintf(fout, "%8.3f %16.10f\n", r, evaluate(r));
        fclose(fout);
      }


      void print(std::ostream& os)
      {
        int n=100;
        T d=cutoff_radius/100.,r=0;
        T u,du,d2du;
        for (int i=0; i<n; ++i)
          {
            u=evaluate(r,du,d2du);
            os << setw(22) << r << setw(22) << u << setw(22) << du
            << setw(22) << d2du << std::endl;
            r+=d;
          }
      }
    };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1691 $   $Date: 2007-02-01 15:51:50 -0600 (Thu, 01 Feb 2007) $
 * $Id: BsplineConstraints.h 1691 2007-02-01 21:51:50Z jnkim $
 ***************************************************************************/
