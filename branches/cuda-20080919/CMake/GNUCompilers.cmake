#GNU compilers
IF(CMAKE_COMPILER_IS_GNUCXX) 
  ADD_DEFINITIONS(-Drestrict=__restrict__ -DADD_ -DINLINE_ALL=inline)
  SET(CMAKE_CXX_FLAGS "-O6 -ftemplate-depth-60 -Drestrict=__restrict__ -fstrict-aliasing -funroll-all-loops   -finline-limit=1000 -ffast-math -Wno-deprecated -msse3 -fopenmp")
  SET(CMAKE_C_FLAGS "-O3 -Drestrict=__restrict__ -fstrict-aliasing -funroll-all-loops   -finline-limit=1000 -ffast-math -std=gnu99 -fomit-frame-pointer ")

  SET(CMAKE_TRY_GNU_CC_FLAGS "-mmmx")
  CHECK_C_COMPILER_FLAG(${CMAKE_TRY_GNU_CC_FLAGS} GNU_CC_FLAGS)
  IF(GNU_CC_FLAGS)
    SET(HAVE_MMX 1)
  ENDIF(GNU_CC_FLAGS)

  SET(CMAKE_TRY_GNU_CC_FLAGS "-msse")
  CHECK_C_COMPILER_FLAG(${CMAKE_TRY_GNU_CC_FLAGS} GNU_CC_FLAGS)
  IF(GNU_CC_FLAGS)
    SET(HAVE_SSE 1)
  ENDIF(GNU_CC_FLAGS)

  SET(CMAKE_TRY_GNU_CXX_FLAGS "-msse2")
  CHECK_C_COMPILER_FLAG(${CMAKE_TRY_GNU_CC_FLAGS} GNU_CC_FLAGS)
  IF(GNU_CC_FLAGS)
    SET(HAVE_SSE2 1)
  ENDIF(GNU_CC_FLAGS)

  SET(CMAKE_TRY_GNU_CC_FLAGS "-msse3")
  CHECK_C_COMPILER_FLAG(${CMAKE_TRY_GNU_CC_FLAGS} GNU_CC_FLAGS)
  IF(GNU_CC_FLAGS)
    SET(HAVE_SSE3 1)
  ENDIF(GNU_CC_FLAGS)

  SET(CMAKE_TRY_GNU_CXX_FLAGS "-msse3")
  CHECK_C_COMPILER_FLAG(${CMAKE_TRY_GNU_CXX_FLAGS} GNU_CXX_FLAGS)
  IF(GNU_CC_FLAGS)
    SET(HAVE_SSE3 1)
  ENDIF(GNU_CC_FLAGS)

  SET(CMAKE_TRY_GNU_CC_FLAGS "-mssse3")
  CHECK_C_COMPILER_FLAG(${CMAKE_TRY_GNU_CC_FLAGS} GNU_CC_FLAGS)
  IF(GNU_CC_FLAGS)
    SET(HAVE_SSSE3 1)
  ENDIF(GNU_CC_FLAGS)

  #  SET(CMAKE_CXX_FLAGS "-O6 -ftemplate-depth-60 -Drestrict=__restrict__ -fstrict-aliasing -funroll-all-loops   -finline-limit=1000 -ffast-math -Wno-deprecated -pg")
  #  SET(CMAKE_CXX_FLAGS "-g -ftemplate-depth-60 -Drestrict=__restrict__ -fstrict-aliasing -Wno-deprecated")

  IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # SET(CMAKE_SHARED_LIBRARY_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CXX_FLAGS} -faltivec -framework Accelerate -bind_at_load")
    SET(F77 xlf)
    SET(F77FLAGS -O3)
  ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  #  SET(FORTRAN_LIBS "-lg2c")
    SET(F77 g77)
    SET(F77FLAGS  -funroll-loops -O3)
  ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  
  #  INCLUDE(${CMAKE_ROOT}/Modules/TestCXXAcceptsFlag.cmake)
  IF(QMC_OMP)
    SET(CMAKE_TRY_OPENMP_CXX_FLAGS "-fopenmp")
    CHECK_CXX_ACCEPTS_FLAG(${CMAKE_TRY_OPENMP_CXX_FLAGS} GNU_OPENMP_FLAGS)
    IF(GNU_OPENMP_FLAGS)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_TRY_OPENMP_CXX_FLAGS}")
      SET(ENABLE_OPENMP 1)
    ENDIF(GNU_OPENMP_FLAGS)
  ENDIF(QMC_OMP)

  IF(QMC_BUILD_STATIC)
    SET(CMAKE_CXX_LINK_FLAGS " -static")
  ENDIF(QMC_BUILD_STATIC)

  SET(CMAKE_CXX_FLAGS "$ENV{CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
  SET(CMAKE_C_FLAGS "$ENV{CC_FLAGS} ${CMAKE_C_FLAGS}")

ENDIF(CMAKE_COMPILER_IS_GNUCXX) 

IF(APPLE)
  INCLUDE_DIRECTORIES(/sw/include)
ENDIF(APPLE)
