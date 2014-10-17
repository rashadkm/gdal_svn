# ******************************************************************************
# * Project:  CMake4GDAL
# * Purpose:  CMake build scripts
# * Author: Alexander Lisovenko, alexander.lisovenko@gmail.com
# ******************************************************************************
# * Copyright (C) 2014 NextGIS <info@nextgis.ru>
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

set(LIB_NAME "rsmdreader")
project ("lib${LIB_NAME}")

include_directories(${GDAL_ROOT_SOURCE_DIR}/gcore)

set(LIB_HHEADERS
    utils.h
    remote_sensing_metadata.h
    rsmd_reader.h
    reader_abstract.h
    reader_digital_globe.h
    reader_orb_view.h
    reader_pleiades.h
    reader_geo_eye.h
    reader_kompsat.h
    reader_rdk1.h
    reader_spot.h
    reader_alos.h
    reader_eros.h
    reader_formosat.h
    reader_landsat.h
    reader_rapid_eye.h
)

set(LIB_CSOURCES
    utils.cpp
    rsmd_reader.cpp
    reader_digital_globe.cpp
    reader_orb_view.cpp
    reader_pleiades.cpp
    reader_geo_eye.cpp
    reader_kompsat.cpp
    reader_rdk1.cpp
    reader_spot.cpp
    reader_alos.cpp
    reader_eros.cpp
    reader_formosat.cpp
    reader_landsat.cpp
    reader_rapid_eye.cpp
)

set(GDAL_TARGET_OBJECTS ${GDAL_TARGET_OBJECTS} $<TARGET_OBJECTS:${LIB_NAME}> PARENT_SCOPE)
add_library(${LIB_NAME} OBJECT ${LIB_CSOURCES} ${LIB_HHEADERS} )
