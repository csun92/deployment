import argparse
import glob
import os
import shutil
import subprocess
import re
import zipfile


def Directory(value):
    if not os.path.isdir(value):
        raise argparse.ArgumentTypeError("No such directory: '{}'".format(value))
    return os.path.abspath(value)

def build(binary_dir, jobs):
    subprocess.check_call(["mingw32-make", "-j", str(jobs)], cwd=binary_dir)

def install(binary_dir, install_dir):
    # No need to add path "usr"
    # binary_dir = "F:/workspace/ITK-MinGW/ITK-build/"
    # install_dir = "C:/ITK-4.6/"
    # Always use the above 2 paths for usage
    
    # Patch install files that use absolute paths
    # Attention use string format like "\\"
    # For path, we can use (for example) "C:/" instead of "C:\"(except in shell)

    for dirpath, dirnames, filenames in os.walk(binary_dir):
        targets = [
            "install_wrapitk_compatibility.cmake",
            "cmake_install.cmake"
        ]
        for target in targets:
            if target in filenames:
                filename = os.path.join(dirpath, target)
                
                pattern = re.compile(r'file\(INSTALL DESTINATION \"C:/', re.I)
                repl = "file(INSTALL DESTINATION \"{}".format(install_dir)
                
                with open(filename,"r+") as fs:
					before = fs.read()
					after = re.sub(pattern,repl,before)
					fs.seek(0)
					fs.truncate(0)
					fs.write(after)

    # Install
    subprocess.check_call([
        "cmake", 
        "-DCMAKE_INSTALL_PREFIX:PATH={}".format(install_dir),
        "-P", os.path.join(binary_dir, "cmake_install.cmake")
    ])

    # No need to Fix install in PREFIX/usr

    # No need to Patch files that force downloading external projects,
    # since these projects were built included
    
    # Patch files with original install dir
    patchfile = os.path.join(install_dir, "lib/cmake/ITK-4.6/WrapITK/WrapITKConfig.cmake")
    pattern = re.compile(r'set\(possible_itk_dir.*')
    new_path = os.path.join(install_dir, "lib/cmake/ITK-4.6")
    repl = "set(possible_itk_dir \"{}\")".format(new_path)
    with open(patchfile, "r+") as fs:
        before = fs.read()
        after = re.sub(pattern,repl,before)
        fs.seek(0)
        fs.truncate(0)
        fs.write(after)

    # Fix PYTHONPATH for pygccxml
    with open(os.path.join(install_dir, "tools/python2/Lib/site-packages/WrapITK.pth"), "a") as fd:
        p = os.path.join(install_dir,"lib/cmake/ITK-4.6/WrapITK/Configuration/Generators/SwigInterface/src")
        fd.write(p+"\n")

    # Add missing files
    # * Wrapping/Typedefs/*SwigInterface.h -> usr/lib/cmake/ITK-4.6/WrapITK/Configuration/Typedefs/
    typedefs_source = os.path.join(binary_dir, "Wrapping", "Typedefs")
    typedefs_dest = os.path.join(install_dir, 
        "lib", "cmake", "ITK-4.6", "WrapITK", "Configuration", "Typedefs")
    for filename in glob.glob(os.path.join(typedefs_source, "*SwigInterface.h")):
        shutil.copy(filename, typedefs_dest)

def package(install_dir):
    package_name = "ITK-4.6.zip"
    pattern_py = re.compile(r'/tools', re.I)
    pattern_prog = re.compile(r'/Program Files', re.I)
    with zipfile.ZipFile(package_name, 'w', zipfile.ZIP_DEFLATED) as itkpack:
        for dirpath,dirnames,filenames in os.walk(install_dir):
            for filename in filenames:
                fullpath = os.path.join(dirpath,filename)
                if re.search(pattern_py, fullpath) or re.search(pattern_prog, fullpath):
                    dest = fullpath.replace("/ITK-4.6/","/")
                    itkpack.write(fullpath, dest)
                else:
                    itkpack.write(fullpath)
                

# Notes:
# To avoid import error: ImportError: DLL load failed: ...
# In terminal, add C:\ITK-4.6\bin (dll files location) to system path
# command: set PATH=C:\ITK-4.6\bin:%PATH%
# Attention: MinGW should be installed and configured in order to support WrapITK!

    
