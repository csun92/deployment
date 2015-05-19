import argparse
import glob
import os
import shutil
import subprocess
import re
import zipfile
import hashlib
import urllib2


def Directory(value):
    if not os.path.isdir(value):
        raise argparse.ArgumentTypeError("No such directory: '{}'".format(value))
    return os.path.abspath(value)

def setup_parser(parser):
    parser.add_argument("root", help="Root directory", type=Directory)
    parser.add_argument("-j", "--jobs", type=int, default=1,
        help="Number of parallel make jobs")
    parser.add_argument("maintainer", 
        help="Package maintainer (e.g. John Doe <doe@example.com>)")
    
def run(upstream_version, local_revision, root, maintainer, jobs):
    
    try:
        step = "download"
        source_dir = download(upstream_version, root)
        
        step = "configure"
        binary_dir = os.path.join(root, "build")
        if not os.path.isfile(os.path.join(binary_dir, "CMakeCache.txt")):
            configure(source_dir, binary_dir)
        
        step = "build"
        build(binary_dir, jobs)
        
        step = "install"
        install_dir = os.path.join(root, "install")
        install(binary_dir, install_dir)
        
        step = "package"
        package(install_dir)
    
    except Exception, e:
        print "Could not {}: {}".format(step, e)
    

def download(version, directory):

    sha1 = {
        "4.6.1": "3074e324f786339947dc002d0ec883a357e48825",
    }

    if version not in sha1:
        raise Exception("Unknown version: {}".format(version))

    archive = "InsightToolkit-{}.zip".format(version)
    if not os.path.isfile(os.path.join(directory, archive)):
        url = "http://sourceforge.net/projects/itk/files/itk/{}/{}/download".format(
            version[:3], archive)
        source = urllib2.urlopen(url)
        output = open(os.path.join(directory, archive),'wb')
        output.write(source.read())
        output.close()

        #check signature
        BLOCKSIZE = 65536
        hasher = hashlib.sha1()
        with open(archive, 'rb') as pkg:
            buf = pkg.read(BLOCKSIZE)
            while len(buf) > 0:
                hasher.update(buf)
                buf = pkg.read(BLOCKSIZE)
        print hasher.hexdigest()
        
    source_dir = os.path.join(directory, "InsightToolkit-{}".format(version))
    if not os.path.isdir(source_dir):
        with zipfile.ZipFile(archive, 'r') as pkg:
            pkg.extractall(directory)

    return source_dir

#Attention, DCMTK must be installed before configure.
#If not, set ITK_USE_SYSTEM_DCMTK: OFF(might cause compile errors)
def configure(source_dir, build_dir):
    options = {}
    
    # General CMake options
    options.update({
        "BUILD_EXAMPLES": "OFF",
        "BUILD_SHARED_LIBS": "ON",
        "BUILD_TESTING": "OFF",
        "CMAKE_INSTALL_PREFIX": "C:/Program Files/ITK",
        "CMAKE_SKIP_RPATH": "ON"
    })
    
    # Use system tools as much as possible.
    options.update({
        "ITK_USE_SYSTEM_DCMTK": "ON"
    })
    
    # Modules
    modules = {
        "DCMTK": True, "Deprecated": True, "IOPhilipsREC": False,
        "LevelSetsv4Visualization": False, "Review": True,
        "VideoBridgeOpenCV": False, "VideoBridgeVXL": False,
        "VtkGlue": False
    }
    for module, value in modules.items():
        options["Module_ITK{}".format(module)] = "ON" if value else "OFF"
    
    # Wrapping
    options.update({
        "ITK_WRAPPING": "ON",
        "ITK_WRAP_PYTHON": "ON",
        "ITK_WRAP_DIMS": "2;3"
    })
    types = {
        # Float types
        "double": False, "float": True,
        "complex_double": False, "complex_float": True,
        "vector_double": False, "vector_float": True,
        "covariant_vector_double": False, "covariant_vector_float": True,
        # RGB types
        "rgb_unsigned_char": True, "rgb_unsigned_short": False,
        "rgba_unsigned_char": True, "rgba_unsigned_short": False,
        # Integer types
        "signed_char": True, "signed_long": True, "signed_short": True,
        "unsigned_char": True, "unsigned_long": True, "unsigned_short": True
    }
    for type_, value in types.items():
        options["ITK_WRAP_{}".format(type_)] = "ON" if value else "OFF"     
    
    # Miscelaneous options
    options.update({
        "ITK_USE_STRICT_CONCEPT_CHECKING": "ON",
        "ITKV3_COMPATIBILITY": "ON",
        "VCL_INCLUDE_CXX_0X": "ON"
    })
    
    options = ["-D{}={}".format(name, value) 
        for name, value in options.items()]
    #specify MinGW as generator(instead of VS)
    generator = "-GMinGW Makefiles"
    
    if not os.path.isdir(build_dir):
        os.makedirs(build_dir)
    subprocess.check_call(["cmake"] + options + [generator] + [source_dir],
        cwd=build_dir)

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

    # Patch files that force downloading external projects
    for generator, entry in [("SwigInterface", "SWIG"), ("GccXML", "GCCXML")]:        
        filename = os.path.join(install_dir, "lib", "cmake", "ITK-4.6", 
            "WrapITK", "Configuration", "Generators", generator, 
            "CMakeLists.txt")
        pattern = re.compile(r'option\(ITK_USE_SYSTEM_{} .* OFF\)'.format(entry))
        with open(filename,"r+") as fs:
            before = fs.read()
            result = re.findall(pattern, before)
            for each in result:
                repl = each.replace("OFF)","ON)")
                after = re.sub(pattern, repl, before)
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

    
