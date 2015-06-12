import os
import shutil

medipy_path = "F:/workspace/medipy/"
src = "E:/deployment/medipy_log/medipy_changed_files/"

filelist = {}
filelist.update({
    "circle.py": "lib/medipy/gui/brushes",
    "EM.cpp": "plugins/hmc",
    "encapsulated_document.py": "lib/medipy/io/dicom",
    "explorer.py": "lib/medipy/gui/dicom",
    "float_interval.py": "lib/medipy/gui/control",
    "HilbertCurveChainGenerator.txx": "plugins/hmc",
    "HMCInitializer.cpp": "plugins/hmc",
    "image.py": "lib/medipy/base",
    "imx_sort.c": "plugins/medimax/outils",
    "itkBETImageFilter.txx": "plugins/segmentation",
    "itkBootstrapDWIStatisticsImageFilter.h": "plugins/diffusion",
    "itkJointHistogramCalculator.txx": "plugins/intensity",
    "itkStreamlineTractographyAlgorithm.txx": "plugins/diffusion",
    "square.py": "lib/medipy/gui/brushes",
    "test_bet.py": "plugins/fsl/tests",
    "test_siena.py": "plugins/fsl/tests",
    "test_sienax.py": "plugins/fsl/tests",
    "test_encapsulated_document.py": "lib/tests/code/io/dicom",
    "test_scu.py": "lib/tests/code/network/dicom",
    "types.py": "lib/medipy/itk",
    "vtk_io.py": "plugins/diffusion"
    })

for name, path in filelist.items():
    filelist[name] = medipy_path + filelist[name]


for name, path in filelist.items():
    if not os.path.exists(path):
        os.makedirs(path)
    shutil.copy(os.path.join(src, name), path)
