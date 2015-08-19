file(GLOB_RECURSE files "*.py")
install_py_files(${files})

add_library(medipy_hmc SHARED EM.cpp HMCInitializer.cpp)

find_package(ITK REQUIRED)
include(${USE_ITK_FILE})
target_link_libraries(medipy_hmc ${ITK_LIBRARIES})

itk_wrap_module("MediPyHMC")
    set(WRAPPER_LIBRARY_DEPENDS ITKCommon)
    
    itk_wrap_submodule("itkHMCSegmentationImageFilter")
        itk_wrap_class("itk::HMCSegmentationImageFilter" POINTER)
            itk_wrap_template("IF3IF3IF3" "${ITKT_IF3}, ${ITKT_IF3}, ${ITKT_IF3}")
        itk_end_wrap_class()
    itk_end_wrap_submodule()

itk_end_wrap_module()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(objs_dir "./medipy_hmc.dir/Release")
	set_target_properties(MediPyHMCPython PROPERTIES LINK_FLAGS_RELEASE "${objs_dir}/EM.obj ${objs_dir}/HMCInitializer.obj")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(${lib} medipy_hmc)
else()
	add_dependencies(${lib} medipy_hmc)
endif()
    
