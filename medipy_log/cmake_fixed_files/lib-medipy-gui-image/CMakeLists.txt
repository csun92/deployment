file(GLOB files "*.py")
install_py_files(${files})

find_package(VTK REQUIRED)
include(${USE_VTK_FILE})

swig_python_module(helpers helpers.i helpers.cpp)
swig_link_libraries(helpers ${VTK_PYTHON_LIBRARIES} ${VTK_LIBRARIES})
install_swig_python_module(helpers)
