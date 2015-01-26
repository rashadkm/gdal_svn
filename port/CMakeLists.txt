# ******************************************************************************
# * Project:  CMake4GDAL
# * Purpose:  CMake build scripts
# * Author: Dmitry Baryshnikov (aka Bishop), polimax@mail.ru
# ******************************************************************************
# * Copyright (C) 2012,2013 Dmitry Baryshnikov
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

set(LIB_NAME "cpl")
project ("lib${LIB_NAME}")

#check Iconv
option(GDAL_USE_ICONV "Set ON to use libiconv" ON)
if(GDAL_USE_ICONV)
    find_package(Iconv REQUIRED)
    set(HAVE_ICONV ${ICONV_FOUND})
    set(GDAL_HAVE_ICONV ${ICONV_FOUND} PARENT_SCOPE)
    if(ICONV_SECOND_ARGUMENT_IS_CONST)
      set(ICONV_CONST "const")
    endif(ICONV_SECOND_ARGUMENT_IS_CONST)
endif(GDAL_USE_ICONV)

set(LIB_HHEADERS
    cplkeywordparser.h
    cpl_atomic_ops.h
    cpl_config_extras.h
    cpl_conv.h
    cpl_csv.h
    cpl_error.h
    cpl_hash_set.h
    cpl_http.h
    cpl_list.h
    cpl_minixml.h
    cpl_multiproc.h
    cpl_port.h
    cpl_quad_tree.h
    cpl_string.h
    cpl_time.h
    cpl_vsi.h
    cpl_vsi_virtual.h
    #cpl_win32ce_api.h
    #cpl_wince.h
    gdal_csv.h
    cpl_spawn.h
    cpl_progress.h
    cpl_virtualmem.h
    cpl_vsil_curl_priv.h
)

set(LIB_CSOURCES
    cpl_conv.cpp
    cpl_error.cpp 
    cpl_string.cpp      
    cplstring.cpp
    cplstringlist.cpp
    cpl_strtod.cpp  
    cpl_vsisimple.cpp 
    cplgetsymbol.cpp
    cpl_path.cpp 
    cpl_csv.cpp 
    cpl_findfile.cpp
    cpl_minixml.cpp 
    cpl_multiproc.cpp
    cpl_list.cpp 
    cpl_getexecpath.cpp
    cpl_vsil.cpp
    cpl_vsi_mem.cpp 
    cpl_http.cpp 
    cpl_hash_set.cpp 
    cplkeywordparser.cpp
    cpl_recode.cpp 
    cpl_recode_iconv.cpp 
    cpl_recode_stub.cpp 
    cpl_quad_tree.cpp 
    cpl_vsil_subfile.cpp     
    cpl_atomic_ops.cpp
    cpl_time.cpp
    cpl_vsil_stdout.cpp
    cpl_vsil_sparsefile.cpp 
    cpl_vsil_abstract_archive.cpp
    cpl_vsil_tar.cpp
    cpl_vsil_curl.cpp 
    cpl_vsil_curl_streaming.cpp 
    cpl_vsil_stdin.cpp 
    cpl_vsil_buffered_reader.cpp 
    cpl_vsil_cache.cpp 
    cpl_base64.cpp
	cpl_xml_validate.cpp
    cpl_spawn.cpp
    cpl_google_oauth2.cpp
    cpl_progress.cpp
    cpl_virtualmem.cpp
)

if(WIN32)
    set(LIB_CSOURCES ${LIB_CSOURCES} cpl_vsil_win32.cpp)
else(WIN32)
    set(LIB_CSOURCES ${LIB_CSOURCES} cpl_vsil_unix_stdio_64.cpp)
endif(WIN32)

set(LIB_HHEADERS ${LIB_HHEADERS}
	${GDAL_ROOT_BINARY_DIR}/port/cpl_config.h 
)

#check zip and internal zip
option(GDAL_USE_LIBZ "Set ON to use libz" ON)
if(GDAL_USE_LIBZ)
	option(GDAL_USE_LIBZ_INTERNAL "Set ON to use internal libz" OFF)
	add_definitions(-DHAVE_LIBZ -DZIP_SUPPORT)
	set(LIB_HHEADERS ${LIB_HHEADERS}
		cpl_minizip_ioapi.h
		cpl_minizip_unzip.h
		cpl_minizip_zip.h
	)
	
	set(LIB_CSOURCES ${LIB_CSOURCES}
		cpl_vsil_gzip.cpp 
		cpl_minizip_ioapi.cpp 
		cpl_minizip_unzip.cpp 
		cpl_minizip_zip.cpp 
	)	
	if(GDAL_USE_LIBZ_INTERNAL)
		include_directories(${GDAL_ROOT_SOURCE_DIR}/frmts/zlib)
		add_subdirectory(${GDAL_ROOT_SOURCE_DIR}/frmts/zlib ${GDAL_ROOT_BINARY_DIR}/frmts/zlib)
	else(GDAL_USE_LIBZ_INTERNAL)
		find_package(ZLIB REQUIRED)
		if(ZLIB_FOUND)
			include_directories(${ZLIB_INCLUDE_DIRS})
			add_definitions(-DHAVE_ZLIB_H -DHAVE_ZLIB)
			if(MSVC)
				add_definitions(-DZLIB_DLL)
			endif(MSVC)
		else(ZLIB_FOUND)	
			MESSAGE(FATAL_ERROR "Could not find ZLIB")
		endif(ZLIB_FOUND)		
	endif(GDAL_USE_LIBZ_INTERNAL)
endif(GDAL_USE_LIBZ)	

#check curl
option(GDAL_USE_CURL "Set ON to use libcurl" ON)
if(GDAL_USE_CURL)
	find_package(CURL REQUIRED)
	if(CURL_FOUND)
		include_directories(${CURL_INCLUDE_DIRS})
		add_definitions(-DHAVE_CURL)
		
		set(LIB_HHEADERS ${LIB_HHEADERS}
			cpl_vsil_curl_priv.h
		)
	
		set(LIB_CSOURCES ${LIB_CSOURCES}
			cpl_vsil_curl.cpp
			cpl_vsil_curl_streaming.cpp
		)
	endif(CURL_FOUND)
endif(GDAL_USE_CURL)	

#check Iconv
option(GDAL_USE_ICONV "Set ON to use libiconv" ON)
if(GDAL_USE_ICONV)
    include_directories(${ICONV_INCLUDE_DIR})
	add_definitions(-DHAVE_ICONV -DLIBICONV_PLUG)
endif(GDAL_USE_ICONV)

#check odbc
option(GDAL_USE_ODBC "Set ON to use odbc" OFF)
if(GDAL_USE_ODBC)
	set(LIB_HHEADERS ${LIB_HHEADERS}
		cpl_odbc.h
	)
	
	set(LIB_CSOURCES ${LIB_CSOURCES}
		cpl_odbc.cpp  
	)
    
    if(WIN32)
        set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB} odbc32.lib odbcbcp.lib odbccp32.lib)
    else(WIN32)
    endif(WIN32)
    
endif(GDAL_USE_ODBC)

option(GDAL_USE_XMLREFORMAT "Set ON to use xmlreformat" OFF)
if(GDAL_USE_XMLREFORMAT)
		set(LIB_CSOURCES ${LIB_CSOURCES}
			xmlreformat.cpp  
		)
endif(GDAL_USE_XMLREFORMAT)

# source_group("Header Files\\port" FILES ${LIB_HHEADERS})
# source_group("Source Files\\port" FILES ${LIB_CSOURCES})

set(GDAL_TARGET_OBJECTS ${GDAL_TARGET_OBJECTS} $<TARGET_OBJECTS:${LIB_NAME}> PARENT_SCOPE)
add_library(${LIB_NAME} OBJECT ${LIB_HHEADERS} ${LIB_CSOURCES})

set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB})

if(ZLIB_FOUND)
	set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB} ${ZLIB_LIBRARIES})
endif(ZLIB_FOUND)

if(CURL_FOUND)
	set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB} ${CURL_LIBRARIES})
endif(CURL_FOUND)

if(ICONV_FOUND)
	set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB} ${ICONV_LIBRARIES})
endif(ICONV_FOUND)

set(GDAL_TARGET_LINK_LIB ${GDAL_TARGET_LINK_LIB} PARENT_SCOPE)

install(FILES ${LIB_HHEADERS} DESTINATION include/gdal)
#install(DIRECTORY ${GDAL_ROOT_SOURCE_DIR}/port DESTINATION include/gdal FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
