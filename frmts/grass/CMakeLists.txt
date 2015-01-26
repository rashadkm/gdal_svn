# ******************************************************************************
# * Project:  CMake4GDAL
# * Purpose:  CMake build scripts
# * Author:   Alexander Lisovenko, alexander.lisovenko@gmail.com
# ******************************************************************************
# * Copyright (C) 2014 Alexander Lisovenko
# * Copyright (C) 2014 NextGIS
# * 
# * Permission is hereby granted, free of charge, to any person obtaining a
# * copy of this software and associated documentation files (the "Software"),
# * to deal in the Software without restriction, including without limitation
# * the rights to use, copy, modify, merge, publish, distribute, sublicense,
# * and/or sell copies of the Software, and to permit persons to whom the
# * Software is furnished to do so, subject to the following conditions:
# *
# * The above copyright notice and this permission notice shall be included
# * in all copies or substantial portions of the Software.
# *
# * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# * DEALINGS IN THE SOFTWARE.
# ******************************************************************************

cmake_minimum_required (VERSION 2.8.10)
set(LIB_NAME "frmts_grass")
project ("lib${LIB_NAME}")

find_package(GRASS REQUIRED)
if(GRASS_FOUND)
    include_directories(${GRASS_INCLUDE_DIR})
    add_definitions(-DHAVE_grass)
    set(GDAL_HAVE_GRASS ON CACHE INTERNAL "GDAL_HAVE_GRASS")
    add_definitions(-DGRASS_GISBASE="${GRASS_HOME}")
else(GRASS_FOUND)
    MESSAGE(FATAL_ERROR "Could not find GRASS")
endif(GRASS_FOUND)

find_package(PROJ REQUIRED)
if(PROJ_FOUND)
    include_directories(${PROJ_INCLUDE_DIR})
endif(PROJ_FOUND)

set(LIB_HHEADERS ${LIB_INST_HEADERS}
)

set(LIB_CSOURCES
)

IF(GRASS_MAJOR_VERSION LESS 7)
        set(LIB_CSOURCES ${LIB_CSOURCES} grass57dataset.cpp)
ELSE(GRASS_MAJOR_VERSION LESS 7)
        set(LIB_CSOURCES ${LIB_CSOURCES} grassdataset.cpp)
ENDIF(GRASS_MAJOR_VERSION LESS 7 )

set(GDAL_TARGET_OBJECTS ${GDAL_TARGET_OBJECTS} $<TARGET_OBJECTS:${LIB_NAME}> PARENT_SCOPE)
add_library(${LIB_NAME} OBJECT ${LIB_HHEADERS} ${LIB_CSOURCES})

if(GRASS_FOUND AND PROJ_FOUND)
		set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB} ${GRASS_LIBRARIES} ${PROJ_LIBRARY} PARENT_SCOPE)
endif(GRASS_FOUND AND PROJ_FOUND)
