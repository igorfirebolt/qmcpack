#CONFIGURE_FILE(${qmcpack_SOURCE_DIR}/src/Configuration.h.in
#               ${qmcpack_BINARY_DIR}/src/Configuration.h)
CONFIGURE_FILE(${qmcpack_SOURCE_DIR}/src/config.h.cmake.in
               ${qmcpack_BINARY_DIR}/src/config.h)

####################################
# create libqmcutil
####################################
SET(UTILITIES
  Utilities/RandRandom.cpp 
  Utilities/RandomGenerator.cpp 
  Utilities/OhmmsObject.cpp 
  Utilities/SpeciesSet.cpp 
  Utilities/SimpleParser.cpp 
  Utilities/OhmmsInform.cpp 
  Utilities/OhmmsInfo.cpp 
  Utilities/NewTimer.cpp
  Utilities/ProgressReportEngine.cpp
  OhmmsData/Libxml2Doc.cpp
  OhmmsApp/ProjectData.cpp
  OhmmsApp/RandomNumberControl.cpp
  Numerics/OhmmsBlas.cpp
  Platforms/sysutil.cpp
  QMCFactory/OneDimGridFactory.cpp
  Message/Communicate.cpp 
  Message/MPIObjectBase.cpp 
  Optimize/VariableSet.cpp
)

IF(QMC_CUDA)
   SET(UTILITIES ${UTILITIES} 
       Numerics/CUDA/cuda_inverse.cu 
       CUDA/gpu_vector.cpp)
ENDIF(QMC_CUDA)


##gsl library will be optional
#IF(HAVE_LIBGSL)
#  SET(UTILITIES ${UTILITIES} Optimize/GSLMinimize.cpp)
#ENDIF(HAVE_LIBGSL)
IF(QMC_CUDA)
  CUDA_ADD_LIBRARY(qmcutil ${UTILITIES})
ELSE(QMC_CUDA)
  ADD_LIBRARY(qmcutil ${UTILITIES})
ENDIF(QMC_CUDA)
#IF(QMC_BUILD_STATIC)
#  ADD_LIBRARY(qmcutil STATIC ${UTILITIES})
#ELSE(QMC_BUILD_STATIC)
#  ADD_LIBRARY(qmcutil SHARED ${UTILITIES})
#ENDIF(QMC_BUILD_STATIC)

####################################
# create liboompi
####################################
IF(HAVE_OOMPI)
  CONFIGURE_FILE(${qmcpack_SOURCE_DIR}/src/oompi-config.h.cmake.in 
                 ${qmcpack_BINARY_DIR}/src/oompi-config.h)
  INCLUDE_DIRECTORIES(${qmcpack_SOURCE_DIR}/src/OOMPI)
  SUBDIRS(OOMPI)
  LINK_LIBRARIES(oompi)
ENDIF(HAVE_OOMPI)


####################################
# create libqmcbase 
####################################
SET(PARTICLE
  Lattice/Uniform3DGridLayout.cpp 
  Particle/ParticleSet.cpp 
  Particle/ParticleSet.BC.cpp 
  Particle/MCWalkerConfiguration.cpp 
  Particle/Bead_ParticleSet.cpp 
  Particle/DistanceTable.cpp
  Particle/HDFWalkerInputManager.cpp
  LongRange/KContainer.cpp
  LongRange/StructFact.cpp
  LongRange/LPQHIBasis.cpp			      	
  LongRange/LRCoulombSingleton.cpp
)

IF(QMC_CUDA)
   SET(PARTICLE ${PARTICLE} Particle/accept_kernel.cu )
ENDIF(QMC_CUDA)



SET(PARTICLEIO
  ParticleTags.cpp 
  ParticleIO/ParticleLayoutIO.cpp 
  ParticleIO/XMLParticleIO.cpp 
  ParticleIO/ESHDFParticleParser.cpp
)

IF(HAVE_LIBHDF5)
  SET(PARTICLEIO ${PARTICLEIO}
      Particle/HDFWalkerIOEngine.cpp 
      Particle/HDFWalkerOutput.cpp
      Particle/HDFWalkerInput_0_0.cpp
      Particle/HDFWalkerInput_0_4.cpp
      ParticleIO/HDFParticleIO.cpp 
     )
ENDIF(HAVE_LIBHDF5)

IF(QMC_CUDA)
   CUDA_ADD_LIBRARY(qmcbase ${PARTICLE}  ${PARTICLEIO})	
ELSE(QMC_CUDA)
  ADD_LIBRARY(qmcbase ${PARTICLE}  ${PARTICLEIO})
ENDIF(QMC_CUDA)

#IF(QMC_BUILD_STATIC)
#  ADD_LIBRARY(qmcbase STATIC ${PARTICLE}  ${PARTICLEIO})
#ELSE(QMC_BUILD_STATIC)
#  ADD_LIBRARY(qmcbase SHARED ${PARTICLE}  ${PARTICLEIO})
#ENDIF(QMC_BUILD_STATIC)


SUBDIRS(QMCWaveFunctions)
SUBDIRS(QMCHamiltonians)
SUBDIRS(QMCDrivers)
SUBDIRS(QMCApp)
#SUBDIRS(einspline)

IF(BUILD_QMCTOOLS)
  SUBDIRS(QMCTools)
ENDIF(BUILD_QMCTOOLS)

IF(BUILD_SANDBOX)
  SUBDIRS(SandBox)
ENDIF(BUILD_SANDBOX)

#SUBDIRS(SQD)
