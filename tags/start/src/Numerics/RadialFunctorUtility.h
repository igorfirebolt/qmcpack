// -*- C++ -*-
#ifndef RADIALFUNCTIONUTILITY_H
#define RADIALFUNCTIONUTILITY_H

#ifndef ONEDIMINTEGRATION_H
#include "Numerics/OneDimIntegration.h"
#endif
/**@file RadialFunctorUtility.h
   @brief Utility functions for atomic Hartree-fock solutions
   @authors Jeongnim Kim, Jordan Vincent
   @note  The original Prim was written in F90 by Tim Wilkens.
 */

/*!\fn
 * \param g \f$Y_k(n_al_a;n_bl_b/r)\f$
 * \param a \f$psi_a\f$
 * \param b \f$\psi_b\f$
 * \param prefactor The scaling constant (simple multiplication factor)
 * \return double The result of the calculation after integration
 *
 * \brief Calculates \f$\int_0^{\infty} dr \psi_a(r) \psi_b(r)
 Y_k(n_al_a,n_bl_b/r) \psi_b(r) \f$ with appropriate scaling.
 */

template<class T, class GF>
inline T Phisq_x_Yk(const GF& g, const GF& a, 
		    const GF& b, T prefactor) {

  GF t(g);
  for(int i=0; i < t.size(); i++) {
    t(i) = prefactor * g(i)*a(i)*b(i);
  }
  return integrate_RK2(t);
}


/*! \fn
 * \param g return 
 * \param a \f$psi_a\f$
 * \param b \f$\psi_b\f$
 * \param k the integer parameter.
 *
 * \brief Calculates the function: 
 \f[
 \frac{{\cal Y}_k(n_a l_a; n_b l_b/r)}{r} 
 \f]
 *
 On Page 17 of "Quantum Theory of Atomic
 Structure" Vol 2 by JC Slater.  Note: (here we use \f$ u(r) \f$ not \f$ R(r) \f$,
 where the relation is \f$ u(r) = rR(r) \f$) this function is used extensively
 to calculate the Coulomb and Exchange potentials.  The actual
 equation is:
 \f[ 
 \frac{{\cal Y}_k(n_al_a;n_bl_b/r)}{r} = 
 \frac{1}{r^{k+1}}
 \int_{0}^{r} dr' \: r'^k u_{n_a l_a}(r') u_{n_b l_b}(r') 
 + r^{k} \int_{r}^{\infty}dr' \:
 r'^{-k-1} u_{n_a l_a}(r') u_{n_b l_b}(r')
 \f]
 */
template<class GF>
inline void
Ykofr(GF& g, const GF& a, const GF& b, int k) {

  typedef typename GF::value_type value_type;
  int n = g.size();

  //The integrands of each of the two integrals 	 
  GF first_integrand(g);
  GF second_integrand(g);

  vector<value_type> r_to_k(n);
  vector<value_type> r_to_minus_k_plus_one(n);

  //Store values for r_to_k and r_to_minus_kplus1
  for(int i=0; i < n; ++i) {
    value_type r0 = g.r(i)+1e-12;
    value_type t = pow(r0,k); // r0^k
    r_to_k[i] = t;
    r_to_minus_k_plus_one[i] = 1.0/(t*r0); // 1/r0^(k+1)
    value_type ab = a(i)*b(i);
    first_integrand(i) = r_to_k[i]*ab;
    second_integrand(i) = r_to_minus_k_plus_one[i]*ab;
  }

  //Allow r_1 to range over all the radial grid points.
  //Store the value of the integral at each grid point in 
  //temporary grid objects A and B. 

  GF A(g), B(g);
  integrate_RK2_forward(first_integrand,A);
  integrate_RK2_backward(second_integrand,B);

  //Too many for loops here  *Yk_r = r_to_minus_k_plus_one * A + r_to_k * B;
  for(int i=0; i < n; ++i) {
    g(i) = r_to_minus_k_plus_one[i]*A(i)+r_to_k[i]*B(i);
  }
}

/*! \fn 
 * \param g the grid function to be returned
 * \param y the grid function to be transformed
 * \param a \f$\psi_a\f$
 * \param b \f$\psi_b\f$
 * \param coeff a coefficient
 *
 * \brief Makes \f$V_{Exchange}\f$ a local function.  
 *
 \f$ V_{Exchange} \f$ is a non-local operator: 
 \f[
 \hat{V}_{Exchange} \psi_a(r) = -\sum_b
 \delta_{\sigma_a,\sigma_b}\int dr'
 \frac{\psi_b^*(r')\psi_a(r')}{|r-r'|}\psi_b(r),
 \f]
 It is possible to transform this into a local operator by multiplying 
 and dividing by $\psi_a(r)$: 
 \f[
 -\sum_b \delta_{\sigma_a,\sigma_b}\int dr'
 \frac{\psi_b^* (r')\psi_a(r')\psi_b(r')\psi_b(r)}{\psi_a(r)|r-r'|} \psi_a(r) 
 \f]
*/

template<class GF>
inline void
Make_Loc_Pot(GF& g, const GF& y, const GF& a, const GF& b, 
	     typename GF::value_type coeff) {

  int max_pt, pt=1;
  typename GF::value_type  ratio;
  while(a(pt)/a(pt-1) >= 1.0 && pt<a.size()-1) {
    pt++;
  }
  max_pt = pt;
  const double tol = pow(10.0,-10.0);
  for(pt=0; pt < g.size(); pt++) {
    if(fabs(a(pt)) > tol){
      ratio = b(pt)/a(pt);
      if(pt >= max_pt && fabs(ratio) > 1000) {ratio = 1000000.0 / ratio;}
      g(pt) -= coeff*ratio*y(pt);
    }
  }
}

//}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

