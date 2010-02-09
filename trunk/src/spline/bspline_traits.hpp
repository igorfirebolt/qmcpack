//////////////////////////////////////////////////////////////////
// (c) Copyright 2006-  by Jeongnim Kim and Ken Esler           //
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &          //
//   Materials Computation Center                               //
//   University of Illinois, Urbana-Champaign                   //
//   Urbana, IL 61801                                           //
//   e-mail: jnkim@ncsa.uiuc.edu                                //
//                                                              //
// Supported by                                                 //
//   National Center for Supercomputing Applications, UIUC      //
//   Materials Computation Center, UIUC                         //
//////////////////////////////////////////////////////////////////
/** @file bspline_traits.h
 */
#ifndef QMCPLUSPLUS_BSPLINE_TRAITS_H
#define QMCPLUSPLUS_BSPLINE_TRAITS_H

extern "C"
{
#include <einspline/bspline.h>
#include <einspline/multi_bspline_structs.h>
}

namespace qmcplusplus
{
  /** determine if EngT (e.g., einspline engine) handles real data or complex data
   *
   * Default is true and complex cases are specialized
   */
  template<typename EngT>
  struct is_real_bspline
  {
      static const bool value=true;
  };

  ///specialization for multi_UBspline_3d_z
  template<>
  struct is_real_bspline<multi_UBspline_3d_z>
  {
      static const bool value=false;
  };

  ///specialization for multi_UBspline_3d_z
  template<>
  struct is_real_bspline<multi_UBspline_3d_c>
  {
      static const bool value=false;
  };
  //
  /** struct to check if two types are the same
   */
  template<typename T1, typename T2>
    struct type_check
    {
      static const bool value=false;
    };

  template<typename T1>
    struct type_check<T1,T1>
    {
      static const bool value=true;
    };

  /** dummy traits class for bspline engine
   *
   * Should fail to instantiate invalid engines if the trait class is not implemented
   * The specialization provides
   * - DIM, physical dimension
   * - real_type real data type
   * - value_type value data type
   * - Spline_t einspline engine type
   */
  template<typename EngT>
  struct bspline_engine_traits {};

#if OHMMS_DIM == 3
  /** specialization with multi_UBspline_3d_d
   */
  template<>
  struct bspline_engine_traits<multi_UBspline_3d_d>
  {
    enum {DIM=3};
    typedef double real_type;
    typedef double value_type;
    typedef multi_UBspline_3d_d Spline_t;
  };

  ///specialization with multi_UBspline_3d_z
  template<>
  struct bspline_engine_traits<multi_UBspline_3d_z>
  {
    enum {DIM=3};
    typedef double value_type;
    typedef std::complex<double> value_type;
    typedef multi_UBspline_3d_z Spline_t;
  };

  /** specialization with multi_UBspline_3d_d
   */
  template<>
  struct bspline_engine_traits<multi_UBspline_3d_s>
  {
    enum {DIM=3};
    typedef float real_type;
    typedef float value_type;
    typedef multi_UBspline_3d_s Spline_t;
  };

  ///specialization with multi_UBspline_3d_z
  template<>
  struct bspline_engine_traits<multi_UBspline_3d_c>
  {
    enum {DIM=3};
    typedef float value_type;
    typedef std::complex<float> value_type;
    typedef multi_UBspline_3d_c Spline_t;
  };

#else
#error "DIMENSION!=3 is not implemented yet
#endif
}
#endif
