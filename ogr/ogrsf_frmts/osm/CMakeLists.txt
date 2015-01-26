# ******************************************************************************
# * Project:  CMake4GDAL
# * Purpose:  CMake build scripts
# * Author: Alexander Lisovenko, alexander.lisovenko@gmail.com
# ******************************************************************************
# * Copyright (C) 2014 Alexander Lisovenko
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

set(LIB_NAME "ogrsf_osm")
project ("lib${LIB_NAME}")

if(OGR_ENABLE_SQLITE)
    find_package(Sqlite3 REQUIRED)
    if(SQLITE3_FOUND)
        include_directories(${SQLITE3_INCLUDE_DIR})
        add_definitions(-DHAVE_SQLITE)
        
        if(HAS_COLUMN_METADATA)
            add_definitions(-DSQLITE_HAS_COLUMN_METADATA)
        endif()
        
        set(GDAL_HAVE_SQLITE ON CACHE INTERNAL "GDAL_HAVE_SQLITE")
    else(SQLITE3_FOUND)
        MESSAGE(FATAL_ERROR "Could not find Sqlite3")
    endif(SQLITE3_FOUND)

    include_directories(${GDAL_ROOT_SOURCE_DIR}/ogr/ogrsf_frmts/sqlite)
    include_directories(${GDAL_ROOT_SOURCE_DIR}/ogr/ogrsf_frmts/generic)

    set(LIB_HHEADERS
        gpb.h
        ogr_osm.h
        osm_parser.h
    )

    set(LIB_CSOURCES
        ogrosmdatasource.cpp
        ogrosmdriver.cpp
        ogrosmlayer.cpp
        osm_parser.cpp
    )

    set(GDAL_TARGET_OBJECTS ${GDAL_TARGET_OBJECTS} $<TARGET_OBJECTS:${LIB_NAME}> PARENT_SCOPE)
    add_library(${LIB_NAME} OBJECT ${LIB_HHEADERS} ${LIB_CSOURCES})

    if(SQLITE3_FOUND)
        set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB} ${SQLITE3_LIBRARY} PARENT_SCOPE)
    endif(SQLITE3_FOUND)
else(OGR_ENABLE_SQLITE)
    MESSAGE(FATAL_ERROR "Need SQLITE driver")
endif(OGR_ENABLE_SQLITE)