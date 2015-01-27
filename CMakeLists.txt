# ******************************************************************************
# * Project:  CMake4GDAL
# * Purpose:  CMake build scripts
# * Author: Dmitry Baryshnikov (aka Bishop), polimax@mail.ru
# ******************************************************************************
# * Copyright (C) 2012,2013,2014 Dmitry Baryshnikov
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

cmake_minimum_required (VERSION 2.8.10 FATAL_ERROR)

set(PROJECT_NAME "gdallib")
project(${PROJECT_NAME})

set(CMAKE_COLOR_MAKEFILE ON)

# parse the version number from gdal_version.h and include in GDAL_MAJOR_VERSION and GDAL_MINOR_VERSION
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/gcore/gdal_version.h GDAL_VERSION_H_CONTENTS)
string(REGEX MATCH "GDAL_VERSION_MAJOR[ \t]+([0-9]+)"
  GDAL_MAJOR_VERSION ${GDAL_VERSION_H_CONTENTS})
string (REGEX MATCH "([0-9]+)"
  GDAL_MAJOR_VERSION ${GDAL_MAJOR_VERSION})
string(REGEX MATCH "GDAL_VERSION_MINOR[ \t]+([0-9]+)"
  GDAL_MINOR_VERSION ${GDAL_VERSION_H_CONTENTS})
string (REGEX MATCH "([0-9]+)"
  GDAL_MINOR_VERSION ${GDAL_MINOR_VERSION})
string(REGEX MATCH "GDAL_VERSION_REV[ \t]+([0-9]+)"
  GDAL_REV_VERSION ${GDAL_VERSION_H_CONTENTS})  
string (REGEX MATCH "([0-9]+)"
  GDAL_REV_VERSION ${GDAL_REV_VERSION})
    
# Setup package meta-data
set(GDAL_VERSION ${GDAL_MAJOR_VERSION}.${GDAL_MINOR_VERSION}.${GDAL_REV_VERSION})
message(STATUS "gdal version=[${GDAL_VERSION}]")

if(WIN32)    
    set(GDAL_LIB_NAME gdal${GDAL_MAJOR_VERSION}${GDAL_MINOR_VERSION})
elseif(UNIX)
    set(GDAL_LIB_NAME gdal)
endif()
message(STATUS "GDAL lib name ... " ${GDAL_LIB_NAME})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(GDAL_ROOT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(GDAL_ROOT ${GDAL_ROOT_SOURCE_DIR})
set(GDAL_ROOT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# set path to additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# uninstall target
configure_file(${CMAKE_MODULE_PATH}/cmake_uninstall.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake IMMEDIATE @ONLY)
   
include(configure)

option(GDAL_USE_CPL "Set ON to use cpl" ON)
option(GDAL_USE_SSE "Set ON to use sse" OFF)

option(GDAL_ENABLE_OGR "Build OGR module" ON)
if(GDAL_ENABLE_OGR)
	add_definitions(-DOGR_ENABLED)
endif()

include_directories(${GDAL_ROOT_SOURCE_DIR}/port)
include_directories(${GDAL_ROOT_BINARY_DIR}/port)
if(WIN32)
    include_directories(${GDAL_ROOT_SOURCE_DIR}/ogr)   
endif()

set(GDAL_TARGET_OBJECTS "")
set(GDAL_TARGET_LINK_LIB "")
set(GDAL_FORMATS "")

add_subdirectory(port)
add_subdirectory(gcore)
add_subdirectory(alg)
add_subdirectory(ogr)
add_subdirectory(frmts)
option(GDAL_BUILD_NETWORK_SUPPORT "Set ON to build geography network support" ON)
if(GDAL_BUILD_NETWORK_SUPPORT)
    add_subdirectory(gnm)
endif()    
add_subdirectory(swig)
add_subdirectory(apps)

list(REMOVE_DUPLICATES GDAL_TARGET_OBJECTS)
foreach(TO ${GDAL_TARGET_OBJECTS})
	message(STATUS "gdal link=${TO}")
endforeach(TO)
  
option(GDAL_BUILD_SHARED "Set ON to build shared library" ON)
if(GDAL_BUILD_SHARED)
	add_library(${GDAL_LIB_NAME} SHARED ${GDAL_CSOURCES} ${GDAL_TARGET_OBJECTS})#${GDAL_HHEADERS} 
else(GDAL_BUILD_SHARED)
	add_library(${GDAL_LIB_NAME} STATIC ${GDAL_CSOURCES} ${GDAL_TARGET_OBJECTS})#${GDAL_HHEADERS} 
endif(GDAL_BUILD_SHARED)

#list(REMOVE_DUPLICATES GDAL_TARGET_LINK_LIB)
set(LIBS "")
foreach(TL ${GDAL_TARGET_LINK_LIB})
	message(STATUS "gdal target link lib=${TL}")
	set(LIBS ${LIBS} " -l${TL}")
endforeach(TL)
if(GDAL_TARGET_LINK_LIB)
	target_link_libraries(${GDAL_LIB_NAME} ${GDAL_TARGET_LINK_LIB})
endif(GDAL_TARGET_LINK_LIB)
 
set_target_properties(${GDAL_LIB_NAME}
	PROPERTIES PROJECT_LABEL ${PROJECT_NAME}
    VERSION ${GDAL_VERSION}
    SOVERSION 1
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
    
add_custom_command(TARGET ${GDAL_LIB_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_LINKER_FILE:${GDAL_LIB_NAME}> ${GDAL_ROOT_BINARY_DIR}
)    
	
option(GDAL_INSTALL_DATA_IN_VERSION "Set ON to install GDAL in path with version (i.e. usr/local/share/gdal/1.11" OFF)

if(WIN32)
    if(CMAKE_CL_64)
        install(TARGETS ${GDAL_LIB_NAME}
			RUNTIME DESTINATION bin
			ARCHIVE DESTINATION lib/x64
			LIBRARY DESTINATION lib/x64)
    else()
        install(TARGETS ${GDAL_LIB_NAME}
			RUNTIME DESTINATION bin
			ARCHIVE DESTINATION lib/x86
			LIBRARY DESTINATION lib/x86)
    endif()
	install(FILES ${GDAL_ROOT_BINARY_DIR}/gcore/gdal_def.h DESTINATION include/gdal)
else()
    install(TARGETS ${GDAL_LIB_NAME}
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
	install(FILES ${GDAL_ROOT_BINARY_DIR}/gcore/gdal_def.h DESTINATION include/gdal)
	
	include(install_gdal_config)
endif() 
    
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

