file(GLOB files "*.py")
install_py_files(${files})

set_source_files_properties(traitement.i PROPERTIES CPLUSPLUS ON)
include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/..")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/../medimax.dir/Release")
endif()

swig_python_module(traitement traitement.i)
swig_link_libraries(traitement medimax)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set_target_properties(_traitement PROPERTIES LINK_FLAGS_RELEASE
 "trai_3d.obj
imx_matrix.obj
imx_3d.obj
oper_3d.obj
segm_3d.obj
imx_sort.obj
imx_export_file.obj
imx_log.obj
mani_3d.obj
ana_3d.obj
otsu_3d.obj
global.obj
chps_3d.obj
imx_misc.obj
imx_math.obj
mtch_3d.obj
imx_file.obj
imx_bspline.obj
transformations_3d.obj
divers_topo.obj
imx_picture3d.obj
minimisation_3d.obj
distance_3d.obj
mtch_robust_3d.obj
norm_3d.obj
multiresolution_3d.obj
erreur_recalage.obj
imx_head.obj
imx_picture2d.obj
imx_2d.obj
analyse_intervalle.obj
normalisation_topo.obj
file_3d.obj
morpho_3d.obj
gradient_distance_3d.obj
mtch_topo_3d.obj
detection_anomalies_3d.obj
croissance_de_region.obj
queues_3d.obj
distance_topo.obj
optimisation_topo.obj
differential_bias_correction.obj
gradient_distance_topo.obj
hessien_distance_topo.obj")
endif()

install_swig_python_module(traitement)

