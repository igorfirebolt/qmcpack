#ifndef CUDA_COULOMB_H
#define CUDA_COULOMB_H

#include <einspline/bspline_base.h>
#include <einspline/bspline_structs_cuda.h>

class TextureSpline
{
public:
  float rMin, rMax;
  int NumPoints;
  int MyTexture;
  cudaArray *myArray;
  void set (float data[], int numPoints, float rmin, float rmax);
  void set (double data[], int numPoints, double rmin, double rmax);

  TextureSpline();
  ~TextureSpline();
};

void
CoulombAA_SR_Sum(float *R[], int N, float rMax, int Ntex, int texNum,
		 float lattice[], float latticeInv[], float sum[],
		 int numWalkers);

void
CoulombAB_SR_Sum(float *R[], int Nelec, float I[], int Ifirst, int Ilast,
		 float rMax, int Ntex, int textureNum, 
		 float lattice[], float latticeInv[], 
		 float sum[], int numWalkers);

void
eval_rhok_cuda(float *R[], int numr, float kpoints[], 
	       int numk, float* rhok[], int numWalkers);

void
eval_vk_sum_cuda (float *rhok[], float vk[], int numk, float sum[],
		  int numWalkers);

void
eval_rhok_cuda(float *R[], int first, int last, float kpoints[], 
	       int numk, float* rhok[], int numWalkers);

void
eval_vk_sum_cuda (float *rhok1[], float *rhok2[], 
		  float vk[], int numk, float sum[],
		  int numWalkers);

// In this case, the rhok2 is the same for all walkers
void
eval_vk_sum_cuda (float *rhok1[], float rhok2[], 
		  float vk[], int numk, float sum[],
		  int numWalkers);



// Double-precision
void
CoulombAA_SR_Sum(double *R[], int N, double rMax, int Ntex, int texNum,
		 double lattice[], double latticeInv[], double sum[],
		 int numWalkers);

void
CoulombAB_SR_Sum(double *R[], int Nelec, double I[], int Ifirst, int Ilast,
		 double rMax, int Ntex, int textureNum, 
		 double lattice[], double latticeInv[], 
		 double sum[], int numWalkers);

void
eval_rhok_cuda(double *R[], int numr, double kpoints[], 
	       int numk, double* rhok[], int numWalkers);

void
eval_vk_sum_cuda (double *rhok[], double vk[], int numk, double sum[],
		  int numWalkers);

void
eval_rhok_cuda(double *R[], int first, int last, double kpoints[], 
	       int numk, double* rhok[], int numWalkers);

void
eval_vk_sum_cuda (double *rhok1[], double *rhok2[], 
		  double vk[], int numk, double sum[],
		  int numWalkers);

// In this case, the rhok2 is the same for all walkers
void
eval_vk_sum_cuda (double *rhok1[], double rhok2[], 
		  double vk[], int numk, double sum[],
		  int numWalkers);



void
MPC_SR_Sum(float *R[], int N, float lattice[], float latticeInv[], 
	   float sum[], int numWalkers);
void
MPC_SR_Sum(double *R[], int N, double lattice[], double latticeInv[], 
	   double sum[], int numWalkers);
void
MPC_LR_Sum(float *R[], int N, UBspline_3d_s_cuda *spline, 
	   float latticeInv[], float sum[], int numWalkers);
void
MPC_LR_Sum(double *R[], int N, UBspline_3d_d_cuda *spline,
	   double latticeInv[], double sum[], int numWalkers);
void init_Acuda();

#endif
