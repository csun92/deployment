find_package(DCMTK REQUIRED)
if(NOT GDCM_FOUND)
    find_package(GDCM REQUIRED)
endif()

add_definitions(-DHAVE_CONFIG_H)
swig_python_module(dataset_io
    dataset_io.i dataset_io.cpp DCMTKToPython.cpp PythonToDCMTK.cpp)

set_directory_properties(PROPERTIES
    INCLUDE_DIRECTORIES "${DCMTK_INCLUDE_DIRS};${GDCM_INCLUDE_DIRS};${PYTHON_INCLUDE_DIRS}"
)

if("${DCMTK_LIBRARIES}" STREQUAL "")
    set(DCMTK_LIBRARIES dcmdata dcmimage dcmimgle dcmjpeg dcmnet dcmpstat
        dcmqrdb dcmsr dcmtls ijg12 ijg16 ijg8 ofstd)
endif()
find_library(oflog oflog)
if(NOT ${oflog} STREQUAL "oflog-NOTFOUND")
    set(DCMTK_LIBRARIES ${DCMTK_LIBRARIES} oflog)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	swig_link_libraries(dataset_io
	    ${DCMTK_LIBRARIES}  gdcmMSFF ${DCMTK_DIR}/lib/oflog.lib ${CMAKE_SOURCE_DIR}/lib/medipy/base/_exception.dir/Release/Exception.obj
        ${CMAKE_SOURCE_DIR}/lib/medipy/base/_exception.dir/Release/ExceptionPYTHON_wrap.obj)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  swig_link_libraries(dataset_io
	    dcmdsig ${DCMTK_LIBRARIES} gdcmMSFF z pthread ${CMAKE_SOURCE_DIR}/lib/medipy/base/CMakeFiles/_exception.dir/Exception.cpp.o
      ${CMAKE_SOURCE_DIR}/lib/medipy/base/CMakeFiles/_exception.dir/ExceptionPYTHON_wrap.cxx.o)
else()
	swig_link_libraries(dataset_io
	    medipy_base dcmdsig ${DCMTK_LIBRARIES} gdcmMSFF wrap pthread)
endif()

install_swig_python_module(dataset_io)
