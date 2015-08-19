import os
import shutil

medipy_path = "/Users/sun/workspace/medipy_changed/"
src = "./cmake_fixed_files/"

filelist = {}
filename = "CMakeLists.txt"
filelist.update({
    "dataset_io": "lib/medipy/io/dicom/dataset_io",
    "lib-medipy-gui-image": "lib/medipy/gui/image",
    "medimax_traitement": "plugins/medimax/traitement",
    "plugins_hmc": "plugins/hmc",
    "plugins_intensity": "plugins/intensity",
    "plugins_medimax": "plugins/medimax",
    "io_dicom": "lib/medipy/io/dicom",
    "network_dicom_scu": "lib/medipy/network/dicom/scu"
    })

for name, path in filelist.items():
    filelist[name] = medipy_path + filelist[name]

for name, path in filelist.items():
    if not os.path.exists(path):
        os.makedirs(path)
    shutil.copy(os.path.join(src, name, filename), path)
