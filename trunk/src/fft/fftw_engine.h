//////////////////////////////////////////////////////////////////
// (c) Copyright 2010-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
/** @file fftw_engine.h
 * @brief definition of fft_engine_base<fftw_real,FFTW_ENG> 
 */
#ifndef QMCPLUSPLUS_FFTW_ENGINE_BASE_H
#define QMCPLUSPLUS_FFTW_ENGINE_BASE_H

#include <fftw3.h>
typedef double fftw_real;

namespace qmcplusplus
{
  /** fftw_base to be specialized with the precision
   *
   * fftw_base class defines the interfaces to fftw libraries with matching argument types
   */
  template<>
    class fft_engine_base<fftw_real,FFTW_ENG>
    {
      template<typename T1>
        inline fftw_complex* mangle(T1* in)
        {
          return reinterpret_cast<fftw_complex*>(in);
        }

      public:
        typedef std::complex<fftw_real> complex_type;
        typedef fftw_plan fft_plan_type;
        fft_plan_type forward_plan;
        fft_plan_type backward_plan;

        ///default constructor
        fft_engine_base() :forward_plan(0), backward_plan(0) { }

        ///virtual destructor to clean up the plans
        virtual ~fft_engine_base() { clear(); }

        ///clean up the allocated data
        void clear()
        {
          if(forward_plan) {fftw_destroy_plan(forward_plan); forward_plan=0;}
          if(backward_plan) {fftw_destroy_plan(backward_plan); backward_plan=0;}
        }

        /** plan for outplace, complex-to-complex  transform
         */
        void create_plan(int* desc, complex_type* in, complex_type* out 
            , int dir, unsigned uflag)
        {
          if(dir==FFTW_FORWARD)
          { 
            if(forward_plan) fftw_destroy_plan(forward_plan);
            forward_plan=create_plan_c2c(desc,mangle(in),mangle(out),dir,uflag);
          }
          else
          {
            if(backward_plan) fftw_destroy_plan(backward_plan);
            backward_plan=create_plan_c2c(desc,mangle(in),mangle(out),dir,uflag);
          }
        }

        /** plan for inplace or outplace, real-to-complex  transform
         */
        void create_plan(int* desc, fftw_real* in, fftw_real* out , int dir, unsigned uflag)
        {
          if(dir==FFTW_FORWARD)
          { 
            if(forward_plan) fftw_destroy_plan(forward_plan);
            forward_plan=create_plan_r2c(desc,in,mangle(out),uflag);
          }
          else
          {
            if(backward_plan) fftw_destroy_plan(backward_plan);
            backward_plan=create_plan_c2r(desc,mangle(in),out,uflag);
          }
        }

        /** plan for outplace, real-to-complex */
        void create_plan(int* desc, fftw_real* in, complex_type* out, int idir, unsigned uflag)
        {
          if(forward_plan) fftw_destroy_plan(forward_plan);
          forward_plan= create_plan_r2c(desc,in,mangle(out),uflag);
        }

        /** plan for outplace, complex-to-real */
        void create_plan(int* desc, complex_type* in, fftw_real* out, int idir, unsigned uflag)
        {
          if(backward_plan) fftw_destroy_plan(backward_plan);
          backward_plan=create_plan_c2r(desc,mangle(in),out,uflag);
        }

        inline void execute_fft(complex_type* inout)
        {
          fftw_execute_dft(forward_plan,mangle(inout),mangle(inout));
        }
        inline void execute_ifft(complex_type* inout)
        {
          fftw_execute_dft(backward_plan,mangle(inout),mangle(inout));
        }

        inline void execute_fft(fftw_real* inout)
        {
          fftw_execute_dft(forward_plan,mangle(inout),mangle(inout));
        }
        inline void execute_ifft(fftw_real* inout)
        {
          fftw_execute_dft(backward_plan,mangle(inout),mangle(inout));
        }

        inline void execute_fft(complex_type* in, complex_type* out)
        {
          fftw_execute_dft(forward_plan,mangle(in),mangle(out));
        }

        inline void execute_ifft(complex_type* in, complex_type* out)
        {
          fftw_execute_dft(backward_plan,reinterpret_cast<fftw_complex*>(in),reinterpret_cast<fftw_complex*>(out));
        }

        inline void execute_fft(fftw_real* in, complex_type* out)
        {
          fftw_execute_dft_r2c(forward_plan,in,reinterpret_cast<fftw_complex*>(out));
        }

        inline void execute_ifft(complex_type* in, fftw_real* out)
        {
          fftw_execute_dft_c2r(backward_plan,reinterpret_cast<fftw_complex*>(in),out);
        }

      private:
        fft_plan_type create_plan_c2c(int* desc, fftw_complex* in, fftw_complex* out, int idir, unsigned uflag)
        {
          fftw_iodim data_dims;
          fftw_iodim howmany_dims;
          if(idir==FFTW_FORWARD)
          {
            data_dims.n=desc[FFT_LENGTH]; 
            data_dims.is=desc[FFT_IN_STRIDE]; 
            data_dims.os=desc[FFT_OUT_STRIDE];
            howmany_dims.n=desc[FFT_NUMBER_OF_TRANSFORMS]; 
            howmany_dims.is=desc[FFT_IN_DISTANCE]; 
            howmany_dims.os=desc[FFT_OUT_DISTANCE];
          }
          else
          {
            data_dims.n=desc[FFT_LENGTH]; 
            data_dims.is=desc[FFT_OUT_STRIDE]; 
            data_dims.os=desc[FFT_IN_STRIDE];
            howmany_dims.n=desc[FFT_NUMBER_OF_TRANSFORMS]; 
            howmany_dims.is=desc[FFT_OUT_DISTANCE]; 
            howmany_dims.os=desc[FFT_IN_DISTANCE];
          }
          return fftw_plan_guru_dft(1,&data_dims, 1,&howmany_dims,in,out,idir,uflag);
        }

        fft_plan_type create_plan_r2c(int* desc, fftw_real* in, fftw_complex* out, unsigned uflag)
        {
          fftw_iodim data_dims;
          fftw_iodim howmany_dims;
          data_dims.n=desc[FFT_LENGTH]; data_dims.is=desc[FFT_IN_STRIDE]; data_dims.os=desc[FFT_OUT_STRIDE];
          howmany_dims.n=desc[FFT_NUMBER_OF_TRANSFORMS]; 
          howmany_dims.is=desc[FFT_IN_DISTANCE]; 
          howmany_dims.os=desc[FFT_OUT_DISTANCE];
          return fftw_plan_guru_dft_r2c(1,&data_dims, 1,&howmany_dims, in, out,uflag);
        }

        /** create complex-to-real backward plan 
         * @param dims fft size
         * @param howmany number of concurrent ffts
         * @param in complex input data
         * @param out real output data
         * @param uflag fftw plan (measure ...)
         */
        fft_plan_type create_plan_c2r(int* desc, fftw_complex* in, fftw_real* out, unsigned uflag)
        {
          fftw_iodim data_dims;
          fftw_iodim howmany_dims;
          data_dims.n=desc[FFT_LENGTH]; data_dims.is=desc[FFT_OUT_STRIDE]; data_dims.os=desc[FFT_IN_STRIDE];
          howmany_dims.n=desc[FFT_NUMBER_OF_TRANSFORMS]; 
          howmany_dims.is=desc[FFT_OUT_DISTANCE]; 
          howmany_dims.os=desc[FFT_IN_DISTANCE];
          return fftw_plan_guru_dft_c2r(1,&data_dims, 1,&howmany_dims,in,out,uflag);
        }

    };

}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 3738 $   $Date: 2009-04-07 02:08:20 -0500 (Tue, 07 Apr 2009) $
 * $Id: fftw_engine.h 3738 2009-04-07 07:08:20Z jnkim $ 
 ***************************************************************************/
