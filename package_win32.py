import argparse
import glob
import os
import shutil
import subprocess


def Directory(value):
    if not os.path.isdir(value):
        raise argparse.ArgumentTypeError("No such directory: '{}'".format(value))
    return os.path.abspath(value)



def build(binary_dir, jobs):
    subprocess.check_call(["mingw32-make", "-j", str(jobs)], cwd=binary_dir)

def install(binary_dir, install_dir):
    # No need to add path "usr"
    # binary_dir = "F:/workspace/ITK-MinGW/ITK-build/"
    # install_dir = "F:/workspace/ITK-MinGW/install/"
    # Always use the above 2 paths for testing
    
    # Patch install files that use absolute paths
    # Attention use string format like "\\"
    # For path, we can use (for example) "C:/" instead of "C:\"(except in shell)

#Not yet needed. No particular requirement. 
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
    subprocess.check_call([
        "sed", "-i", 
        """s@\(set(possible_itk_dir\) ".*")@\\1 "/usr/lib/cmake/ITK-4.6")@i""",
        os.path.join(install_dir, "lib/cmake/ITK-4.6/WrapITK/WrapITKConfig.cmake")
    ])

    # Not Found: No need to Fix PYTHONPATH for pygccxml

    # Add missing files
    # * Wrapping/Typedefs/*SwigInterface.h -> usr/lib/cmake/ITK-4.6/WrapITK/Configuration/Typedefs/
    typedefs_source = os.path.join(binary_dir, "Wrapping", "Typedefs")
    typedefs_dest = os.path.join(install_dir, 
        "lib", "cmake", "ITK-4.6", "WrapITK", "Configuration", "Typedefs")
    for filename in glob.glob(os.path.join(typedefs_source, "*SwigInterface.h")):
        shutil.copy(filename, typedefs_dest)
