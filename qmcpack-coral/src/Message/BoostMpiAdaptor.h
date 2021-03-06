#ifndef QMCPLUSPLUS_COLLECTIVE_OPERATIONS_H
#define QMCPLUSPLUS_COLLECTIVE_OPERATIONS_H

/** allreduce of single **/
#define QMCPP_ALLREDUCE(CppType, MPIType)                \
template<>  inline void                                  \
Communicate::allreduce(CppType& g)                       \
{                                                        \
  CppType gt(g);                                         \
  MPI_Allreduce(&(gt), &(g), 1, MPIType, MPI_SUM, myMPI);\
}

QMCPP_ALLREDUCE(double,MPI_DOUBLE);

QMCPP_ALLREDUCE(int,MPI_INT);

/** allreduce of container **/
#define QMCPP_ALLREDUCE_C(Container,MPIType)               \
template<>  inline void                                    \
Communicate::allreduce(Container& g)                       \
{                                                          \
  Container gt(g)                                          \
  MPI_Allreduce(&(g[0]), &(gt[0]), g.size(), MPIType, MPI_SUM, myMPI);\
  g=gt;                                                               \
}

QMCPP_ALLREDUCE_C(vector<double>,MPI_DOUBLE);

QMCPP_ALLREDUCE_C(vector<int>,MPI_INT);

QMCPP_ALLREDUCE_C(APPNAMESPACE::Matrix<double>,MPI_DOUBLE);

/** gather operations **/
#define QMCPP_GATHER(CONTAINER,CppType, MPIType)                                     \
template<>                                                                           \
inline void                                                                          \
Communicate::gather(CONTAINER< CppType >&sb, CONTAINER< CppType >& rb, int dest)     \
{                                                                                    \
  MPI_Gather(&(sb[0]),sb.size(),MPI_UNSIGNED, &(rb[0]),sb.size(),MPIType,dest,myMPI);\
}

QMCPP_GATHER(vector,uint32_t,MPI_UNSIGNED);
QMCPP_GATHER(vector,double,MPI_DOUBLE);
QMCPP_GATHER(vector,int,MPI_INT);

/** scatter operations **/
#define QMCPP_SCATTER(CONTAINER,CppType, MPIType)                                     \
template<>                                                                           \
inline void                                                                          \
Communicate::gather(CONTAINER< CppType >&sb, CONTAINER< CppType >& rb, int dest)     \
{                                                                                    \
  MPI_Scatter(&(sb[0]),sb.size(),MPI_UNSIGNED, &(rb[0]),sb.size(),MPIType,dest,myMPI);\
}

QMCPP_SCATTER(vector,uint32_t,MPI_UNSIGNED);
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 2458 $   $Date: 2008-02-20 10:45:51 -0500 (Wed, 20 Feb 2008) $
 * $Id: CommOperators.h 2458 2008-02-20 15:45:51Z jnkim $
 ***************************************************************************/
