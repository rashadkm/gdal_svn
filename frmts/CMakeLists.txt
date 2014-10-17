# ******************************************************************************
# * Project:  CMake4GDAL
# * Purpose:  CMake build scripts
# * Author:   Dmitry Baryshnikov (aka Bishop), polimax@mail.ru
# *           Alexandr Lisovenko, alexander.lisovenko@gmail.com
# ******************************************************************************
# * Copyright (C) 2012,2014 Dmitry Baryshnikov
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

set(LIB_NAME "frmts")
project ("lib${LIB_NAME}")

macro(add_frmt_subdir frmt_name)
    add_definitions(-DFRMT_${frmt_name})
    add_subdirectory(${frmt_name})
    set(GDAL_FORMATS ${GDAL_FORMATS} " ${frmt_name}")
endmacro()

macro(add_optional_frmt_subdir frmt_name optstring optval)
    string(TOUPPER ${frmt_name} frmt_name_upper)
    set(optname GDAL_ENABLE_FRMT_${frmt_name_upper})
    option(${optname} "Set ON to build ${optstring} driver" ${optval})
    if(${optname})
        add_frmt_subdir(${frmt_name})
    endif()
endmacro()

include(CMakeDependentOption)

include_directories(${GDAL_ROOT_SOURCE_DIR}/gcore)
include_directories(${GDAL_ROOT_SOURCE_DIR}/frmts)
include_directories(${GDAL_ROOT_SOURCE_DIR}/ogr)
include_directories(${GDAL_ROOT_SOURCE_DIR}/ogr/ogrsf_frmts)

set(LIB_CSOURCES
	gdalallregister.cpp
)

add_frmt_subdir(gtiff)
add_frmt_subdir(hfa)
add_frmt_subdir(vrt)
add_frmt_subdir(raw)

#depended by s57 ogr driver
CMAKE_DEPENDENT_OPTION(GDAL_ENABLE_FRMT_ISO8211 "Set ON to build iso8211 driver" ON "OGR_ENABLE_S57" OFF)
if(GDAL_ENABLE_FRMT_ISO8211)
    add_frmt_subdir(iso8211)
endif(GDAL_ENABLE_FRMT_ISO8211)

option(GDAL_ENABLE_FRMT_BMP "Set ON to build bmp driver" ON)
if(GDAL_ENABLE_FRMT_BMP)
    add_frmt_subdir(bmp)
endif(GDAL_ENABLE_FRMT_BMP)

option(GDAL_ENABLE_FRMT_DIMAP "Set ON to build dimap driver" ON)
if(GDAL_ENABLE_FRMT_DIMAP)
    add_frmt_subdir(dimap)
endif(GDAL_ENABLE_FRMT_DIMAP)

option(GDAL_ENABLE_FRMT_MEM "Set ON to build memory raster driver" ON)
if(GDAL_ENABLE_FRMT_MEM)
    add_frmt_subdir(mem)
endif(GDAL_ENABLE_FRMT_MEM)

# need by apps
# option(GDAL_ENABLE_FRMT_VRT "Set ON to build virtual raster driver" OFF)
# if(GDAL_ENABLE_FRMT_VRT)
	# add_definitions(-DFRMT_vrt)
	# add_subdirectory(vrt)
# endif(GDAL_ENABLE_FRMT_VRT)

option(GDAL_ENABLE_FRMT_SAGA "Set ON to build saga raster driver" ON)
if(GDAL_ENABLE_FRMT_SAGA)
    add_frmt_subdir(saga)
endif(GDAL_ENABLE_FRMT_SAGA)

option(GDAL_ENABLE_FRMT_JPEG "Set ON to build jpeg driver" ON)
if(GDAL_ENABLE_FRMT_JPEG)
    add_frmt_subdir(jpeg)
endif(GDAL_ENABLE_FRMT_JPEG)

option(GDAL_ENABLE_FRMT_PNG "Set ON to build png driver" ON)
if(GDAL_ENABLE_FRMT_PNG)
    add_frmt_subdir(png)
endif(GDAL_ENABLE_FRMT_PNG)

option(GDAL_ENABLE_FRMT_GIF "Set ON to build gif driver" ON)
if(GDAL_ENABLE_FRMT_GIF)
    add_frmt_subdir(gif)
endif(GDAL_ENABLE_FRMT_GIF)

option(GDAL_ENABLE_FRMT_TIL "Set ON to build til driver" ON)
if(GDAL_ENABLE_FRMT_TIL)
    add_frmt_subdir(til)
endif(GDAL_ENABLE_FRMT_TIL)

option(GDAL_ENABLE_FRMT_OZI "Set ON to build ozi driver" ON)
if(GDAL_ENABLE_FRMT_OZI)
    add_frmt_subdir(ozi)
endif(GDAL_ENABLE_FRMT_OZI)

option(GDAL_ENABLE_FRMT_OZIMAP "Set ON to build ozi map driver" ON)
if(GDAL_ENABLE_FRMT_OZIMAP)
    add_frmt_subdir(map)
endif(GDAL_ENABLE_FRMT_OZIMAP)

option(GDAL_ENABLE_FRMT_POSTGISRASTER "Set ON to build postgis raster driver" OFF)
if(GDAL_ENABLE_FRMT_POSTGISRASTER)
    add_frmt_subdir(postgisraster)
endif(GDAL_ENABLE_FRMT_POSTGISRASTER)

option(GDAL_ENABLE_FRMT_ECW  "Set ON to build ERDAS Compressed Wavelets (.ecw) raster driver" OFF)
if(GDAL_ENABLE_FRMT_ECW)
    add_frmt_subdir(ecw)
endif(GDAL_ENABLE_FRMT_ECW)

option(GDAL_ENABLE_FRMT_GEORASTER  "Set ON to build Oracle Spatial GeoRaster raster driver" OFF)
if(GDAL_ENABLE_FRMT_GEORASTER)
    add_frmt_subdir(georaster)
endif(GDAL_ENABLE_FRMT_GEORASTER)

option(GDAL_ENABLE_FRMT_AAIGRID  "Set ON to build Arc/Info ASCII Grid raster driver" ON)
if(GDAL_ENABLE_FRMT_AAIGRID)
	add_definitions(-DFRMT_aaigrid)
	add_subdirectory(aaigrid)
    set(GDAL_FORMATS ${GDAL_FORMATS} " AAIGrid")
endif(GDAL_ENABLE_FRMT_AAIGRID)

CMAKE_DEPENDENT_OPTION(GDAL_ENABLE_FRMT_AIG "Set ON to build Arc/Info Binary Grid (.adf) raster driver" ON "OGR_ENABLE_AVC" OFF)
if(GDAL_ENABLE_FRMT_AIG)
	add_definitions(-DFRMT_aigrid)
	add_subdirectory(aigrid)
    set(GDAL_FORMATS ${GDAL_FORMATS} " AIG")
endif(GDAL_ENABLE_FRMT_AIG)

option(GDAL_ENABLE_FRMT_PDS  "Set ON to build NASA Planetary Data System raster driver" ON)
if(GDAL_ENABLE_FRMT_PDS )
	add_definitions(-DFRMT_pds)
	add_subdirectory(pds)
    set(GDAL_FORMATS ${GDAL_FORMATS} " PDS")
endif(GDAL_ENABLE_FRMT_PDS )

CMAKE_DEPENDENT_OPTION(GDAL_ENABLE_FRMT_HDF4 "Set ON to build Hierarchical Data Format Release 4 (HDF4) raster driver" OFF "GDAL_ENABLE_FRMT_PDS" OFF)
if(GDAL_ENABLE_FRMT_HDF4 )
	add_definitions(-DFRMT_hdf4)
	add_subdirectory(hdf4)
    set(GDAL_FORMATS ${GDAL_FORMATS} " HDF4")
endif(GDAL_ENABLE_FRMT_HDF4 )

option(GDAL_ENABLE_FRMT_HDF5  "Set ON to build Hierarchical Data Format Release 5 (HDF5) raster driver" OFF)
if(GDAL_ENABLE_FRMT_HDF5 )
	add_definitions(-DFRMT_hdf5)
	add_subdirectory(hdf5)
    set(GDAL_FORMATS ${GDAL_FORMATS} " HDF5")
endif(GDAL_ENABLE_FRMT_HDF5 )

option(GDAL_ENABLE_FRMT_E00GRID   "Set ON to build Arc/Info Export E00 GRID raster driver" ON)
if(GDAL_ENABLE_FRMT_E00GRID )
	add_definitions(-DFRMT_e00grid)
	add_subdirectory(e00grid)
    set(GDAL_FORMATS ${GDAL_FORMATS} " E00GRID")
endif(GDAL_ENABLE_FRMT_E00GRID )

option(GDAL_ENABLE_FRMT_JDEM   "Set ON to build Arc/Info Export E00 GRID raster driver" ON)
if(GDAL_ENABLE_FRMT_JDEM )
	add_definitions(-DFRMT_jdem)
	add_subdirectory(jdem)
    set(GDAL_FORMATS ${GDAL_FORMATS} " JDEM")
endif(GDAL_ENABLE_FRMT_JDEM )

option(GDAL_ENABLE_FRMT_PDF   "Set ON to build Geospatial PDF raster driver" ON)
if(GDAL_ENABLE_FRMT_PDF )
	add_definitions(-DFRMT_pdf)
	add_subdirectory(pdf)
    set(GDAL_FORMATS ${GDAL_FORMATS} " PDF")
endif(GDAL_ENABLE_FRMT_PDF )

option(GDAL_ENABLE_FRMT_MrSID   "Set ON to build Multi-resolution Seamless Image Database raster driver" OFF)
if(GDAL_ENABLE_FRMT_MrSID )
	add_definitions(-DFRMT_mrsid)
	add_subdirectory(mrsid)
    set(GDAL_FORMATS ${GDAL_FORMATS} " MrSID")
endif(GDAL_ENABLE_FRMT_MrSID )

option(GDAL_ENABLE_FRMT_KMLSUPEROVERLAY   "Set ON to build KMLSUPEROVERLAY raster driver" ON)
if(GDAL_ENABLE_FRMT_KMLSUPEROVERLAY )
	add_definitions(-DFRMT_kmlsuperoverlay)
	add_subdirectory(kmlsuperoverlay)
    set(GDAL_FORMATS ${GDAL_FORMATS} " KMLSUPEROVERLAY")
endif(GDAL_ENABLE_FRMT_KMLSUPEROVERLAY )

option(GDAL_ENABLE_FRMT_L1B   "Set ON to build L1B raster driver" ON)
if(GDAL_ENABLE_FRMT_L1B )
	add_definitions(-DFRMT_l1b)
	add_subdirectory(l1b)
    set(GDAL_FORMATS ${GDAL_FORMATS} " L1B")
endif(GDAL_ENABLE_FRMT_L1B )

option(GDAL_ENABLE_FRMT_NGSGEOID   "Set ON to build NOAA NGS Geoid Height Grids raster driver" ON)
if(GDAL_ENABLE_FRMT_NGSGEOID )
	add_definitions(-DFRMT_ngsgeoid)
	add_subdirectory(ngsgeoid)
    set(GDAL_FORMATS ${GDAL_FORMATS} " NGSGEOID")
endif(GDAL_ENABLE_FRMT_NGSGEOID )

option(GDAL_ENABLE_FRMT_NITF   "Set ON to build NITF (.ntf, .nsf, .gn?, .hr?, .ja?, .jg?, .jn?, .lf?, .on?, .tl?, .tp?, etc.) raster driver" ON)
if(GDAL_ENABLE_FRMT_NITF )
	add_definitions(-DFRMT_nitf)
	add_subdirectory(nitf)
    set(GDAL_FORMATS ${GDAL_FORMATS} " NITF")
endif(GDAL_ENABLE_FRMT_NITF )

option(GDAL_ENABLE_FRMT_JP2OpenJPEG "Set ON to build JP2OpenJPEG raster driver" OFF)
if(GDAL_ENABLE_FRMT_JP2OpenJPEG)
	add_definitions(-DFRMT_openjpeg)
	add_subdirectory(openjpeg)
    set(GDAL_FORMATS ${GDAL_FORMATS} " JP2OpenJPEG")
endif(GDAL_ENABLE_FRMT_JP2OpenJPEG)

#option(GDAL_ENABLE_FRMT_JPEG2000 "Set ON to build JPEG2000 raster driver" OFF)
#if(GDAL_ENABLE_FRMT_JPEG2000)
#	add_definitions(-DFRMT_jpeg2000)
#	add_subdirectory(jpeg2000)
#    set(GDAL_FORMATS ${GDAL_FORMATS} " JPEG2000")
#endif(GDAL_ENABLE_FRMT_JPEG2000)

option(GDAL_ENABLE_FRMT_ELAS "Set ON to build Earth Resources Laboratory Applications Software raster driver" ON)
if(GDAL_ENABLE_FRMT_ELAS)
    add_frmt_subdir(elas)
endif(GDAL_ENABLE_FRMT_ELAS)

option(GDAL_ENABLE_FRMT_CEOS "Set ON to build CEOS raster driver" ON)
if(GDAL_ENABLE_FRMT_CEOS)
    add_frmt_subdir(ceos)
endif(GDAL_ENABLE_FRMT_CEOS)

option(GDAL_ENABLE_FRMT_CEOS2 "Set ON to build SAR CEOS raster driver" ON)
if(GDAL_ENABLE_FRMT_CEOS2)
    add_frmt_subdir(ceos2)
endif(GDAL_ENABLE_FRMT_CEOS2)

option(GDAL_ENABLE_FRMT_JAXAPALSAR "Set ON to build JAXA PALSAR Product Reader raster driver" ON)
if(GDAL_ENABLE_FRMT_JAXAPALSAR)
    add_frmt_subdir(jaxapalsar)
endif(GDAL_ENABLE_FRMT_JAXAPALSAR)

option(GDAL_ENABLE_FRMT_GFF "Set ON to build Ground-based SAR Applications Testbed File Format raster driver" ON)
if(GDAL_ENABLE_FRMT_GFF)
    add_frmt_subdir(gff)
endif(GDAL_ENABLE_FRMT_GFF)

option(GDAL_ENABLE_FRMT_BSB "Set ON to build Maptech/NOAA BSB Nautical Chart Format driver" ON)
if(GDAL_ENABLE_FRMT_BSB)
    add_frmt_subdir(bsb) 
endif(GDAL_ENABLE_FRMT_BSB)   

option(GDAL_ENABLE_FRMT_DTED "Set ON to build Military Elevation Data driver" ON)
if(GDAL_ENABLE_FRMT_DTED)
    add_frmt_subdir(dted)
endif(GDAL_ENABLE_FRMT_DTED)       
    
option(GDAL_ENABLE_FRMT_ENVISAT "Set ON to build Envisat Image Product driver" ON)
if(GDAL_ENABLE_FRMT_ENVISAT)
    add_frmt_subdir(envisat)
endif(GDAL_ENABLE_FRMT_ENVISAT)    
    
option(GDAL_ENABLE_FRMT_ERS "Set ON to build ERMapper driver" ON)
if(GDAL_ENABLE_FRMT_ERS)  
    add_frmt_subdir(ers)
endif(GDAL_ENABLE_FRMT_ERS)     
 
option(GDAL_ENABLE_FRMT_SDTS "Set ON to build USGS SDTS DEM driver" ON)
if(GDAL_ENABLE_FRMT_SDTS)   
    add_frmt_subdir(sdts)
endif(GDAL_ENABLE_FRMT_SDTS)     
    
option(GDAL_ENABLE_FRMT_XPM "Set ON to build X11 Pixmap driver" ON)
if(GDAL_ENABLE_FRMT_XPM)   
    add_frmt_subdir(xpm)
endif(GDAL_ENABLE_FRMT_XPM)  

CMAKE_DEPENDENT_OPTION(GDAL_ENABLE_FRMT_WCS "Set ON to build OGC Web Coverage Service driver" OFF "GDAL_USE_CURL" OFF)
if(GDAL_ENABLE_FRMT_WCS)
    add_frmt_subdir(wcs)
endif(GDAL_ENABLE_FRMT_WCS)

CMAKE_DEPENDENT_OPTION(GDAL_ENABLE_FRMT_WMS "Set ON to build wms driver" OFF "GDAL_USE_CURL" OFF)
#option(GDAL_ENABLE_FRMT_WMS "Set ON to build wms driver" OFF)
if(GDAL_ENABLE_FRMT_WMS)
    add_frmt_subdir(wms)
endif(GDAL_ENABLE_FRMT_WMS)

add_optional_frmt_subdir(airsar "AIRSAR Polarimetric" ON)
add_optional_frmt_subdir(rs2 "RadarSat2 XML" ON)
add_optional_frmt_subdir(ilwis "ILWIS Raster Map" ON)
add_optional_frmt_subdir(sgi "SGI Image Format" ON)
add_optional_frmt_subdir(srtmhgt "SRTM HGT Format" ON)
add_optional_frmt_subdir(leveller "Daylon Leveller Heightfield" ON)
add_optional_frmt_subdir(terragen "Terragen Heightfield" ON)
add_optional_frmt_subdir(fit "FIT" ON)
add_optional_frmt_subdir(rmf "Raster Matrix Format" ON)
add_optional_frmt_subdir(gsg "Golden Software" ON)
add_optional_frmt_subdir(cosar "TerraSAR-X Complex SAR Data Product" ON)
add_optional_frmt_subdir(tsx "TerraSAR-X Product" ON)
add_optional_frmt_subdir(coasp "DRDC COASP SAR Processor Raster" ON)
add_optional_frmt_subdir(r "R Object Data Store" ON)
add_optional_frmt_subdir(usgsdem "USGS ASCII DEM / CDED" ON)
add_optional_frmt_subdir(gxf "GXF - Grid eXchange File" ON)
add_optional_frmt_subdir(northwood "Northwood/VerticalMapper" ON)
add_optional_frmt_subdir(adrg "ADRG/ARC Digitilized Raster Graphics" ON)
add_optional_frmt_subdir(blx "Magellan BLX Topo" ON)
add_optional_frmt_subdir(xyz "ASCII Gridded XYZ" ON)
add_optional_frmt_subdir(hf2 "HF2/HFZ heightfield raster" ON)
add_optional_frmt_subdir(ctg "USGS LULC Composite Theme Grid" ON)
add_optional_frmt_subdir(zmap "ZMap Plus Grid" ON)
add_optional_frmt_subdir(iris "IRIS" ON)

set(GDAL_FORMATS ${GDAL_FORMATS} PARENT_SCOPE)

set(GDAL_TARGET_OBJECTS ${GDAL_TARGET_OBJECTS} $<TARGET_OBJECTS:${LIB_NAME}> PARENT_SCOPE)
set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB} PARENT_SCOPE)
add_library(${LIB_NAME} OBJECT ${LIB_CSOURCES})

