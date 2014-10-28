# ******************************************************************************
# * Project:  CMake4GDAL
# * Purpose:  CMake build scripts
# * Author: Dmitry Baryshnikov (aka Bishop), polimax@mail.ru
# ******************************************************************************
# * Copyright (C) 2014 Dmitry Baryshnikov
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

function(find_python_module module)
	string(TOUPPER ${module} module_upper)
    if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
        set(${module}_FIND_REQUIRED TRUE)
    else()
        if (ARGV1 STREQUAL "QUIET")
            set(PY_${module}_FIND_QUIETLY TRUE)
        endif()
    endif()
    
	if(NOT PY_${module_upper})
		# A module's location is usually a directory, but for binary modules
		# it's a .so file.
		execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c" 
			"import re, ${module}; print(re.compile('/__init__.py.*').sub('',${module}.__file__))"
			RESULT_VARIABLE _${module}_status 
			OUTPUT_VARIABLE _${module}_location
			ERROR_QUIET 
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		if(NOT _${module}_status)
			set(PY_${module_upper} ${_${module}_location} CACHE STRING 
				"Location of Python module ${module}")
		endif(NOT _${module}_status)
	endif(NOT PY_${module_upper})
	find_package_handle_standard_args(PY_${module} DEFAULT_MSG PY_${module_upper})
endfunction(find_python_module)

find_package(PythonInterp REQUIRED)
if(NOT PYTHONINTERP_FOUND)
    MESSAGE(FATAL_ERROR "Cannot find required python module numpy")        
endif()

find_package(PythonLibs REQUIRED)
if(PYTHONLIBS_FOUND)

    if(NOT PYTHON_DEBUG_LIBRARY)
        set(PYTHON_DEBUG_LIBRARY ${PYTHON_LIBRARY} CACHE  STRING 
				"PYTHON_DEBUG_LIBRARY-NOTFOUND")
    endif()
    
    if(NOT PYTHON_LIBRARY_DEBUG)    
        set(PYTHON_LIBRARY_DEBUG ${PYTHON_LIBRARY} CACHE STRING 
				"PYTHON_LIBRARY_DEBUG-NOTFOUND")
    endif()

    find_python_module(numpy REQUIRED) 
    if(PY_NUMPY)
        get_filename_component(PY_NUMPY_ROOT_LOCATION ${PY_NUMPY} PATH)
        include_directories(${PY_NUMPY_ROOT_LOCATION}/core/include)
     
        set(PACKAGE_DIR "osgeo")    
        set(SWIGOUTPUTDIR "extensions/")
        
        include_directories(${PYTHON_INCLUDE_DIRS})  
        include_directories(${SWIG}/include) 
        include_directories(${SWIG}/include/${BINDING})  
        include_directories(${SWIG}/include/${BINDING}/docs) 
        include_directories(${CMAKE_CURRENT_SOURCE_DIR})
        include_directories(${SWIG}) 
        
        include_directories(${GDAL_ROOT_SOURCE_DIR}/gcore)
        include_directories(${GDAL_ROOT_SOURCE_DIR}/frmts)
        include_directories(${GDAL_ROOT_SOURCE_DIR}/ogr)
        include_directories(${GDAL_ROOT_SOURCE_DIR}/ogr/ogrsf_frmts)
        include_directories(${GDAL_ROOT_SOURCE_DIR}/alg)
		include_directories(${GDAL_ROOT_SOURCE_DIR}/gnm)
		include_directories(${GDAL_ROOT_SOURCE_DIR}/gnm/frmts)
        
        execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${GDAL_ROOT_BINARY_DIR}/swig/include)   
        execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/${SWIGOUTPUTDIR} )
        
        set(PYTHON_SWIG_INCLUDE "")
        set(PYTHON_SWIG_LIBRARY "")
        if(UNIX)
            set(PYTHON_SWIG_INCLUDE ${GDAL_ROOT_SOURCE_DIR}/port:${GDAL_ROOT_BINARY_DIR}/port:${GDAL_ROOT_SOURCE_DIR}/gcore:${GDAL_ROOT_SOURCE_DIR}/alg:${GDAL_ROOT_SOURCE_DIR}/ogr:${GDAL_ROOT_SOURCE_DIR}/ogr/ogrsf_frmts:${GDAL_ROOT_SOURCE_DIR}/gnm:${GDAL_ROOT_SOURCE_DIR}/gnm/frmts)
            set(PYTHON_SWIG_LIBRARY "../../.libs:../../")
        endif()
        
        if(WIN32)
            set(PYTHON_SWIG_INCLUDE ${GDAL_ROOT_SOURCE_DIR}/port ${GDAL_ROOT_BINARY_DIR}/port ${GDAL_ROOT_SOURCE_DIR}/gcore ${GDAL_ROOT_SOURCE_DIR}/alg ${GDAL_ROOT_SOURCE_DIR}/ogr ${GDAL_ROOT_SOURCE_DIR}/ogr/ogrsf_frmts ${GDAL_ROOT_SOURCE_DIR}/gnm ${GDAL_ROOT_SOURCE_DIR}/gnm/frmts)
            set(PYTHON_SWIG_LIBRARY "../../.libs:../../")
        endif()
        
        configure_file(${CMAKE_MODULE_PATH}/python_setup.cfg.cmake ${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/setup.cfg IMMEDIATE @ONLY)
     
        
        add_definitions(-DSWIGPYTHON -DNDEBUG)
        remove_definitions(-D_DEBUG)
        remove_definitions(-DDEBUG)             
        
        set(GDAL_WRAP_INTERFACE_FILES
            ../include/gdal.i 
            ../include/MajorObject.i 
            ../include/Driver.i 
            ../include/Dataset.i 
            ../include/Band.i 
            ../include/ColorTable.i 
            ../include/cpl.i 
            ../include/${BINDING}/gdal_${BINDING}.i 
            ../include/${BINDING}/typemaps_${BINDING}.i         
        )
        
        set_source_files_properties(${GDAL_WRAP_INTERFACE_FILES}  PROPERTIES CPLUSPLUS ON)
        set_source_files_properties(${GDAL_WRAP_INTERFACE_FILES} PROPERTIES SWIG_FLAGS "-includeall")
        swig_add_module(gdal_wrap ${BINDING}
            ../include/gdal.i 
        )    
        swig_link_libraries(gdal_wrap ${PYTHON_LIBRARIES} ${GDAL_LIB_NAME})
        
        set_source_files_properties(../include/gdalconst.i PROPERTIES CPLUSPLUS OFF)
        swig_add_module(gdalconst_wrap ${BINDING} 
            ../include/gdalconst.i 
        )
        swig_link_libraries(gdalconst_wrap ${PYTHON_LIBRARIES} ${GDAL_LIB_NAME})
        
        
        set(GDAL_WRAP_INTERFACE_FILES
        ../include/ogr.i 
        ../include/${BINDING}/ogr_${BINDING}.i 
        ../include/${BINDING}/typemaps_${BINDING}.i    
        )
        set_source_files_properties(${GDAL_WRAP_INTERFACE_FILES}  PROPERTIES CPLUSPLUS ON)    
        set_source_files_properties(${GDAL_WRAP_INTERFACE_FILES} PROPERTIES SWIG_FLAGS "-includeall")
        swig_add_module(ogr_wrap ${BINDING}
            ../include/ogr.i 
        )
        swig_link_libraries(ogr_wrap ${PYTHON_LIBRARIES} ${GDAL_LIB_NAME})


        set(GDAL_WRAP_INTERFACE_FILES
        ../include/osr.i 
        ../include/${BINDING}/osr_${BINDING}.i 
        ../include/${BINDING}/typemaps_${BINDING}.i   
        )  
        set_source_files_properties(${GDAL_WRAP_INTERFACE_FILES}  PROPERTIES CPLUSPLUS ON) 
        set_source_files_properties(${GDAL_WRAP_INTERFACE_FILES} PROPERTIES SWIG_FLAGS "-includeall")
        swig_add_module(osr_wrap ${BINDING}
            ../include/osr.i 
        )
        swig_link_libraries(osr_wrap ${PYTHON_LIBRARIES} ${GDAL_LIB_NAME})   
        
        
        set(GDAL_WRAP_INTERFACE_FILES
        ../include/gdal_array.i
        ../include/${BINDING}/typemaps_${BINDING}.i  
        )    
        set_source_files_properties(${GDAL_WRAP_INTERFACE_FILES}  PROPERTIES CPLUSPLUS ON)    
        set_source_files_properties(${GDAL_WRAP_INTERFACE_FILES} PROPERTIES SWIG_FLAGS "-includeall")
        swig_add_module(gdal_array_wrap ${BINDING}
            ../include/gdal_array.i
        )
        swig_link_libraries(gdal_array_wrap ${PYTHON_LIBRARIES} ${GDAL_LIB_NAME})   
        
        set(GDAL_WRAP_INTERFACE_FILES
        ../include/gnm.i 
        ../include/${BINDING}/gnm_${BINDING}.i 
        ../include/${BINDING}/typemaps_${BINDING}.i    
        )
        set_source_files_properties(${GDAL_WRAP_INTERFACE_FILES}  PROPERTIES CPLUSPLUS ON)    
        set_source_files_properties(${GDAL_WRAP_INTERFACE_FILES} PROPERTIES SWIG_FLAGS "-includeall")
        swig_add_module(gnm_wrap ${BINDING}
            ../include/gnm.i 
        )
        swig_link_libraries(gnm_wrap ${PYTHON_LIBRARIES} ${GDAL_LIB_NAME})
                
        set(VC_DIR "")
        if(MSVC)
            if(EXISTS "$ENV{VS100COMNTOOLS}../../VC") 
               set(VC_DIR $ENV{VS100COMNTOOLS}) 
            endif() 
            if(EXISTS "$ENV{VS110COMNTOOLS}../../VC") 
               set(VC_DIR $ENV{VS110COMNTOOLS}) 
            endif() 
            if(EXISTS "$ENV{VS120COMNTOOLS}../../VC") 
               set(VC_DIR $ENV{VS120COMNTOOLS}) 
            endif() 
            
            message(STATUS "VC_DIR='${VC_DIR}'")
            
            set(ENV{VS90COMNTOOLS} ${VC_DIR})
        endif()
        
        if (CMAKE_MAJOR_VERSION GREATER 2)
            set(PYTHONSWIG_H_PATH "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}")
        else(CMAKE_MAJOR_VERSION GREATER 2)
            set(PYTHONSWIG_H_PATH "${GDAL_ROOT_BINARY_DIR}/swig/include")
        endif(CMAKE_MAJOR_VERSION GREATER 2)
        
        add_custom_target(swig_build ALL             
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PYTHONSWIG_H_PATH}/gdal_arrayPYTHON_wrap.cxx" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/${SWIGOUTPUTDIR}/gdal_array_wrap.cpp"   
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PYTHONSWIG_H_PATH}/gdalconstPYTHON_wrap.c" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/${SWIGOUTPUTDIR}/gdalconst_wrap.c"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PYTHONSWIG_H_PATH}/gdalPYTHON_wrap.cxx" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/${SWIGOUTPUTDIR}/gdal_wrap.cpp"  
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PYTHONSWIG_H_PATH}/ogrPYTHON_wrap.cxx" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/${SWIGOUTPUTDIR}/ogr_wrap.cpp"  
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PYTHONSWIG_H_PATH}/osrPYTHON_wrap.cxx" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/${SWIGOUTPUTDIR}/osr_wrap.cpp"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PYTHONSWIG_H_PATH}/gnmPYTHON_wrap.cxx" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/${SWIGOUTPUTDIR}/gnm_wrap.cpp"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GDAL_ROOT_SOURCE_DIR}/swig/${BINDING}/README.txt" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/README.txt"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GDAL_ROOT_SOURCE_DIR}/swig/${BINDING}/gdalnumeric.py" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/gdalnumeric.py" 
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GDAL_ROOT_SOURCE_DIR}/swig/${BINDING}/osgeo/gdalnumeric.py" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/osgeo/gdalnumeric.py" 
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/gdal.py" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/osgeo/gdal.py"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/gdalconst.py" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/osgeo/gdalconst.py"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/ogr.py" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/osgeo/ogr.py"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/osr.py" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/osgeo/osr.py"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/gnm.py" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/osgeo/gnm.py"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GDAL_ROOT_SOURCE_DIR}/swig/${BINDING}/osgeo/__init__.py" "${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/osgeo/__init__.py"

            COMMAND set VS90COMNTOOLS=${VC_DIR}
            COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/setup.py" "build" 
            WORKING_DIRECTORY  ${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}/
            DEPENDS _gdal_wrap _gdalconst_wrap _ogr_wrap _osr_wrap _gdal_array_wrap _gnm_wrap
        )

        
        install(CODE "execute_process(
            COMMAND \"${PYTHON_EXECUTABLE}\" \"${CMAKE_CURRENT_SOURCE_DIR}/setup.py\" install
            WORKING_DIRECTORY  ${GDAL_ROOT_BINARY_DIR}/swig/${BINDING}
        )")

    #    add_custom_target(swig_install
    #        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/setup.py" "install"
    #        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
    #    )
        
    #    install(TARGETS swig_install)
        
    #    set(PY_COMMANDS     
    #        epsg_tr.py 
    #        gdalchksum.py 
    #        gdal2xyz.py 
    #        gcps2wld.py
    #        gdalimport.py 
    #        gdal_merge.py 
    #        pct2rgb.py 
    #        rgb2pct.py
    #        gcps2vec.py
    #        )

    #    set(PY_MODULES 
    #        ${PACKAGE_DIR}/gdal.py 
    #        ${PACKAGE_DIR}/ogr.py
    #        ${PACKAGE_DIR}/osr.py 
    #        ${PACKAGE_DIR}/gdalconst.py 
    #        ${PACKAGE_DIR}/gdal_array.py
    #        )
            
    #    execute_process(COMMAND python -c "from distutils.sysconfig import get_python_lib; print get_python_lib()" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
    #    install(TARGETS _gdal_wrap _gdalconst_wrap _ogr_wrap _osr_wrap _gdal_array_wrap DESTINATION ${PYTHON_SITE_PACKAGES}/${PACKAGE_DIR})
    #    install(DIRECTORY ${CMAKE_BINARY_DIR}/swig/python DESTINATION ${PYTHON_SITE_PACKAGES}/${PACKAGE_DIR} FILES_MATCHING PATTERN "*.py")
    else(PY_NUMPY)
        MESSAGE(FATAL_ERROR "Cannot find required python module numpy")
    endif(PY_NUMPY)
endif()
        
