add_subdirectory("dataset_io")
add_subdirectory("routing")

file(GLOB files "*.py")
install_py_files(${files})

add_definitions(-DHAVE_CONFIG_H)

find_path(OFLOG "dcmtk/oflog/oflog.h")

if(NOT ${OFLOG} STREQUAL "OFLOG-NOTFOUND")
    swig_python_module(generate_dicomdir 
        generate_dicomdir.i generate_dicomdir.cpp DicomDirGenerator.cpp)
endif()

get_directory_property(include_directories INCLUDE_DIRECTORIES)
set_directory_properties(PROPERTIES
    INCLUDE_DIRECTORIES "${include_directories};${DCMTK_INCLUDE_DIRS};${PYTHON_INCLUDE_DIRS}"
)

if("${DCMTK_LIBRARIES}" STREQUAL "")
    set(DCMTK_LIBRARIES dcmdata dcmimage dcmimgle dcmjpeg dcmnet dcmpstat 
        dcmqrdb dcmsr dcmtls ijg12 ijg16 ijg8 ofstd)
endif()
find_library(oflog oflog)
if(NOT ${oflog} STREQUAL "oflog-NOTFOUND")
    set(DCMTK_LIBRARIES ${DCMTK_LIBRARIES} oflog)
endif()

if(NOT ${OFLOG} STREQUAL "OFLOG-NOTFOUND")
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        swig_link_libraries(generate_dicomdir 
            ${DCMTK_LIBRARIES} gdcmMSFF ${DCMTK_DIR}/lib/oflog.lib)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        swig_link_libraries(generate_dicomdir 
            dcmdsig ${DCMTK_LIBRARIES} gdcmMSFF z pthread)
    else()
        swig_link_libraries(generate_dicomdir 
            dcmdsig ${DCMTK_LIBRARIES} gdcmMSFF wrap pthread)
    endif()
    install_swig_python_module(generate_dicomdir)
endif()

itk_wrap_module("AssembleTilesImageFilter")
    set(WRAPPER_LIBRARY_DEPENDS ITKCommon)
    
    itk_wrap_submodule("itkAssembleTilesImageFilter")
       itk_wrap_class("itk::AssembleTilesImageFilter" POINTER)
            foreach(type ${WRAP_ITK_SCALAR})
                set(dimensions "2")
                foreach(dimension ${dimensions})
                    math(EXPR next_dimension "${dimension}+1")
                    
                    set(input_image_type ${ITKT_I${type}${dimension}})
                    set(output_image_type ${ITKT_I${type}${next_dimension}})
                    
                    set(template_params "${input_image_type}, ${output_image_type}")
                    
                    set(input_mangled_type ${ITKM_I${type}${dimension}})
                    set(output_mangled_type ${ITKM_I${type}${next_dimension}})
                    set(mangled_type "${input_mangled_type}${output_mangled_type}")
                    
                    itk_wrap_template("${mangled_type}" "${template_params}")
                endforeach()
            endforeach(type)
        itk_end_wrap_class()
    itk_end_wrap_submodule()
itk_end_wrap_module()
