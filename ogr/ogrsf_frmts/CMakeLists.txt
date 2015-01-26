# ******************************************************************************
# * Project:  CMake4GDAL
# * Purpose:  CMake build scripts
# * Author:   Dmitry Baryshnikov (aka Bishop), polimax@mail.ru
# ******************************************************************************
# * Copyright (C) 2012-2014 Dmitry Baryshnikov
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

set(LIB_NAME "ogrsf_frmts")
project ("lib${LIB_NAME}")

include_directories(${GDAL_ROOT_SOURCE_DIR}/gcore)
include_directories(${GDAL_ROOT_SOURCE_DIR}/ogr/ogrsf_frmts)  

set(LIB_HHEADERS
	ogr_attrind.h
	ogrsf_frmts.h
	generic/ogr_gensql.h
	generic/ogrlayerdecorator.h
	generic/ogrlayerpool.h
	generic/ogrunionlayer.h
	generic/ogrwarpedlayer.h	
)

set(LIB_CSOURCES
	generic/ogrsfdriverregistrar.cpp
	generic/ogrlayer.cpp
	generic/ogrdatasource.cpp
	generic/ogrsfdriver.cpp
	generic/ogrregisterall.cpp
	generic/ogr_gensql.cpp
	generic/ogr_attrind.cpp
	generic/ogr_miattrind.cpp
	generic/ogrwarpedlayer.cpp
	generic/ogrunionlayer.cpp
	generic/ogrlayerpool.cpp
	generic/ogrlayerdecorator.cpp	
)
#need to compile
#add_subdirectory(generic)

add_definitions(-DTAB_ENABLED)
add_subdirectory(mitab)

option(OGR_ENABLE_CSV "Set ON to build csv driver" ON)
if(OGR_ENABLE_CSV)
	add_definitions(-DCSV_ENABLED)
	add_subdirectory(csv)
endif()

option(OGR_ENABLE_DFX "Set ON to build dxf driver" ON)
if(OGR_ENABLE_DFX)
	add_definitions(-DDXF_ENABLED)
	add_subdirectory(dxf)
endif()

# NOTE For now it can not be optional
set(OGR_ENABLE_GEOJSON ON)
if(OGR_ENABLE_GEOJSON)
	add_definitions(-DGEOJSON_ENABLED)
	add_subdirectory(geojson)
endif()

if(GDAL_ENABLE_OGR)
	add_definitions(-DGML_ENABLED)
	add_subdirectory(gml)
	add_definitions(-DKML_ENABLED)
	add_subdirectory(kml)
else()
  option(OGR_ENABLE_GML "Set ON to build gml driver" ON)
  if(OGR_ENABLE_GML)
		add_definitions(-DGML_ENABLED)
		add_subdirectory(gml)
	endif()

  option(OGR_ENABLE_KML "Set ON to build kml driver" ON)
  if(OGR_ENABLE_KML)
		add_definitions(-DKML_ENABLED)
		add_subdirectory(kml)
	endif()
endif()

option(OGR_ENABLE_MEM "Set ON to build mem driver" ON)
if(OGR_ENABLE_MEM)
	add_definitions(-DMEM_ENABLED)
	add_subdirectory(mem)
endif()

option(OGR_ENABLE_VRT "Set ON to build vrt driver" ON)
if(OGR_ENABLE_VRT)
	add_definitions(-DVRT_ENABLED)
	add_subdirectory(vrt)
endif()

option(OGR_ENABLE_GPX "Set ON to build gpx driver" ON)
if(OGR_ENABLE_GPX)
	add_definitions(-DGPX_ENABLED)
	add_subdirectory(gpx)
endif()

option(OGR_ENABLE_SHP "Set ON to build ESRI shape-file driver" ON)
if(OGR_ENABLE_SHP)
	add_definitions(-DSHAPE_ENABLED)
	add_subdirectory(shape)
endif()

include(CMakeDependentOption)

CMAKE_DEPENDENT_OPTION(OGR_ENABLE_WFS "Set ON to build wfs driver" OFF "GDAL_USE_CURL" OFF)
#option(OGR_ENABLE_WFS "Set ON to build WFS driver" OFF)
if(OGR_ENABLE_WFS)
	add_definitions(-DWFS_ENABLED)
	add_subdirectory(wfs)
endif()

option(OGR_ENABLE_PG "Set ON to build PostGIS driver" OFF)
if(OGR_ENABLE_PG)
	add_definitions(-DPG_ENABLED)
	add_subdirectory(pg)
endif()

option(OGR_ENABLE_SQLITE "Set ON to build SpatialLite and GeoPackage drivers" OFF)
if(OGR_ENABLE_SQLITE)
	add_definitions(-DSQLITE_ENABLED)
	add_subdirectory(sqlite)
	add_subdirectory(gpkg)	
endif()

option(OGR_ENABLE_MYSQL "Set ON to build MySQL driver" OFF)
if(OGR_ENABLE_MYSQL)
	add_definitions(-DMYSQL_ENABLED)
	add_subdirectory(mysql)
endif()

option(OGR_ENABLE_LIBKML "Set ON to build libkml driver" OFF)
if(OGR_ENABLE_LIBKML)
	add_definitions(-DLIBKML_ENABLED)
	add_subdirectory(libkml)
endif()

option(OGR_ENABLE_SXF "Set ON to build sxf driver" ON)
if(OGR_ENABLE_SXF)
    add_definitions(-DSXF_ENABLED)
    add_subdirectory(sxf)
endif()

option(OGR_ENABLE_S57 "Set ON to build s57 driver" ON)
if(OGR_ENABLE_S57)
    add_definitions(-DS57_ENABLED)
    add_subdirectory(s57)
endif()

option(OGR_ENABLE_OGDB "Set ON to build OpenGDB driver" ON)
if(OGR_ENABLE_OGDB)
    add_definitions(-DOPENFILEGDB_ENABLED)
    add_subdirectory(openfilegdb)
endif()


option(OGR_ENABLE_XLS "Set ON to build XLS - MS Excel format driver" OFF)
if(OGR_ENABLE_XLS)
    add_definitions(-DFREEXL_ENABLED)
    add_subdirectory(xls)
endif()

option(OGR_ENABLE_XLSX "Set ON to build XLSX - MS Office Open XML spreadsheet driver" OFF)
if(OGR_ENABLE_XLSX)
    add_definitions(-DXLSX_ENABLED)
    add_subdirectory(xlsx)
endif()

option(OGR_ENABLE_AVC "Set ON to build Arc/Info Binary Coverage driver" OFF)
if(OGR_ENABLE_AVC)
	add_definitions(-DAVC_ENABLED)
	add_subdirectory(avc)
endif()

option(OGR_ENABLE_ODS "Set ON to build ODS - Open Document Spreadsheet format driver" OFF)
if(OGR_ENABLE_ODS)
    add_definitions(-DODS_ENABLED)
    add_subdirectory(ods)
endif()

option(OGR_ENABLE_GME "Set ON to build Google Maps Engine format driver" OFF)
if(OGR_ENABLE_GME)
    add_definitions(-DGME_ENABLED)
    add_subdirectory(gme)
endif()

option(OGR_ENABLE_GMT "Set ON to build GMT format driver" ON)
if(OGR_ENABLE_GMT)
    add_definitions(-DGMT_ENABLED)
    add_subdirectory(gmt)
endif()

option(OGR_ENABLE_GPSTrackMaker "Set ON to build GPSTrackMaker format driver" ON)
if(OGR_ENABLE_GPSTrackMaker)
    add_definitions(-DGTM_ENABLED)
    add_subdirectory(gtm)
endif()

option(OGR_ENABLE_HTF "Set ON to build Hydrographic Transfer Format format driver" ON)
if(OGR_ENABLE_HTF)
    add_definitions(-DHTF_ENABLED)
    add_subdirectory(htf)
endif()

option(OGR_ENABLE_DGN "Set ON to build Hydrographic Transfer Format format driver" ON)
if(OGR_ENABLE_DGN)
    add_definitions(-DDGN_ENABLED)
    add_subdirectory(dgn)
endif()

option(OGR_ENABLE_PGDump "Set ON to build PostgreSQL SQL dump format driver" ON)
if(OGR_ENABLE_PGDump)
    add_definitions(-DPGDUMP_ENABLED)
    add_subdirectory(pgdump)
endif()

option(OGR_ENABLE_OCI "Set ON to build OCI - Oracle Spatial" OFF)
if(OGR_ENABLE_OCI)
    add_definitions(-DOCI_ENABLED)
    add_subdirectory(oci)
endif()

option(OGR_ENABLE_ODBC "Set ON to build ODBC - ODBC" OFF)
if(OGR_ENABLE_ODBC)
    add_definitions(-DODBC_ENABLED)
    add_subdirectory(odbc)
endif()

option(OGR_ENABLE_OSM "Set ON to build OSM - OpenStreetMap XML and PBF" OFF)
if(OGR_ENABLE_OSM)
    add_definitions(-DOSM_ENABLED)
    add_subdirectory(osm)
endif()

option(OGR_ENABLE_GRASS "Set ON to build GRASS - GRASS Vector Format" OFF)
if(OGR_ENABLE_GRASS)
    add_definitions(-DGRASS_ENABLED)
    add_subdirectory(grass)
endif()

set(GDAL_TARGET_OBJECTS ${GDAL_TARGET_OBJECTS} $<TARGET_OBJECTS:${LIB_NAME}> PARENT_SCOPE)
set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB} PARENT_SCOPE)
add_library(${LIB_NAME} OBJECT ${LIB_HHEADERS} ${LIB_CSOURCES})

install(FILES ${LIB_HHEADERS} DESTINATION include/gdal)

