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
    # Always use the above 2 paths for testing
    
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
    with zipfile.ZipFile(package_name, 'w', zipfile.ZIP_DEFLATED) as itkpack:
        for dirpath,dirnames,filenames in os.walk(install_dir):
            for filename in filenames:
                fullpath = os.path.join(dirpath,filename)
                itkpack.write(fullpath)
                
    
def unpack():
    package_name = "ITK-4.6.zip"
    # Get python path
    pypath = subprocess.check_output(["where", "python"]).strip()
    dest = "C:/" #Always absolute path
    # Attention cmake path is considered by default "C:/Program Files/CMake/"
    pattern_cmake = re.compile(r'ITK-4.6/Program Files/CMake/', re.I)
    pattern_itk = re.compile(r'ITK-4.6/Program Files/ITK', re.I)
    pattern_py = re.compile(r'ITK-4.6/tools/', re.I)
    with zipfile.ZipFile(package_name, 'r') as itkpack:
        filelist = itkpack.namelist()
        for member in filelist:
            if re.search(pattern_py, member):
                pass
            elif re.search(pattern_itk, member):
                itk_dest = os.path.join(dest, member.split('/',1)[1])
                itkpack.extract(member, itk_dest)
            else:
                itkpack.extract(member, dest)
        




    
