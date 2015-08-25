file(GLOB python_files "*.py")
install_py_files(${python_files})

set(modules noyau segmentation outils detection math morpho
            recalage recalage/topo traitement noyau/io noyau/gui 
            wrapping)

set(source_files "")
foreach(module ${modules})
    if((NOT ${module} STREQUAL "wrapping"))
        file(GLOB swig_files "${module}/*.i")
        if((NOT ${swig_files} STREQUAL ""))
            add_subdirectory(${module})
        endif()
    endif()
    file(GLOB module_source_files "${module}/*.c" "${module}/*.cpp")
    set(source_files ${source_files} ${module_source_files})
endforeach()

find_package(PythonLibs)
# Include before, otherwise config.h is not found
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PYTHON_INCLUDE_PATH})
add_library(medimax SHARED ${source_files})
target_link_libraries(medimax ${PYTHON_LIBRARIES})

file(RELATIVE_PATH destination ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/dummy)
get_filename_component(destination ${destination} PATH)
install(TARGETS medimax DESTINATION ${destination})
