find_package(VTK REQUIRED)
include(${USE_VTK_FILE})

file(GLOB files "*.py")
install_py_files(${files})

itk_wrap_module("MediPyIntensity")
    
    set(WRAPPER_LIBRARY_DEPENDS ITKCommon ITKStatistics)
    
    find_swig_library_files("JointHistogramTransferFunctionCalculator")
    
    itk_wrap_submodule("itkJointHistogramCalculator")
        itk_wrap_class("itk::JointHistogramCalculator" "POINTER")
            itk_wrap_image_filter("${WRAP_ITK_SCALAR}" 2)
        itk_end_wrap_class()
    itk_end_wrap_submodule()
    
    itk_wrap_submodule("itkJointHistogramTransferFunctionCalculator")
        itk_wrap_class("itk::JointHistogramTransferFunctionCalculator" "POINTER")
            itk_wrap_template("HF" ${ITKT_HF})
        itk_end_wrap_class()
    itk_end_wrap_submodule()
    
    itk_wrap_submodule("itkTransformIntensityImageFilter")
        itk_wrap_class("itk::TransformIntensityImageFilter" "POINTER")
            itk_wrap_image_filter("${WRAP_ITK_SCALAR}" 2)
        itk_end_wrap_class()
    itk_end_wrap_submodule()
    
itk_end_wrap_module()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(VTK_LIBS_ELSE
            vtkCommon
            vtkFiltering
            vtkImaging
            vtkGraphics
            vtkGenericFiltering
            vtkIO
            vtkRendering
            vtkVolumeRendering
            vtkHybrid
            vtkWidgets
            vtkInfovis
            vtkGeovis
            vtkViews
            vtkCharts
            vtkCommonPythonD
            vtkFilteringPythonD
            vtkImagingPythonD
            vtkGraphicsPythonD
            vtkGenericFilteringPythonD
            vtkIOPythonD
            vtkRenderingPythonD
            vtkVolumeRenderingPythonD
            vtkHybridPythonD
            vtkWidgetsPythonD
            vtkInfovisPythonD
            vtkGeovisPythonD
            vtkViewsPythonD
            vtkChartsPythonD
            vtkPythonCore
            vtksys
        )
    target_link_libraries(MediPyIntensityPython ${VTK_LIBS_ELSE})
endif()