#ifndef CUDA_ALLOCATOR_H
#define CUDA_ALLOCATOR_H

#ifdef QMC_CUDA
  #include <cuda_runtime_api.h>
#endif
#include <malloc.h>
#include <iostream>
#include <string>

template<typename T> class cuda_allocator;
template<>
class cuda_allocator<void>
{
public:
  typedef void*  pointer;
  typedef const  void* const_pointer;
  typedef void value_type;

  template<class T1> struct rebind 
  { 
    typedef cuda_allocator<T1> other; 
  };
};



extern size_t total_allocated;


template<typename T> 
class cuda_allocator 
{
public:
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;
  template<typename U> struct rebind { typedef cuda_allocator<U> other; };

  
  cuda_allocator() throw() { }
  cuda_allocator(const cuda_allocator&) throw() { }
  template<typename U> cuda_allocator(const cuda_allocator<U>&) throw() { }
  ~cuda_allocator() throw() { };

  pointer address(reference x) const 
  { return &x; }

  const_pointer address(const_reference x) const 
  { return &x; }
  
  pointer allocate(size_type s, cuda_allocator<void>::const_pointer hint = 0)
  {
#ifdef QMC_CUDA 
    if (s) {
      pointer mem;
      cudaMalloc ((void**)&mem, s*sizeof(T));
      //fprintf (stderr, "mem = %p\n", mem);
      cudaError_t err = cudaGetLastError();
      if (err != cudaSuccess) {
	fprintf (stderr, "Failed to allocate %ld:\n  %s\n",
		 s*sizeof(T), cudaGetErrorString(err));
	abort();
      }
      else {
	total_allocated += s;
	fprintf (stderr, "Total GPU mem usage=%ldk\n",
		 total_allocated>>10);
      }
      return mem;
    }
    else
      return 0;
#else
    return (pointer) malloc(s*sizeof(T));
#endif
  }
  
  void deallocate(pointer p, size_type n)
  {
#ifdef QMC_CUDA
    fprintf (stderr, "Freeing %ld on GPU card.  Total = %ldk\n",
	     n, total_allocated>>10);
    cudaFree (p);
    total_allocated -= n;
#endif
  }

  size_type max_size() const throw()
  { return (size_type)1 << 32 - 1; }
  
  void construct(pointer p, const T& val)
  { 
    //new(static_cast<void*>(p)) T(val);  
  }
  
  void destroy(pointer p) {
    //p->~T();
  }
};

#include <vector>

template<typename T> class host_vector;

template<typename T>
class cuda_vector : public std::vector<T, cuda_allocator<T> >
{
private:
  size_t mySize;
  std::string myName;

public:
  cuda_vector(std::string name) : std::vector<T, cuda_allocator<T> >()
  {
    mySize = 0;
    myName = name;
  }

  cuda_vector() : std::vector<T, cuda_allocator<T> >()
  {
    mySize = 0;
  }

  cuda_vector(int size) : 
    std::vector<T, cuda_allocator<T> > ()
  { resize(size); }

  cuda_vector(const host_vector<T> &vec);

  void resize (size_t size)
  {
    fprintf (stderr, "Resizing %s to %ld\n", myName.c_str(), size);
    //std::vector<T,cuda_allocator<T> >::resize(size);
    std::vector<T,cuda_allocator<T> >::reserve(size);
    mySize = size;
#ifdef QMC_CUDA
    cudaMemset((void*)data(), 0, this->size()*sizeof(T));
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
      fprintf (stderr, "CUDA error in resize():\n  %s\n",
	       cudaGetErrorString(err));
      abort();
    }
#endif
  }

  inline size_t size() const
  { return mySize;  }

  cuda_vector(const cuda_vector<T> &vec) :
    std::vector<T, cuda_allocator<T> > (), mySize(0)
  {
    resize(vec.size());
#ifdef QMC_CUDA
    if (this->size() != 0) {
      cudaMemcpy (&((*this)[0]), &(vec[0]), vec.size()*sizeof(T),
		  cudaMemcpyDeviceToDevice);
      cudaError_t err = cudaGetLastError();
      if (err != cudaSuccess) {
	fprintf (stderr, "CUDA error in cuda_vector::copy constructor:\n  %s\n",
		 cudaGetErrorString(err));
	abort();
      }
    }
#endif
  }

  cuda_vector& 
  operator=(const cuda_vector<T> &vec)
  {
    if (this->size() != vec.size())
      resize(vec.size());
#ifdef QMC_CUDA
    cudaMemcpy (&((*this)[0]), &(vec[0]), this->size()*sizeof(T), 
		cudaMemcpyDeviceToDevice);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
      fprintf (stderr, 
	       "CUDA error in cuda_vector::operator=(cuda_vector):\n  %s\n",
	       cudaGetErrorString(err));
      cerr << "vec.size() = " << vec.size();
      abort();
    }
#endif
    return *this;
  }

  cuda_vector& 
  operator=(const std::vector<T,std::allocator<T> > &vec)
  {
    if (this->size() != vec.size())
      resize(vec.size());
#ifdef QMC_CUDA
    cudaMemcpy (&((*this)[0]), &(vec[0]), this->size()*sizeof(T), 
		cudaMemcpyHostToDevice);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
      fprintf (stderr, "CUDA error in cuda_vector::operator=(vector):\n  %s\n",
	       cudaGetErrorString(err));
      cerr << "size = " << vec.size() << endl;
      abort();
    }
#endif
    return *this;
  }

  cuda_vector& 
  operator=(const host_vector<T> &vec)
  {
    if (this->size() != vec.size()) 
      this->resize(vec.size());
#ifdef QMC_CUDA
    cudaMemcpy (&((*this)[0]), &(vec[0]), vec.size()*sizeof(T), 
		cudaMemcpyHostToDevice);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
      fprintf (stderr, 
	       "CUDA error in cuda_vector::operator=(host_vector):\n  %s\n",
	       cudaGetErrorString(err));
      cerr << "size = " << vec.size() << endl;
      cerr << "mysize = " << size() << endl;
      cerr << "capacity = " << this->capacity() << endl;
      abort();
    }
#endif
    return *this;
  }

  T* 
  data()
  { return &((*this)[0]); }

};




template<typename T>
class host_vector : public std::vector<T>
{
public:
  host_vector() : std::vector<T>()
  { }

  host_vector(const host_vector<T> &vec) :
    std::vector<T> (vec)
  {  }

  host_vector(int size) : 
    std::vector<T> (size)
  {  }

  host_vector(const cuda_vector<T> &vec) :
    std::vector<T> (vec.size())
  {
#ifdef QMC_CUDA
    cudaMemcpy (&((*this)[0]), &(vec[0]), this->size()*sizeof(T), 
		cudaMemcpyDeviceToHost);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
      fprintf (stderr, "CUDA error in host_vector::copy constructor():\n  %s\n",
	       cudaGetErrorString(err));
      abort();
    }
#endif
  }


  host_vector& 
  operator=(const host_vector<T> &vec)
  {
    if (this->size() != vec.size())
      resize(vec.size());
#ifdef QMC_CUDA
    cudaMemcpy (&((*this)[0]), &(vec[0]), this->size()*sizeof(T), 
		cudaMemcpyHostToDevice);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
      fprintf (stderr, "CUDA error in host_vector::operator=():\n  %s\n",
	       cudaGetErrorString(err));
      abort();
    }

#endif
    return *this;
  }

  host_vector& 
  operator=(const cuda_vector<T> &vec)
  {
    if (this->size() != vec.size())
      resize(vec.size());
#ifdef QMC_CUDA
    cudaMemcpy (&((*this)[0]), &(vec[0]), this->size()*sizeof(T), 
		cudaMemcpyDeviceToHost);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
      fprintf (stderr, "CUDA error in host_vector::operator=():\n  %s\n",
	       cudaGetErrorString(err));
      abort();
    }
#endif
    return *this;
  }
};

template<typename T>
cuda_vector<T>::cuda_vector(const host_vector<T> &vec) :
  std::vector<T, cuda_allocator<T> > (vec.size())
{
#ifdef QMC_CUDA
  cudaMemcpy (&((*this)[0]), &(vec[0]), this->size()*sizeof(T), 
	      cudaMemcpyDeviceToHost);
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    fprintf (stderr, "CUDA error in host_vector::operator=():\n  %s\n",
	     cudaGetErrorString(err));
    abort();
  }

#endif
  
}

#endif
