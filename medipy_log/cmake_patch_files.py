import os
import shutil

medipy_path = "F:/workspace/medipy/"
src = "E:/deployment/medipy_log/cmake_fixed_files/"

filelist = {}
filename = "CMakeLists.txt"
filelist.update({
    "dataset_io": "lib/medipy/io/dicom/dataset_io",
    "lib-medipy-gui-image": "lib/medipy/gui/image",
    "medimax_traitement": "plugins/medimax/traitement",
    "plugins_hmc": "plugins/hmc",
    "plugins_intensity": "plugins/intensity"
    })

for name, path in filelist.items():
    filelist[name] = medipy_path + filelist[name]

for name, path in filelist.items():
    if not os.path.exists(path):
        os.makedirs(path)
    shutil.copy(os.path.join(src, name, filename), path)
