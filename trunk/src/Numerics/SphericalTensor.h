#ifndef OHMMS_QMC__SPHERICAL_CARTESIAN_TENSOR_H
#define OHMMS_QMC__SPHERICAL_CARTESIAN_TENSOR_H

/** SphericalTensor
 * @author Jeongnim Kim, John Shumway and J. Vincent
 * @brief evaluates the Real Spherical Harmonics 
 \f[ r^l (S_l^m(x,y,z). \f]
 *
 Here we use the default convention (for addsign=false):
 \f[ r^l S_l^m = \sqrt{2}\Re(r^l Y_l^{|m|}), m > 0 \f] 
 *
 \f[ r^l S_l^m = r^l Y_l^0, m = 0 \f] 
 *
 \f[ r^l S_l^m = \sqrt{2}\Im(r^l Y_l^{|m|}), m < 0 \f] 
 
 For (addsign=true):
 \f[ r^l S_l^m = (-1)^m\sqrt{2}\Re(r^l Y_l^{|m|}), m > 0 \f] 
 *
 \f[ r^l S_l^m =  r^l Y_l^0, m = 0 \f] 
 *
 \f[ r^l  S_l^m = (-1)^m\sqrt{2}\Im(r^l Y_l^{|m|}), m < 0 \f] 
 
 The template parameter T is the value_type, e.g. double, and the 
 template parameter Point_t is a vector type which must have the
 operator[] defined.
*/
template<class T, class Point_t>
class SphericalTensor {
public : 

  typedef T value_type;
  typedef Point_t pos_type;
  typedef SphericalTensor<T,Point_t> This_t;

  ///constructor
  explicit SphericalTensor(const int lmax, bool addsign=false);

  ///makes a table of \f$ r^l S_l^m \f$ and their gradients up to lmax.
  void evaluate(const Point_t& p);     

  ///makes a table of \f$ r^l S_l^m \f$ and their gradients up to lmax.
  void evaluateTest(const Point_t& p);     

  ///returns the index \f$ l(l+1)+m \f$
  inline int index(int l, int m) const {return (l*(l+1))+m;}

  ///returns the value of \f$ r^l S_l^m \f$ given \f$ (l,m) \f$
  inline value_type getYlm(int l, int m) const
   {return Ylm[index(l,m)];}

  ///returns the gradient of \f$ r^l S_l^m \f$ given \f$ (l,m) \f$
  inline Point_t getGradYlm(int l, int m) const
  {return gradYlm[index(l,m)];}

  ///returns the value of \f$ r^l S_l^m \f$ given \f$ (l,m) \f$
  inline value_type getYlm(int lm) const {return Ylm[lm];}

  ///returns the gradient of \f$ r^l S_l^m \f$ given \f$ (l,m) \f$
  inline Point_t getGradYlm(int lm) const {return gradYlm[lm];}

  inline int size() const { return Ylm.size();}

  inline int lmax() const { return Lmax;}

  int Lmax;
  std::vector<value_type> Ylm;

  /// NormFactor[index(l,m)]  \f$(-1)^m\sqrt(2)\f$
  std::vector<value_type> NormFactor;

  ///\f$1/\sqrt{(l+m)*(l+1-m)}\f$, not implemented yet
  std::vector<value_type> FactorLM;
  std::vector<value_type> FactorL;
  std::vector<value_type> Factor2L;

  std::vector<Point_t> gradYlm;

};

template<class T, class Point_t>
SphericalTensor<T, Point_t>::SphericalTensor(const int lmax, bool addsign) : Lmax(lmax){ 
  int ntot = (lmax+1)*(lmax+1);
  Ylm.resize(ntot);
  gradYlm.resize(ntot);
  gradYlm[0] = 0.0;

  NormFactor.resize(ntot,1);
  const value_type sqrt2 = sqrt(2.0);
  if(addsign) {
    for (int l=0; l<=Lmax; l++) {
      NormFactor[index(l,0)]=1.0;
      for (int m=1; m<=l; m++) {
        NormFactor[index(l,m)]=pow(-1.0e0,m)*sqrt2;
        NormFactor[index(l,-m)]=pow(-1.0e0,-m)*sqrt2;
     }
    }
  } else {
    for (int l=0; l<=Lmax; l++) {
      for (int m=1; m<=l; m++) {
       NormFactor[index(l,m)]=sqrt2;
       NormFactor[index(l,-m)]=sqrt2;
     }
    }
  }

  FactorL.resize(lmax+1);
  const value_type omega = 1.0/sqrt(16.0*atan(1.0));
  for(int l=1; l<=lmax; l++) FactorL[l] = sqrt(static_cast<T>(2*l+1))*omega;

  Factor2L.resize(lmax+1);
  for(int l=1; l<=lmax; l++) Factor2L[l] = static_cast<T>(2*l+1)/static_cast<T>(2*l-1);

  FactorLM.resize(ntot);
  for(int l=1; l<=lmax; l++) 
    for(int m=1; m<=l; m++) {
      T fac2 = 1.0/sqrt(static_cast<T>((l+m)*(l+1-m)));
      FactorLM[index(l,m)]=fac2;
      FactorLM[index(l,-m)]=fac2;
    }
}

template<class T, class Point_t>
void SphericalTensor<T,Point_t>::evaluate(const Point_t& p) {
  value_type x=p[0], y=p[1], z=p[2];
  const value_type pi = 4.0*atan(1.0);
  const value_type pi4 = 4.0*pi;
  const value_type omega = 1.0/sqrt(pi4);
  const value_type sqrt2 = sqrt(2.0);

  /*  Calculate r, cos(theta), sin(theta), cos(phi), sin(phi) from input 
      coordinates. Check here the coordinate singularity at cos(theta) = +-1. 
      This also takes care of r=0 case. */

  value_type cphi,sphi,ctheta;
  value_type r2xy=x*x+y*y;
  value_type r=sqrt(r2xy+z*z);  
  //@todo use tolerance
  if (r2xy<1e-16) {
     cphi = 0.0;
     sphi = 1.0;
     ctheta = (z<0)?-1.0:1.0;
  } else {
     ctheta = z/r;
     value_type rxyi = 1.0/sqrt(r2xy);
     cphi = x*rxyi;
     sphi = y*rxyi;
  }

  value_type stheta = sqrt(1.0-ctheta*ctheta);

  /* Now to calculate the associated legendre functions P_lm from the
     recursion relation from l=0 to lmax. Conventions of J.D. Jackson, 
     Classical Electrodynamics are used. */
  
  Ylm[0] = 1.0;

  // calculate P_ll and P_l,l-1

  value_type fac = 1.0; 
  int j = -1;
  for (int l=1; l<=Lmax; l++) {
    j += 2;
    fac *= -j*stheta;
    int ll=index(l,l);
    int l1=index(l,l-1);
    int l2=index(l-1,l-1);
    Ylm[ll] = fac;
    Ylm[l1] = j*ctheta*Ylm[l2];
  }

  // Use recurence to get other plm's //
  
  for (int m=0; m<Lmax-1; m++) {
    int j = 2*m+1;
    for (int l=m+2; l<=Lmax; l++) {
      j += 2;
      int lm=index(l,m);
      int l1=index(l-1,m);
      int l2=index(l-2,m);
      Ylm[lm] = (ctheta*j*Ylm[l1]-(l+m-1)*Ylm[l2])/(l-m);
    }
  }
  
  // Now to calculate r^l Y_lm. //
  
  value_type sphim,cphim,fac2,temp;
  Ylm[0] = omega; //1.0/sqrt(pi4);          
  value_type rpow = 1.0;
  for (int l=1; l<=Lmax; l++) {
    rpow *= r;
    //fac = rpow*sqrt(static_cast<T>(2*l+1))*omega;//rpow*sqrt((2*l+1)/pi4);  
    //FactorL[l] = sqrt(2*l+1)/sqrt(4*pi)
    fac = rpow*FactorL[l];
    int l0=index(l,0);
    Ylm[l0] *= fac;
    cphim = 1.0;
    sphim = 0.0;
    for (int m=1; m<=l; m++) {
      //fac2 = (l+m)*(l+1-m);
      //fac = fac/sqrt(fac2);
      temp = cphim*cphi-sphim*sphi;
      sphim = sphim*cphi+cphim*sphi;
      cphim = temp;
      int lm = index(l,m);    
      //fac2,fac use precalculated FactorLM
      fac *= FactorLM[lm];
      temp = fac*Ylm[lm];
      Ylm[lm] = temp*cphim;
      lm = index(l,-m);
      Ylm[lm] = temp*sphim;
    }
  }

  // Calculating Gradient now//

  for (int l=1; l<=Lmax; l++) {
    //value_type fac = ((value_type) (2*l+1))/(2*l-1);
    value_type fac = Factor2L[l];
    for (int m=-l; m<=l; m++) {
      int lm = index(l-1,0);  
      value_type gx,gy,gz,dpr,dpi,dmr,dmi;
      int ma = abs(m);
      value_type cp = sqrt(fac*(l-ma-1)*(l-ma));
      value_type cm = sqrt(fac*(l+ma-1)*(l+ma));
      value_type c0 = sqrt(fac*(l-ma)*(l+ma));
      gz = (l > ma) ? c0*Ylm[lm+m]:0.0;
      if (l > ma+1) {
        dpr = cp*Ylm[lm+ma+1];
        dpi = cp*Ylm[lm-ma-1];
      } else {
        dpr = 0.0;
        dpi = 0.0;
      }
      if (l > 1) {
        switch (ma) {
          
        case 0:
          dmr = -cm*Ylm[lm+1];
          dmi = cm*Ylm[lm-1];
          break;
        case 1:
          dmr = cm*Ylm[lm];
          dmi = 0.0;
          break;
        default:
          dmr = cm*Ylm[lm+ma-1];
          dmi = cm*Ylm[lm-ma+1];
        }
      } else {
        dmr = cm*Ylm[lm];
        dmi = 0.0;
        //dmr = (l==1) ? cm*Ylm[lm]:0.0;
        //dmi = 0.0;
      }
      if (m < 0) {
        gx = 0.5*(dpi-dmi);
        gy = -0.5*(dpr+dmr);
      } else {
        gx = 0.5*(dpr-dmr);
        gy = 0.5*(dpi+dmi);
      }
      lm = index(l,m);
      if(ma)
        gradYlm[lm] = NormFactor[lm]*Point_t(gx,gy,gz); 
      else
        gradYlm[lm] = Point_t(gx,gy,gz); 
    }
  }
  for (int i=0; i<Ylm.size(); i++) Ylm[i]*= NormFactor[i];
 //for (int i=0; i<Ylm.size(); i++) gradYlm[i]*= NormFactor[i];
}
//These template functions are slower than the recursive one above
template<class SCT, unsigned L> 
struct SCTFunctor { 
  typedef typename SCT::value_type value_type;
  typedef typename SCT::pos_type pos_type;
  static inline void apply(std::vector<value_type>& Ylm, std::vector<pos_type>& gYlm, const pos_type& p) {
    SCTFunctor<SCT,L-1>::apply(Ylm,gYlm,p);
  }
};

template<class SCT> 
struct SCTFunctor<SCT,1> { 
  typedef typename SCT::value_type value_type;
  typedef typename SCT::pos_type pos_type;
  static inline void apply(std::vector<value_type>& Ylm, std::vector<pos_type>& gYlm, const pos_type& p) {

    const value_type L1 = sqrt(3.0);

    Ylm[1]=L1*p[1];
    Ylm[2]=L1*p[2];
    Ylm[3]=L1*p[0];

    gYlm[1]=pos_type(0.0,L1,0.0);
    gYlm[2]=pos_type(0.0,0.0,L1);
    gYlm[3]=pos_type(L1,0.0,0.0);
  }
};

template<class SCT> 
struct SCTFunctor<SCT,2> { 
  typedef typename SCT::value_type value_type;
  typedef typename SCT::pos_type pos_type;
  static inline void apply(std::vector<value_type>& Ylm, std::vector<pos_type>& gYlm, const pos_type& p) {
    SCTFunctor<SCT,1>::apply(Ylm,gYlm,p);
    value_type x=p[0], y=p[1], z=p[2];
    value_type x2=x*x, y2=y*y, z2=z*z;
    value_type xy=x*y, xz=x*z, yz=y*z; 

    const value_type L0 = sqrt(1.25);
    const value_type L1 = sqrt(15.0);
    const value_type L2 = sqrt(3.75);

    Ylm[4]=L1*xy;
    Ylm[5]=L1*yz;
    Ylm[6]=L0*(2.0*z2-x2-y2);
    Ylm[7]=L1*xz;
    Ylm[8]=L2*(x2-y2);

    gYlm[4]=pos_type(L1*y,L1*x,0.0);
    gYlm[5]=pos_type(0.0,L1*z,L1*y);
    gYlm[6]=pos_type(-2.0*L0*x,-2.0*L0*y,4.0*L0*z);
    gYlm[7]=pos_type(L1*z,0.0,L1*x);
    gYlm[8]=pos_type(2.0*L2*x,-2.0*L2*y,0.0);
  }
};

template<class SCT> 
struct SCTFunctor<SCT,3> { 
  typedef typename SCT::value_type value_type;
  typedef typename SCT::pos_type pos_type;
  static inline void apply(std::vector<value_type>& Ylm, std::vector<pos_type>& gYlm, const pos_type& p) {
    SCTFunctor<SCT,2>::apply(Ylm,gYlm,p);
  }
};


template<class T, class Point_t>
void SphericalTensor<T,Point_t>::evaluateTest(const Point_t& p) {

  const value_type pi = 4.0*atan(1.0);
  const value_type norm = 1.0/sqrt(4.0*pi);

  Ylm[0]=1.0;
  gradYlm[0]=0.0;

  switch (Lmax)
  {
    case(0): break;
    case(1): SCTFunctor<This_t,1>::apply(Ylm,gradYlm,p); break;
    case(2): SCTFunctor<This_t,2>::apply(Ylm,gradYlm,p); break;
    //case(3): SCTFunctor<This_t,3>::apply(Ylm,gradYlm,p); break;
    //case(4): SCTFunctor<This_t,4>::apply(Ylm,gradYlm,p); break;
    //case(5): SCTFunctor<This_t,5>::apply(Ylm,gradYlm,p); break;
    defaults: 
             std::cerr << "Lmax>2 is not valid." << std::endl;
             break;
  }

  for(int i=0; i<Ylm.size(); i++) Ylm[i] *= norm;
  for(int i=0; i<Ylm.size(); i++) gradYlm[i] *= norm;
}

#endif
