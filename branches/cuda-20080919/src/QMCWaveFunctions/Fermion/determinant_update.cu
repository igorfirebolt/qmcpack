#define DET_BLOCK_SIZE 64

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


// The first kernel just computes AinvT * u and also stores the kth
// col of Ainv in global memory
template<typename T>
__global__ void
update_inverse_cuda1 (T *A_g[], T *Ainv_g[], T *u_g[], 
		      T *Ainv_delta_g[], T *Ainv_colk_g[], 
		      int N, int rowstride, int k)
{
  __shared__ T *A, *Ainv, *u, *Ainv_delta, *Ainv_colk;
  if (threadIdx.x==0) {
    A           = A_g[blockIdx.y];
    Ainv        = Ainv_g[blockIdx.y];
    u           = u_g[blockIdx.y];
    Ainv_delta  = Ainv_delta_g[blockIdx.y];
    Ainv_colk   = Ainv_colk_g[blockIdx.y];
  }

  __syncthreads();

  // Store the product Ainv * u in shared memory
  __shared__ T Ainv_delta_shared[DET_BLOCK_SIZE], 
    Ainv_colk_shared[DET_BLOCK_SIZE], u_shared[DET_BLOCK_SIZE],
    uold_shared[DET_BLOCK_SIZE];
  Ainv_delta_shared[threadIdx.x] = 0.0;
  int col = blockIdx.x*DET_BLOCK_SIZE + threadIdx.x;
  int numblocks = N / DET_BLOCK_SIZE;

  // If the column I need to pull from Ainv is in this thread block
  // domain, do the following
  if (blockIdx.x*DET_BLOCK_SIZE <= k && k < (blockIdx.x+1)*DET_BLOCK_SIZE) {
    for (int block=0; block<numblocks; block++) {
      u_shared[threadIdx.x] = u[block*DET_BLOCK_SIZE+threadIdx.x];
      uold_shared[threadIdx.x] = 
      	A[k*rowstride + block*DET_BLOCK_SIZE+threadIdx.x];
      // Write new row into A matrix
      A[k*rowstride + block*DET_BLOCK_SIZE+threadIdx.x] = u_shared[threadIdx.x];
      __syncthreads();
      for (int i=0; i<DET_BLOCK_SIZE; i++) {
      	int row = block*DET_BLOCK_SIZE + i;
	
      	T a = Ainv[row*rowstride+col];
      	if (col == k)
      	  Ainv_colk_shared[i] = a;
      	Ainv_delta_shared[threadIdx.x] += a*(u_shared[i]-uold_shared[i]);
      }
      __syncthreads();
      Ainv_colk[block*DET_BLOCK_SIZE+threadIdx.x] = Ainv_colk_shared[threadIdx.x];
    }
  }
  else {
    for (int block=0; block<numblocks; block++) {
      u_shared[threadIdx.x] = u[block*DET_BLOCK_SIZE+threadIdx.x];
      uold_shared[threadIdx.x] = 
  	A[k*rowstride + block*DET_BLOCK_SIZE+threadIdx.x];
      // Write new row into A matrix
      A[k*rowstride + block*DET_BLOCK_SIZE+threadIdx.x] = u_shared[threadIdx.x];
      __syncthreads();
      for (int i=0; i<DET_BLOCK_SIZE; i++) {
  	int row = block*DET_BLOCK_SIZE + i;
  	Ainv_delta_shared[threadIdx.x] += 
  	  Ainv[row*rowstride+col]*(u_shared[i]- uold_shared[i]);
      }
    }
  }

  __syncthreads();
  
  // Write the data back to global memory
  Ainv_delta[col]    = Ainv_delta_shared[threadIdx.x];
}


template<typename T>
__global__ void
update_inverse_cuda2 (T *Ainv_g[], T *u_g[], T *Ainv_delta_g[],
		      T *Ainv_colk_g[], int N, int rowstride, int k)
{
  __shared__ T *Ainv, *Ainv_delta, *Ainv_colk;
  if (threadIdx.x==0) {
    Ainv     = Ainv_g[blockIdx.y];
    Ainv_delta    = Ainv_delta_g[blockIdx.y];
    Ainv_colk = Ainv_colk_g[blockIdx.y];
  }
  __syncthreads();

  __shared__ T Ainv_delta_shared[DET_BLOCK_SIZE];
  __shared__ T  Ainv_colk_shared[DET_BLOCK_SIZE];
  int col = blockIdx.x*DET_BLOCK_SIZE + threadIdx.x;
  // Read the data back from global memory
  Ainv_delta_shared[threadIdx.x] = Ainv_delta[col];
  Ainv_colk_shared[threadIdx.x] = Ainv_colk[col];
  __shared__ T prefact;
  if (threadIdx.x == 0)
    prefact = -1.0f/(1.0f+Ainv_delta[k]);
  __syncthreads();
		   
  int numblocks = N / DET_BLOCK_SIZE;
  for (int block=0; block<numblocks; block++) {
    Ainv_colk_shared[threadIdx.x] = 
      prefact*Ainv_colk[block*DET_BLOCK_SIZE+threadIdx.x];
    __syncthreads();
    for (int i=0; i<DET_BLOCK_SIZE; i++) {
      int row = block*DET_BLOCK_SIZE + i;
      Ainv[row*rowstride+col] += 
	Ainv_delta_shared[threadIdx.x]*Ainv_colk_shared[i];
    }
  }
}


void
update_inverse_cuda(float *A_g[], float *Ainv_g[], float *u_g[], 
		    float *Ainv_delta_g[], float *Ainv_colk_g[], 
		    int N, int rowstride, int iat, int numWalkers)
{
  dim3 dimBlock(DET_BLOCK_SIZE);
  dim3 dimGrid(N/DET_BLOCK_SIZE, numWalkers);

  update_inverse_cuda1<float><<<dimGrid,dimBlock>>>
    (A_g, Ainv_g, u_g, Ainv_delta_g, Ainv_colk_g, N, rowstride, iat);
  update_inverse_cuda2<float><<<dimGrid,dimBlock>>>
    (Ainv_g, u_g, Ainv_delta_g, Ainv_colk_g, N, rowstride, iat);
  cudaThreadSynchronize();
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    fprintf (stderr, "CUDA error in update_inverse_cuda:\n  %s\n",
	     cudaGetErrorString(err));
    abort();
  }
}

void
update_inverse_cuda(double *A_g[], double *Ainv_g[], double *u_g[], 
		    double *Ainv_delta_g[], double *Ainv_colk_g[], 
		    int N, int rowstride, int iat, int numWalkers)
{
  dim3 dimBlock(DET_BLOCK_SIZE);
  dim3 dimGrid(N/DET_BLOCK_SIZE, numWalkers);

  fprintf (stderr, "dimBlock = %d\n", dimBlock.x);
  fprintf (stderr, "dimGrid  = (%d, %d)\n", dimGrid.x, dimGrid.y);

  update_inverse_cuda1<double><<<dimGrid,dimBlock>>>
    (A_g, Ainv_g, u_g, Ainv_delta_g, Ainv_colk_g, N, rowstride, iat);
  update_inverse_cuda2<double><<<dimGrid,dimBlock>>>
    (Ainv_g, u_g, Ainv_delta_g, Ainv_colk_g, N, rowstride, iat);

  cudaThreadSynchronize();
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    fprintf (stderr, "CUDA error in update_inverse_cuda:\n  %s\n",
	     cudaGetErrorString(err));
    abort();
  }

}


template<typename T, int BS>
__global__ void
calc_ratios (T *Ainv_list[], T *new_row_list[], 
	     T *ratio, int N, int row_stride, int elec)
{
  int tid = threadIdx.x;

  int col = /*blockIdx.x*BS * */tid;
  __shared__ T *Ainv, *new_row;

  if (tid == 0) {
    Ainv = Ainv_list[blockIdx.x];
    new_row = new_row_list[blockIdx.x];
  }
  __syncthreads();
  __shared__ T new_row_shared[BS];
   
  if (col < N) 
    new_row_shared[tid] = new_row[tid];
    
  __shared__ T Ainv_colk_shared[BS];
  // This is *highly* uncoallesced, but we just have to eat it to allow
  // other kernels to operate quickly.
  if (col < N)
    Ainv_colk_shared[tid] = Ainv[col*row_stride + elec];
  __syncthreads();

  __shared__ T Ainv_new_row[BS];
  if (col < N)
    Ainv_new_row[tid] = Ainv_colk_shared[tid] * new_row_shared[tid];
    
  __syncthreads();
    // Now, we have to dot
  for (unsigned int s=BS/2; s>0; s>>=1) {
    if (tid < s && (tid+s) < N)
      Ainv_new_row[tid] += Ainv_new_row[tid + s];
    __syncthreads();
  }
  if (tid == 0)      ratio[blockIdx.x] = Ainv_new_row[0];
}


void
determinant_ratios_cuda (float *Ainv_list[], float *new_row_list[],
			 float *ratios, int N, int row_stride, int iat,
			 int numWalkers)
{
  dim3 dimBlock(N);
  dim3 dimGrid(numWalkers);

  cudaThreadSynchronize();
  cudaError_t err1 = cudaGetLastError();
  if (err1 != cudaSuccess) {
    fprintf (stderr, "CUDA error before determinant_ratios_cuda:\n  %s\n",
	     cudaGetErrorString(err1));
    abort();
  }

  if (N <= 32) 
    calc_ratios<float,32><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else if (N <= 64)
    calc_ratios<float,64><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else if (N <= 128)
    calc_ratios<float,128><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else if (N <= 256)
    calc_ratios<float,256><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else if (N <= 512)
    calc_ratios<float,512><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else if (N <= 1024)
    calc_ratios<float,1024><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else {
    fprintf (stdout, "Error:  N too large for CUDA evaluation.\n");
    abort();
  }

  cudaThreadSynchronize();
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    fprintf (stderr, "CUDA error in determinant_ratios_cuda:\n  %s\n",
	     cudaGetErrorString(err));
    abort();
  }

}

void
determinant_ratios_cuda (double *Ainv_list[], double *new_row_list[],
			 double *ratios, int N, int row_stride, int iat,
			 int numWalkers)
{
  dim3 dimBlock(N);
  dim3 dimGrid(numWalkers);

  if (N <= 32) 
    calc_ratios<double,32><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else if (N <= 64)
    calc_ratios<double,64><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else if (N <= 128)
    calc_ratios<double,128><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else if (N <= 256)
    calc_ratios<double,256><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else if (N <= 512)
    calc_ratios<double,512><<<dimGrid,dimBlock>>>(Ainv_list, new_row_list, ratios, N, row_stride, iat);
  else {
    fprintf (stdout, "Error:  N too large for CUDA evaluation.\n");
    abort();
  }
  cudaThreadSynchronize();
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    fprintf (stderr, "CUDA error in determinant_ratios_cuda:\n  %s\n",
	     cudaGetErrorString(err));
    abort();
  }
}

#define RATIO_BS 16

template<typename T>
__global__ void
all_ratios_kernel (T *Ainv_list[], T *new_mat_list[], 
		   T *ratio_list[], int N, int row_stride)
{
  __shared__ T *Ainv, *new_mat, *ratio;
  
  if (threadIdx.x == 0 && threadIdx.y == 0) {
    Ainv    = Ainv_list[blockIdx.x];
    new_mat = new_mat_list[blockIdx.x];
    ratio   = ratio_list[blockIdx.x];
  }

  __shared__ float Ainv_block[RATIO_BS][RATIO_BS+1];
  // __shared__ float new_block[RATIO_BS][RATIO_BS+1];
  __shared__ float ratio_block[RATIO_BS][RATIO_BS+1];
  unsigned int numBlocks = N >> 4;
  if (N & 15)
    numBlocks++;

  for (unsigned int yBlock=0; yBlock<numBlocks; yBlock++) {
    ratio_block[threadIdx.y][threadIdx.x] = 0.0f;
    __syncthreads();
    for (unsigned int xBlock=0; xBlock<numBlocks; xBlock++) {
      unsigned int Ainv_xIndex = yBlock * RATIO_BS + threadIdx.x;
      unsigned int Ainv_yIndex = xBlock * RATIO_BS + threadIdx.y;
      unsigned int Ainv_index  = Ainv_yIndex*row_stride + Ainv_xIndex;
      if ((Ainv_xIndex < N) && (Ainv_yIndex < N))
	Ainv_block[threadIdx.x][threadIdx.y] = Ainv[Ainv_index];
      __syncthreads();
      unsigned int new_xIndex = xBlock * RATIO_BS + threadIdx.x;
      unsigned int new_yIndex = yBlock * RATIO_BS + threadIdx.y;
      unsigned int new_index  = new_yIndex*row_stride + new_xIndex;

      if ((new_xIndex < N) && (new_yIndex < N))
	ratio_block[threadIdx.y][threadIdx.x] +=
	  new_mat[new_index] * Ainv_block[threadIdx.y][threadIdx.x];
    }
    __syncthreads();
    // Now, we have to do the reduction across the ratio_blocks
    
    if (threadIdx.x < 8)
      ratio_block[threadIdx.y][threadIdx.x] +=
	ratio_block[threadIdx.y][threadIdx.x+8];
    if (threadIdx.x < 4)
      ratio_block[threadIdx.y][threadIdx.x] +=
	ratio_block[threadIdx.y][threadIdx.x+4];
    if (threadIdx.x < 2)
      ratio_block[threadIdx.y][threadIdx.x] +=
	ratio_block[threadIdx.y][threadIdx.x+2];
    if (threadIdx.x < 1) {
      ratio_block[threadIdx.y][threadIdx.x] +=
	ratio_block[threadIdx.y][threadIdx.x+1];
    }
    __syncthreads();
    if (threadIdx.y == 0 && (yBlock * RATIO_BS + threadIdx.x) < N)
      ratio[yBlock * RATIO_BS + threadIdx.x] = ratio_block[threadIdx.x][0];
  }      
}


void
all_ratios_kernel (float *Ainv_list[], float *new_mat_list[],
		   float *ratio_list[], int N, int row_stride, int num_mats)
{
  dim3 dimBlock(RATIO_BS, RATIO_BS);
  dim3 dimGrid (num_mats);

  all_ratios_kernel<float><<<dimGrid,dimBlock>>>
    (Ainv_list, new_mat_list, ratio_list, N, row_stride);
}


#include <stdlib.h>

void
test_all_ratios_kernel()
{
  int N = 128;

  float *A, *A_d, *Ainv, *Ainv_d, *ratio, *ratio_d;

  cudaMalloc ((void**)&A_d,    N*N*sizeof(float));
  cudaMalloc ((void**)&Ainv_d, N*N*sizeof(float));  
  cudaMalloc ((void**)&ratio_d, 1*N*sizeof(float));
  A     = (float *)malloc (N*N*sizeof(float));
  Ainv  = (float *)malloc (N*N*sizeof(float));
  ratio = (float *)malloc (1*N*sizeof(float));

  float ratio2[N];
  for (int i=0; i<N; i++)
    for (int j=0; j<N; j++) {
      A[i*N+j] = 1.0f+drand48();
      Ainv[i*N+j] = 1.0f+drand48();
    }
  
  cudaMemcpy (A_d,     A,    N*N*sizeof(float), cudaMemcpyHostToDevice);  
  cudaMemcpy (Ainv_d,  Ainv, N*N*sizeof(float), cudaMemcpyHostToDevice);

  float **A_list, **A_list_d, **Ainv_list, **Ainv_list_d, **ratio_list, **ratio_list_d;
  cudaMalloc ((void**)&A_list_d,     sizeof(float*));
  cudaMalloc ((void**)&Ainv_list_d,  sizeof(float*));
  cudaMalloc ((void**)&ratio_list_d, sizeof(float*));
  A_list     = (float **)malloc (sizeof(float*));
  Ainv_list  = (float **)malloc (sizeof(float*));
  ratio_list = (float **)malloc (sizeof(float*));

  A_list[0] = A_d;
  Ainv_list[0] = Ainv_d;
  ratio_list[0] = ratio_d;

  cudaMemcpy (A_list_d,    A_list,      sizeof(float*), cudaMemcpyHostToDevice);
  cudaMemcpy (Ainv_list_d, Ainv_list,   sizeof(float*), cudaMemcpyHostToDevice);
  cudaMemcpy (ratio_list_d, ratio_list, sizeof(float*), cudaMemcpyHostToDevice);

  all_ratios_kernel (Ainv_list_d, A_list_d, ratio_list_d, N, N, 1);


  cudaMemcpy (ratio, ratio_d, N*sizeof(float), cudaMemcpyDeviceToHost);

  for (int i=0; i<N; i++) {
    ratio2[i] = 0.0f;
    for (int j=0; j<N; j++)
      ratio2[i] += A[i*N+j]*Ainv[j*N+i];
    fprintf (stderr, "%3d  %10.6f  %10.6f\n", i, ratio2[i], ratio[i]);
  }
  

}


#ifdef CUDA_TEST_MAIN
main()
{
  test_all_ratios_kernel();
}



#endif