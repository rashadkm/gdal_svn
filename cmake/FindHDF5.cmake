# Find HDF5
# ~~~~~~~~
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Once run this will define:
#
# HDF5_FOUND        = system has HDF5 lib
# HDF5_LIBRARY      = full path to the HDF5 libraries
# HDF5_INCLUDE_DIRS = where to find headers
#

FIND_PATH(HDF5_INCLUDE_DIRS NAMES "hdf5.h" "hdf.h" PATHS
    /usr/include
    /usr/local/include
    "$ENV{LIB_DIR}/include"
    "$ENV{INCLUDE}"
    "$ENV{OSGEO4W_ROOT}/include"
    PATH_SUFFIXES hdf5
  )

SET(LIB_SEARCH_PATH 
    /usr/lib
    /usr/local/lib
    "$ENV{LIB_DIR}/lib"
    "$ENV{LIB}/lib"
    "$ENV{OSGEO4W_ROOT}/lib")

FIND_LIBRARY(HDF5_LIBRARY NAMES hdf5 PATHS LIB_SEARCH_PATH)

IF (HDF5_INCLUDE_DIRS AND HDF5_LIBRARY)
  SET(HDF5_FOUND TRUE)
ENDIF (HDF5_INCLUDE_DIRS AND HDF5_LIBRARY)

IF (HDF5_FOUND)
    MESSAGE(STATUS "Found HDF5: ${HDF5_LIBRARY}")
ELSE (HDF5_FOUND)
  IF (HDF5_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find HDF5")
  ENDIF (HDF5_FIND_REQUIRED)
ENDIF (HDF5_FOUND)