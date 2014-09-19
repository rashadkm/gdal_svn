# ******************************************************************************
# * Project:  CMake4GDAL
# * Purpose:  CMake build scripts
# * Author: Alexander Lisovenko, alexander.lisovenko@gmail.com
# ******************************************************************************
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

set(LIB_NAME "rsmdreader")
project ("lib${LIB_NAME}")

set(LIB_HHEADERS
	CPLXML_utils.h
    remote_sensing_metadata.h
    rsmd_reader.h
    reader_abstract.h
    reader_digital_globe.h
    reader_pleiades.h
    reader_geo_eye.h
)

set(LIB_CSOURCES
    CPLXML_utils.cpp
    rsmd_reader.cpp
    reader_abstract.cpp
    reader_digital_globe.cpp
    reader_pleiades.cpp
    reader_geo_eye.cpp
)

set(GDAL_TARGET_OBJECTS ${GDAL_TARGET_OBJECTS} $<TARGET_OBJECTS:${LIB_NAME}> PARENT_SCOPE)
add_library(${LIB_NAME} OBJECT ${LIB_CSOURCES} ${LIB_HHEADERS} )
