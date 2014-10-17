# Find HDF4
# ~~~~~~~~
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Once run this will define:
#
# HDF4_FOUND        = system has HDF4 lib
# HDF4_LIBRARIES      = full path to the HDF4 libraries
# HDF4_INCLUDE_DIRS = where to find headers
#

FIND_PATH(HDF4_INCLUDE_DIRS "hdf.h" PATHS
    /usr/include
    /usr/local/include
    "$ENV{LIB_DIR}/include"
    "$ENV{INCLUDE}"
    "$ENV{OSGEO4W_ROOT}/include"
    PATH_SUFFIXES hdf
  )

SET(LIB_SEARCH_PATH 
    /usr/lib
    /usr/local/lib
    "$ENV{LIB_DIR}/lib"
    "$ENV{LIB}/lib"
    "$ENV{OSGEO4W_ROOT}/lib")

FIND_LIBRARY(HDF4_hdf_LIBRARY NAMES hdf df dfalt PATHS LIB_SEARCH_PATH)
FIND_LIBRARY(HDF4_mfhdf_LIBRARY NAMES mfhdf mfhdfalt PATHS LIB_SEARCH_PATH)

SET(HDF4_LIBRARIES ${HDF4_hdf_LIBRARY} ${HDF4_mfhdf_LIBRARY})

IF (HDF4_INCLUDE_DIRS AND HDF4_LIBRARIES)
  SET(HDF4_FOUND TRUE)
ENDIF (HDF4_INCLUDE_DIRS AND HDF4_LIBRARIES)

IF (HDF4_FOUND)
    MESSAGE(STATUS "Found HDF4: ${HDF4_LIBRARIES}")
ELSE (HDF4_FOUND)
  IF (HDF4_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find HDF4")
  ENDIF (HDF4_FIND_REQUIRED)
ENDIF (HDF4_FOUND)
