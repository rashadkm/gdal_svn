# ******************************************************************************
# * Project:  CMake4GDAL
# * Purpose:  CMake build scripts
# * Author:   Dmitry Baryshnikov (aka Bishop), polimax@mail.ru
# ******************************************************************************
# * Copyright (C) 2014 Dmitry Baryshnikov
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

set(LIB_NAME "ogrsf_xls")
project ("lib${LIB_NAME}")

if(GDAL_USE_CPL)
	add_definitions(-DUSE_CPL)
endif(GDAL_USE_CPL)

find_package(FreeXL REQUIRED)
if(FREEXL_FOUND)
	include_directories(${FREEXL_INCLUDE_DIR})
	add_definitions(-DHAVE_FREEXL)
	set(GDAL_HAVE_FREEXL ON CACHE INTERNAL "GDAL_HAVE_FREEXL")
endif(FREEXL_FOUND)

set(LIB_HHEADERS
    ogr_xls.h
)

set(LIB_CSOURCES
	ogrxlsdatasource.cpp
	ogrxlsdriver.cpp
	ogrxlslayer.cpp
)

set(GDAL_TARGET_OBJECTS ${GDAL_TARGET_OBJECTS} $<TARGET_OBJECTS:${LIB_NAME}> PARENT_SCOPE)
add_library(${LIB_NAME} OBJECT ${LIB_HHEADERS} ${LIB_CSOURCES})

if(FREEXL_FOUND)
	set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB} ${FREEXL_LIBRARIES} PARENT_SCOPE)
endif(FREEXL_FOUND)

