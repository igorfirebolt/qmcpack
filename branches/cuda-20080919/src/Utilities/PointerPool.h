//////////////////////////////////////////////////////////////////
// (c) Copyright 2008-  by Ken Esler
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

#ifndef POINTER_POOL_H
#define PIONTER_POOL_H

#include <vector>

template<typename T, typename CONT=vector<T,std::allocator<T> > >
class PointerPool 
{
public:
  typedef T* pointer;
  typedef CONT buffer_type;

  // Local data routines
  pointer getPointer (int index, buffer_type &buffer)
  { return &(buffer[offsets[index]]);  }

  void allocate (buffer_type &buffer)
  {
    buffer.resize(totalSize);
  }
  
  // Shared-data routines

  // Reserves size elements and returns the offset to the member 
  // in the buffer
  size_t reserveStorage (size_t size)
  {
    size_t off = totalSize;
    offsets.push_back(off);
    totalSize += size;
    return off;
  }

  void reset() {
    offsets.resize(0);
    totalSize = 0;
  }

protected:
  size_t totalSize;
  vector<size_t> offsets;
};

#endif
