#ifndef ATOMIC_ORBITAL_H
#define ATOMIC_ORBITAL_H

#include <vector>
#include <einspline/bspline.h>
#include <einspline/multi_bspline.h>
#include "QMCWaveFunctions/SPOSetBase.h"
#include "Lattice/CrystalLattice.h"
#include "Configuration.h"
#include "Utilities/NewTimer.h"

namespace qmcplusplus {

  ////////////////////////////////////////////////////////////////////
  // This is just a template trick to avoid template specialization //
  // in AtomicOrbital.                                              //
  ////////////////////////////////////////////////////////////////////
  template<typename StorageType>  struct AtomicOrbitalTraits{};
  template<> struct AtomicOrbitalTraits<double>
  {  typedef multi_UBspline_1d_d SplineType;  };
  template<> struct AtomicOrbitalTraits<complex<double> >
  {  typedef multi_UBspline_1d_z SplineType;  };


  inline void EinsplineMultiEval (multi_UBspline_1d_d *spline, 
				  double x, double *val)
  {    eval_multi_UBspline_1d_d (spline, x, val);                 }
  inline void EinsplineMultiEval (multi_UBspline_1d_z *spline, 
				  double x, complex<double> *val)
  {    eval_multi_UBspline_1d_z (spline, x, val);                 }
  inline void EinsplineMultiEval (multi_UBspline_1d_d *spline, double x, 
				  double *val, double *grad, double *lapl)
  {    eval_multi_UBspline_1d_d_vgl (spline, x, val, grad, lapl); }
  inline void EinsplineMultiEval (multi_UBspline_1d_z *spline, double x, 
				  complex<double> *val, complex<double> *grad,
				  complex<double> *lapl)
  {    eval_multi_UBspline_1d_z_vgl (spline, x, val, grad, lapl); }


  template<typename StorageType>
  class AtomicOrbital
  {
  private:
    typedef QMCTraits::PosType                    PosType;
    typedef CrystalLattice<RealType,OHMMS_DIM>    UnitCellType;
    typedef Vector<double>                        RealValueVector_t;
    typedef Vector<TinyVector<double,OHMMS_DIM> > RealGradVector_t;
    typedef Vector<complex<double> >              ComplexValueVector_t;
    typedef Vector<TinyVector<complex<double>,OHMMS_DIM> > ComplexGradVector_t;
    typedef typename AtomicOrbitalTraits<StorageType>::SplineType SplineType;

    // Store in order 
    // Index = l*(l+1) + m.  There are (lMax+1)^2 Ylm's
    vector<StorageType> YlmVec, dYlmVec, ulmVec, dulmVec, d2ulmVec;
    inline void CalcYlm(PosType rhat, 
			vector<complex<double> > &Ylm,
			vector<complex<double> > &dYlm);

    inline void CalcYlm(PosType rhat, 
			vector<double> &Ylm, vector<double> &dYlm);

    SplineType *RadialSpline;
    // The first index is n in r^n, the second is lm = l*(l+1)+m
    Array<StorageType,3> PolyCoefs;
    NewTimer YlmTimer, SplineTimer, SumTimer;
    RealType rmagLast;
    vector<PosType> TwistAngles;
  public:
    PosType Pos;
    RealType CutoffRadius, SplineRadius, PolyRadius;
    int SplinePoints;
    int PolyOrder;
    int lMax, Numlm, NumBands;
    UnitCellType Lattice;

    inline void set_pos  (PosType pos){ Pos = pos;   } 
    inline void set_lmax (int lmax)   { lMax = lmax; }
    inline void set_cutoff (RealType cutoff) 
    { CutoffRadius = cutoff; }
    inline void set_spline (RealType radius, int points)
    { SplineRadius = radius;  SplinePoints = points; }
    inline void set_polynomial (RealType radius, int order)
    { PolyRadius = radius; PolyOrder = order;        }
    inline void set_num_bands (int num_bands) 
    { NumBands = num_bands;                          }

    inline void registerTimers()
    {
      YlmTimer.reset();
      SplineTimer.reset();
      TimerManager.addTimer (&YlmTimer);
      TimerManager.addTimer (&SplineTimer);
      TimerManager.addTimer (&SumTimer);
    }

    void allocate()
    {
      Numlm = (lMax+1)*(lMax+1);
      YlmVec.resize(Numlm);  dYlmVec.resize(Numlm);
      ulmVec.resize  (Numlm*NumBands);  
      dulmVec.resize (Numlm*NumBands); 
      d2ulmVec.resize(Numlm*NumBands);
      PolyCoefs.resize(PolyOrder+1, NumBands, Numlm);
      BCtype_z bc;
      bc.lCode = NATURAL;  bc.rCode = NATURAL;
      Ugrid grid;
      grid.start = 0.0;  grid.end = SplineRadius;  grid.num = SplinePoints;
      // if (RadialSpline) destroy_Bspline (RadialSpline);
      RadialSpline = create_multi_UBspline_1d_z (grid, bc, Numlm*NumBands);
      TwistAngles.resize(NumBands);
    }

    void SetBand (int band, Array<complex<double>,2> &spline_data,
		  Array<complex<double>,2> &poly_coefs,
		  PosType twist);
    // {
    //   vector<complex<double> > one_spline(SplinePoints);
    //   for (int lm=0; lm<Numlm; lm++) {
    // 	int index = band*Numlm + lm;
    // 	for (int i=0; i<SplinePoints; i++)
    // 	  one_spline[i] = spline_data(i, lm);
    // 	set_multi_UBspline_1d_z (RadialSpline, index, &one_spline[0]);
    // 	for (int n=0; n<=PolyOrder; n++)
    // 	  PolyCoefs(n,band,lm) = poly_coefs (n,lm);
    //   }
    //   TwistAngles[band] = twist;
    // }
    
    inline bool evaluate (PosType r, ComplexValueVector_t &vals);
    inline bool evaluate (PosType r, ComplexValueVector_t &val,
			  ComplexGradVector_t &grad,
			  ComplexValueVector_t &lapl);
    inline bool evaluate (PosType r, RealValueVector_t &vals);
    inline bool evaluate (PosType r, RealValueVector_t &val,
			  RealGradVector_t &grad,
			  RealValueVector_t &lapl);

    
    AtomicOrbital() : RadialSpline(NULL), 
		      YlmTimer("AtomicOrbital::CalcYlm"),
		      SplineTimer("AtomicOrbital::1D spline"),
		      SumTimer("AtomicOrbital::Summation"),
		      rmagLast(1.0e50)
    {
      // Nothing else for now
    }
  };

  
  template<typename StorageType> inline bool
  AtomicOrbital<StorageType>::evaluate (PosType r, ComplexValueVector_t &vals)
  {
    PosType dr = r - Pos;
    PosType u = Lattice.toUnit(dr);
    PosType img;
    for (int i=0; i<OHMMS_DIM; i++) {
      img[i] = round(u[i]);
      u[i] -= img[i];
    }
    dr = Lattice.toCart(u);
    double r2 = dot(dr,dr);
    if (r2 > CutoffRadius*CutoffRadius)
      return false;
    
    double rmag = std::sqrt(r2);
    PosType rhat = (1.0/rmag)*dr;
    
    // Evaluate Ylm's
    CalcYlm (rhat, YlmVec, dYlmVec);

    if (std::fabs(rmag - rmagLast) > 1.0e-6) {
      // Evaluate radial functions
      if (rmag > PolyRadius)
	EinsplineMultiEval (RadialSpline, rmag, &(ulmVec[0])); 
      else {
	for (int index=0; index<ulmVec.size(); index++) 
	  ulmVec[index]  = complex<double>();
	double r2n = 1.0;
	for (int n=0; n <= PolyOrder; n++) {
	  int index = 0;
	  for (int i=0; i<vals.size(); i++)
	    for (int lm=0; lm<Numlm; lm++)
	      ulmVec[index++] += r2n*PolyCoefs(n,i,lm);
	  r2n *= rmag;
	}
      }
      rmagLast = rmag;
    }
    SumTimer.start();
    int index = 0;
    for (int i=0; i<vals.size(); i++) {
      vals[i] = complex<double>();
      for (int lm=0; lm < Numlm; lm++)
	vals[i] += ulmVec[index++] * YlmVec[lm];
      double phase = -2.0*M_PI*dot(TwistAngles[i],img);
      double s,c;
      sincos(phase,&s,&c);
      vals[i] *= complex<double>(c,s);
    }
    SumTimer.stop();
    return true;
  }


  template<typename StorageType> inline bool
  AtomicOrbital<StorageType>::evaluate (PosType r, RealValueVector_t &vals)
  {
    PosType dr = r - Pos;
    PosType u = Lattice.toUnit(dr);
    PosType img;
    for (int i=0; i<OHMMS_DIM; i++) {
      img[i] = round(u[i]);
      u[i] -= img[i];
    }
    dr = Lattice.toCart(u);
    double r2 = dot(dr,dr);
    if (r2 > CutoffRadius*CutoffRadius)
      return false;
    
    double rmag = std::sqrt(r2);
    PosType rhat = (1.0/rmag)*dr;
    
    // Evaluate Ylm's
    CalcYlm (rhat, YlmVec, dYlmVec);

    if (std::fabs(rmag - rmagLast) > 1.0e-6) {
      // Evaluate radial functions
      if (rmag > PolyRadius) {
	SplineTimer.start();
	EinsplineMultiEval (RadialSpline, rmag, &(ulmVec[0])); 
	SplineTimer.stop();
      }
      else {
	for (int index=0; index<ulmVec.size(); index++)
	  ulmVec[index] = complex<double>();
	double r2n = 1.0;
	for (int n=0; n <= PolyOrder; n++) {
	  int index = 0;
	  for (int i=0; i<vals.size(); i++)
	    for (int lm=0; lm<Numlm; lm++)
	      ulmVec[index++] += r2n*PolyCoefs(n,i,lm);
	  r2n *= rmag;
	}
      }
      rmagLast = rmag;
    }
    SumTimer.start();
    int index = 0;
    for (int i=0; i<vals.size(); i++) {
      vals[i] = 0.0;
      complex<double> tmp = 0.0;
      for (int lm=0; lm < Numlm; lm++, index++)
	tmp += ulmVec[index] * YlmVec[lm];
        //vals[i] += real(ulmVec[index++] * YlmVec[lm]);
        // vals[i] += (ulmVec[index].real() * YlmVec[lm].real() -
	// 	    ulmVec[index].imag() * YlmVec[lm].imag());
      double phase = -2.0*M_PI*dot(TwistAngles[i],img);
      double s,c;
      sincos(phase,&s,&c);
      vals[i] = real(complex<double>(c,s) * tmp);

    }
    SumTimer.stop();
    return true;
  }


  template<typename StorageType> inline bool
  AtomicOrbital<StorageType>::evaluate (PosType r, 
					RealValueVector_t &vals,
					RealGradVector_t  &grads,
					RealValueVector_t &lapl)
  {
    PosType dr = r - Pos;
    PosType u = Lattice.toUnit(dr);
    PosType img;
    for (int i=0; i<OHMMS_DIM; i++) {
      img[i] = round(u[i]);
      u[i] -= img[i];
    }
    dr = Lattice.toCart(u);
    double r2 = dot(dr,dr);
    if (r2 > CutoffRadius*CutoffRadius)
      return false;

    double rmag = std::sqrt(r2);
    double rInv = 1.0/rmag;
    PosType rhat = rInv*dr;
    double costheta = rhat[2];
    double sintheta = std::sqrt(1.0-costheta*costheta);
    double cosphi = rhat[0]/sintheta;
    double sinphi = rhat[1]/sintheta;
    PosType thetahat = PosType(costheta*cosphi,
		       costheta*sinphi,
		       -sintheta);
    PosType phihat   = PosType(-sinphi, cosphi, 0.0 );

    
    // Evaluate Ylm's
    CalcYlm (rhat, YlmVec, dYlmVec);

    // Evaluate radial functions
    if (rmag > PolyRadius) {
      SplineTimer.start();
      EinsplineMultiEval
	(RadialSpline, rmag, &(ulmVec[0]), &(dulmVec[0]), &(d2ulmVec[0])); 
      SplineTimer.stop();
    }
    else {
      for (int index=0; index<ulmVec.size(); index++) {
      	ulmVec[index]   = StorageType();
	dulmVec[index]  = StorageType();
	d2ulmVec[index] = StorageType();
      }

      double r2n = 1.0, r2nm1=0.0, r2nm2=0.0;
      double dn = 0.0;
      double dnm1 = -1.0;
      for (int n=0; n <= PolyOrder; n++) {
      	int index = 0;
      	for (int i=0; i<vals.size(); i++)
      	  for (int lm=0; lm<Numlm; lm++,index++) {
	    StorageType c = PolyCoefs(n,i,lm);
      	    ulmVec[index]   += r2n*c;
	    dulmVec[index]  += dn * r2nm1 * c;
	    d2ulmVec[index] += dn*dnm1 * r2nm2 * c;
	  }
	dn += 1.0;
	dnm1 += 1.0;
	r2nm2 = r2nm1;
	r2nm1 = r2n;
      	r2n *= rmag;
      }
    }

    SumTimer.start();
    int index = 0;
    for (int i=0; i<vals.size(); i++) {
      vals[i] = 0.0;
      for (int j=0; j<OHMMS_DIM; j++) grads[i][j] = 0.0;
      lapl[i] = 0.0;

      // Compute e^{-i k.L} phase factor
      double phase = -2.0*M_PI*dot(TwistAngles[i],img);
      double s,c;
      sincos(phase,&s,&c);
      complex<double> e2mikr(c,s);

      StorageType tmp_val, tmp_lapl,
	grad_rhat, grad_thetahat, grad_phihat;

      int lm=0;
      for (int l=0; l<= lMax; l++)
	for (int m=-l; m<=l; m++,lm++,index++) {
	  complex<double> im(0.0,(double)m);

	  tmp_val       += ulmVec[index] * YlmVec[lm];
	  grad_rhat     += dulmVec[index] * YlmVec[lm];
	  grad_thetahat += ulmVec[index] * rInv * dYlmVec[lm];
	  grad_phihat   += (ulmVec[index] * im *YlmVec[lm])/(rmag*sintheta);
	  
	  tmp_lapl += YlmVec[lm] * 
	    (-(double)(l*(l+1))*rInv*rInv * ulmVec[index]
	     + d2ulmVec[index] + 2.0*rInv *dulmVec[index]);
	}
      vals[i]  = real(e2mikr*tmp_val);
      lapl[i]  = real(e2mikr*tmp_lapl);
      grads[i] = (real(e2mikr*grad_rhat    ) * rhat     + 
		  real(e2mikr*grad_thetahat) * thetahat +
		  real(e2mikr*grad_phihat  ) * phihat);
    }
    SumTimer.stop();
    rmagLast = rmag;
    return true;
  }



  template<typename StorageType> inline bool
  AtomicOrbital<StorageType>::evaluate (PosType r, ComplexValueVector_t &vals,
					ComplexGradVector_t &grads,
					ComplexValueVector_t &lapl)
  {
    PosType dr = r - Pos;
    PosType u = Lattice.toUnit(dr);
    PosType img;
    for (int i=0; i<OHMMS_DIM; i++) {
      img[i] = round(u[i]);
      u[i] -= img[i];
    }
    dr = Lattice.toCart(u);
    double r2 = dot(dr,dr);
    if (r2 > CutoffRadius*CutoffRadius)
      return false;

    double rmag = std::sqrt(r2);
    double rInv = 1.0/rmag;
    PosType rhat = rInv*dr;
    double costheta = rhat[2];
    double sintheta = std::sqrt(1.0-costheta*costheta);
    double cosphi = rhat[0]/sintheta;
    double sinphi = rhat[1]/sintheta;
    PosType thetahat = PosType(costheta*cosphi,
		       costheta*sinphi,
		       -sintheta);
    PosType phihat   = PosType(-sinphi, cosphi, 0.0 );

    
    // Evaluate Ylm's
    CalcYlm (rhat, YlmVec, dYlmVec);

    // Evaluate radial functions
    if (rmag > PolyRadius) {
      SplineTimer.start();
      EinsplineMultiEval
	(RadialSpline, rmag, &(ulmVec[0]), &(dulmVec[0]), &(d2ulmVec[0])); 
      SplineTimer.stop();
    }
    else {
      for (int index=0; index<ulmVec.size(); index++) {
      	ulmVec[index]  = complex<double>();
	dulmVec[index]  = complex<double>();
	d2ulmVec[index] = complex<double>();
      }

      double r2n = 1.0, r2nm1=0.0, r2nm2=0.0;
      double dn = 0.0;
      double dnm1 = -1.0;
      for (int n=0; n <= PolyOrder; n++) {
      	int index = 0;
      	for (int i=0; i<vals.size(); i++)
      	  for (int lm=0; lm<Numlm; lm++,index++) {
	    complex<double> c = PolyCoefs(n,i,lm);
      	    ulmVec[index]   += r2n*c;
	    dulmVec[index]  += dn * r2nm1 * c;
	    d2ulmVec[index] += dn*dnm1 * r2nm2 * c;
	  }
	dn += 1.0;
	dnm1 += 1.0;
	r2nm2 = r2nm1;
	r2nm1 = r2n;
      	r2n *= rmag;
      }
    }

    SumTimer.start();
    int index = 0;
    for (int i=0; i<vals.size(); i++) {
      vals[i] = 0.0;
      for (int j=0; j<OHMMS_DIM; j++) grads[i][j] = 0.0;
      lapl[i] = 0.0;
      int lm=0;
      complex<double> grad_rhat, grad_thetahat, grad_phihat;

      // Compute e^{-i k.L} phase factor
      double phase = -2.0*M_PI*dot(TwistAngles[i],img);
      double s,c;
      sincos(phase,&s,&c);
      complex<double> e2mikr(c,s);

      for (int l=0; l<= lMax; l++)
	for (int m=-l; m<=l; m++,lm++,index++) {
	  complex<double> im(0.0,(double)m);

	  vals[i]  += ulmVec[index] * YlmVec[lm];
	  grad_rhat     += dulmVec[index] * YlmVec[lm];
	  grad_thetahat += ulmVec[index] * rInv * dYlmVec[lm];
	  grad_phihat   += (ulmVec[index] * im*YlmVec[lm])/(rmag*sintheta);
	  lapl[i] += YlmVec[lm] * 
	    (-(double)(l*(l+1))*rInv*rInv * ulmVec[index]
	     + d2ulmVec[index] + 2.0*rInv *dulmVec[index]);
	}
      for (int j=0; j<OHMMS_DIM; j++) {
	vals[i] *= e2mikr;
	lapl[i] *= e2mikr;
	grads[i][j] = e2mikr*(grad_rhat*rhat[j] + grad_thetahat*thetahat[j] 
			      + grad_phihat * phihat[j]);
      }
    }
    SumTimer.stop();
    rmagLast = rmag;
    return true;
  }





  // Fast implementation
  // See Geophys. J. Int. (1998) 135,pp.307-309
  template<typename StorageType> inline void
  AtomicOrbital<StorageType>::CalcYlm (PosType rhat,
				       vector<complex<double> > &Ylm,
				       vector<complex<double> > &dYlm)
  {
    YlmTimer.start();
    const double fourPiInv = 0.0795774715459477;
    
    double costheta = rhat[2];
    double sintheta = std::sqrt(1.0-costheta*costheta);
    double cottheta = costheta/sintheta;
    
    double cosphi, sinphi;
    cosphi=rhat[0]/sintheta;
    sinphi=rhat[1]/sintheta;
    
    complex<double> e2iphi(cosphi, sinphi);
    
    
    double lsign = 1.0;
    double dl = 0.0;
    double XlmVec[2*lMax+1], dXlmVec[2*lMax+1];
    for (int l=0; l<=lMax; l++) {
      XlmVec[2*l]  = lsign;  
      dXlmVec[2*l] = dl * cottheta * XlmVec[2*l];
      XlmVec[0]    = lsign*XlmVec[2*l];
      dXlmVec[0]   = lsign*dXlmVec[2*l];
      double dm = dl;
      double msign = lsign;
      for (int m=l; m>0; m--) {
	double tmp = std::sqrt((dl+dm)*(dl-dm+1.0));
	XlmVec[l+m-1]  = -(dXlmVec[l+m] + dm*cottheta*XlmVec[l+m])/ tmp;
	dXlmVec[l+m-1] = (dm-1.0)*cottheta*XlmVec[l+m-1] + XlmVec[l+m]*tmp;
	// Copy to negative m
	XlmVec[l-(m-1)]  = -msign* XlmVec[l+m-1];
	dXlmVec[l-(m-1)] = -msign*dXlmVec[l+m-1];
	msign *= -1.0;
	dm -= 1.0;
      }
      double sum = 0.0;
      for (int m=-l; m<=l; m++) 
	sum += XlmVec[l+m]*XlmVec[l+m];
      // Now, renormalize the Ylms for this l
      double norm = std::sqrt((2.0*dl+1.0)*fourPiInv / sum);
      for (int m=-l; m<=l; m++) {
	XlmVec[l+m]  *= norm;
	dXlmVec[l+m] *= norm;
      }
      
      // Multiply by azimuthal phase and store in Ylm
      complex<double> e2imphi (1.0, 0.0);
      for (int m=0; m<=l; m++) {
	Ylm[l*(l+1)+m]  =  XlmVec[l+m]*e2imphi;
	Ylm[l*(l+1)-m]  =  XlmVec[l-m]*conj(e2imphi);
	dYlm[l*(l+1)+m] = dXlmVec[l+m]*e2imphi;
	dYlm[l*(l+1)-m] = dXlmVec[l-m]*conj(e2imphi);
	e2imphi *= e2iphi;
      } 
      
      dl += 1.0;
      lsign *= -1.0;
    }
    YlmTimer.stop();
  }

  // Fast implementation
  // See Geophys. J. Int. (1998) 135,pp.307-309
  template<typename StorageType> inline void
  AtomicOrbital<StorageType>::CalcYlm (PosType rhat,
				       vector<double> &Ylm,
				       vector<double> &dYlm)
  {
    YlmTimer.start();
    const double fourPiInv = 0.0795774715459477;
    
    double costheta = rhat[2];
    double sintheta = std::sqrt(1.0-costheta*costheta);
    double cottheta = costheta/sintheta;
    
    double cosphi, sinphi;
    cosphi=rhat[0]/sintheta;
    sinphi=rhat[1]/sintheta;
    
    complex<double> e2iphi(cosphi, sinphi);
    
    
    double lsign = 1.0;
    double dl = 0.0;
    double XlmVec[2*lMax+1], dXlmVec[2*lMax+1];
    for (int l=0; l<=lMax; l++) {
      XlmVec[2*l]  = lsign;  
      dXlmVec[2*l] = dl * cottheta * XlmVec[2*l];
      XlmVec[0]    = lsign*XlmVec[2*l];
      dXlmVec[0]   = lsign*dXlmVec[2*l];
      double dm = dl;
      double msign = lsign;
      for (int m=l; m>0; m--) {
	double tmp = std::sqrt((dl+dm)*(dl-dm+1.0));
	XlmVec[l+m-1]  = -(dXlmVec[l+m] + dm*cottheta*XlmVec[l+m])/ tmp;
	dXlmVec[l+m-1] = (dm-1.0)*cottheta*XlmVec[l+m-1] + XlmVec[l+m]*tmp;
	// Copy to negative m
	XlmVec[l-(m-1)]  = -msign* XlmVec[l+m-1];
	dXlmVec[l-(m-1)] = -msign*dXlmVec[l+m-1];
	msign *= -1.0;
	dm -= 1.0;
      }
      double sum = 0.0;
      for (int m=-l; m<=l; m++) 
	sum += XlmVec[l+m]*XlmVec[l+m];
      // Now, renormalize the Ylms for this l
      double norm = std::sqrt((2.0*dl+1.0)*fourPiInv / sum);
      for (int m=-l; m<=l; m++) {
	XlmVec[l+m]  *= norm;
	dXlmVec[l+m] *= norm;
      }
      
      // Multiply by azimuthal phase and store in Ylm
      complex<double> e2imphi (1.0, 0.0);
      for (int m=0; m<=l; m++) {
	Ylm[l*(l+1)+m]  =  XlmVec[l+m]*e2imphi.real();
	Ylm[l*(l+1)-m]  =  XlmVec[l-m]*e2imphi.imag();
	dYlm[l*(l+1)+m] = dXlmVec[l+m]*e2imphi.real();
	dYlm[l*(l+1)-m] = dXlmVec[l-m]*e2imphi.imag();
	e2imphi *= e2iphi;
      } 
      
      dl += 1.0;
      lsign *= -1.0;
    }
    YlmTimer.stop();
  }




}
#endif
