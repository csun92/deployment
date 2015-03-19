import argparse
import glob
import os
import shutil
import subprocess

def Directory(value):
    if not os.path.isdir(value):
        raise argparse.ArgumentTypeError("No such directory: '{}'".format(value))
    return os.path.abspath(value)

def setup_parser(parser):
    distributions = {
        "Ubuntu": ["precise", "trusty"],
        "Debian": ["wheezy"]
    }
    
    distributor = subprocess.check_output(["lsb_release", "--id", "--short"]).strip()
    if distributor not in distributions:
        raise Exception("Unknown distributor: '{}'".format(distributor))
    
    codename = subprocess.check_output(["lsb_release", "--codename", "--short"]).strip()
    if codename not in distributions[distributor]:
        raise Exception("Unknown version of {}: '{}'".format(distributor, codename))

    parser.add_argument("root", help="Root directory", type=Directory)
    parser.add_argument("-j", "--jobs", type=int, default=1,
        help="Number of parallel make jobs")
    
    if distributor in ["Ubuntu", "Debian"]:
        parser.add_argument("maintainer", 
            help="Package maintainer (e.g. John Doe <doe@example.com>)")

def run(upstream_version, local_revision, root, maintainer, jobs):
    
    old_umask = os.umask(0022)
    
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
        package(install_dir, upstream_version, local_revision, maintainer, root)
    
    except Exception, e:
        print "Could not {}: {}".format(step, e)
    
    os.umask(old_umask)

def download(version, directory):
    sha1 = {
        "4.6.1": "cc0b6461eed9d6182dae6b3431645c4530241086",
    }
    if version not in sha1:
        raise Exception("Unknown version: {}".format(version))
    
    archive = "InsightToolkit-{}.tar.xz".format(version)
    if not os.path.isfile(os.path.join(directory, archive)):
        url = "http://sourceforge.net/projects/itk/files/itk/{}/{}/download".format(
            version[:3], archive)
        subprocess.check_call([
            "wget", "--quiet", "-O", os.path.join(directory, archive), url])
    
        # Check signature
        subprocess.check_call("""echo "{}  {}" | sha1sum -c --quiet""".format(
            sha1[version], os.path.join(directory, archive)),
            shell=True)
    
    source_dir = os.path.join(directory, "InsightToolkit-{}".format(version))
    if not os.path.isdir(source_dir):
        subprocess.check_call([
            "tar", "-C", directory, "-J", "-x", "-f", os.path.join(directory, archive)])
    
    return source_dir

def configure(source_dir, build_dir):
    options = {}
    
    # General CMake options
    options.update({
        "BUILD_EXAMPLES": "OFF",
        "BUILD_SHARED_LIBS": "ON",
        "BUILD_TESTING": "OFF",
        "CMAKE_INSTALL_PREFIX": "/usr",
        "CMAKE_SKIP_RPATH": "ON"
    })
    
    # Use system tools as much as possible. HDF5 fails on Ubuntu 12.04
    options.update({
        "ITK_USE_SYSTEM_DCMTK": "ON",
        "ITK_USE_SYSTEM_DOUBLECONVERSION": "OFF",
        "ITK_USE_SYSTEM_GCCXML": "ON",
        "ITK_USE_SYSTEM_GDCM": "ON",
        "ITK_USE_SYSTEM_HDF5": "OFF",
        "ITK_USE_SYSTEM_JPEG": "ON",
        "ITK_USE_SYSTEM_PNG": "ON",
        "ITK_USE_SYSTEM_SWIG": "ON",
        "ITK_USE_SYSTEM_TIFF": "ON",
        "ITK_USE_SYSTEM_VXL": "OFF",
        "ITK_USE_SYSTEM_ZLIB": "ON",
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
        "unsigned_char": True, "unsigned_long": True, "unsigned_short": True,
    }
    for type_, value in types.items():
        options["ITK_WRAP_{}".format(type_)] = "ON" if value else "OFF"    
    
    # FFTW options
    architecture = subprocess.check_output(
        ["dpkg", "--print-architecture"]).strip()
    use_fftw = (architecture != "i386")
    fft_options = [
        "USE_FFTW", "ITK_USE_FFTWD", "ITK_USE_FFTWF", "ITK_USE_SYSTEM_FFTW"
    ]
    for option in fft_options:
        options[option] = "ON" if use_fftw else "OFF"
    
    # Miscelaneous options
    options.update({
        "ITK_USE_STRICT_CONCEPT_CHECKING": "ON",
        "ITKV3_COMPATIBILITY": "ON",
        "VCL_INCLUDE_CXX_0X": "ON"
    })
    
    options = ["-D{}={}".format(name, value) 
        for name, value in options.items()]
    
    if not os.path.isdir(build_dir):
        os.makedirs(build_dir)
    subprocess.check_call(["cmake"] + options + [source_dir],
        cwd=build_dir)

def build(binary_dir, jobs):
    subprocess.check_call(["make", "-j", str(jobs)], cwd=binary_dir)

def install(binary_dir, install_dir):
    install_dir = os.path.join(install_dir, "usr")
    
    # Patch install files that use absolute paths
    for dirpath, dirnames, filenames in os.walk(binary_dir):
        targets = [
            "install_wrapitk_compatibility.cmake", 
            "cmake_install.cmake"
        ]
        for target in targets:
            if target in filenames:
                filename = os.path.join(dirpath, target)
                
                before = """\(file(install destination "\)\(/usr\)"""
                after = """\\1{}\\2""".format(install_dir)
                script = "s@{}@{}@i".format(before, after)
            
                command = ["sed", "-i", script, filename]
            
                subprocess.check_call(command)
    
    # Install
    subprocess.check_call([
        "cmake", 
        "-DCMAKE_INSTALL_PREFIX:PATH={}".format(install_dir),
        "-P", os.path.join(binary_dir, "cmake_install.cmake")
    ])

    # Fix install in PREFIX/usr, since PREFIX is supposed to end with /usr
    subprocess.check_call([
        "rsync", "-a", 
        os.path.join(install_dir, "usr", ""),
        os.path.join(install_dir, "")
    ])
    subprocess.check_call([
        "rm", "-rf", os.path.join(install_dir, "usr")
    ])
    
    # Patch files with original install dir
    subprocess.check_call([
        "sed", "-i", 
        """s@\(set(possible_itk_dir\) ".*")@\\1 "/usr/lib/cmake/ITK-4.6")@i""",
        os.path.join(install_dir, "lib/cmake/ITK-4.6/WrapITK/WrapITKConfig.cmake")
    ])
    
    # FIXME? looks unused: include/ITK-4.6/vcl_where_root_dir.h
    
    # Patch files that force downloading external projects
    for generator, entry in [("SwigInterface", "SWIG"), ("GccXML", "GCCXML")]:
        grep_status = subprocess.call([
            "grep", "--quiet", 
            "^ITK_USE_SYSTEM_{}:BOOL=ON".format(entry), 
            os.path.join(binary_dir, "CMakeCache.txt")
        ])
        use_system = "ON" if grep_status == 0 else "OFF"
        
        filename = os.path.join(install_dir, "lib", "cmake", "ITK-4.6", 
            "WrapITK", "Configuration", "Generators", generator, 
            "CMakeLists.txt")
        pattern = r"""option(ITK_USE_SYSTEM_{} \(".*"\) .*)""".format(entry)
        replace = r"""option(ITK_USE_SYSTEM_{} \1 {})""".format(entry, use_system)
        subprocess.check_call([
            "sed", "-i", "s/{}/{}/".format(pattern, replace), filename])
    
    # Fix PYTHONPATH for pygccxml
    with open(os.path.join(install_dir, "lib/python2.7/dist-packages/WrapITK.pth"), "a") as fd:
        fd.write("/usr/lib/cmake/ITK-4.6/WrapITK/Configuration/Generators/SwigInterface/src\n")
    
    # Add missing files
    # * Wrapping/Typedefs/*SwigInterface.h -> usr/lib/cmake/ITK-4.6/WrapITK/Configuration/Typedefs/
    typedefs_source = os.path.join(binary_dir, "Wrapping", "Typedefs")
    typedefs_dest = os.path.join(install_dir, 
        "lib", "cmake", "ITK-4.6", "WrapITK", "Configuration", "Typedefs")
    for filename in glob.glob(os.path.join(typedefs_source, "*SwigInterface.h")):
        shutil.copy(filename, typedefs_dest)
    
def package(install_dir, upstream_version, local_revision, maintainer, root):
    
    architecture = subprocess.check_output(
        ["dpkg", "--print-architecture"]).strip()
    
    codename = subprocess.check_output(["lsb_release", "--codename", "--short"]).strip()
    local_revision = "{}~{}".format(local_revision, codename)
    
    versions = ["3.18", "3.20", "4.3", "4.5", "4.6"]
    conflicts = ["libinsighttoolkit{}".format(x) for x in versions]
    
    depends = [
        "cmake", 
        "swig (>= 2.0)", 
        "gccxml (>= 0.9.0+git20140716)",
        "zlib1g-dev (>= 1.2.2)", 
        "libpng12-dev", 
        "libtiff-dev",
        "libfftw3-dev",
        "libdcmtk2-dev", 
        "libgdcm2-dev", 
        "uuid-dev", 
        "libminc-dev", 
        #"libhdf5-dev", 
        "python-all-dev", 
        "libvtk5-dev", 
        "python-vtk"

    ]
    
    data = {
        "package": "insighttoolkit",
        "version": "{}-{}".format(upstream_version, local_revision),
        "architecture": architecture,
        "maintainer": maintainer,
        "depends": ", ".join(depends),
        "conflicts": ", ".join(conflicts),
    }
    
    control = """Package: {package}
Version: {version}
Section: science
Priority: extra
Architecture: {architecture}
Maintainer: {maintainer}
Depends: {depends}
Conflicts: {conflicts}
Homepage: http://www.itk.org/
Description: Image processing toolkit for registration and segmentation
 ITK is an open-source software toolkit for performing registration and
 segmentation. Segmentation is the process of identifying and
 classifying data found in a digitally sampled
 representation. Typically the sampled representation is an image
 acquired from such medical instrumentation as CT or MRI
 scanners. Registration is the task of aligning or developing
 correspondences between data. For example, in the medical
 environment, a CT scan may be aligned with a MRI scan in order to
 combine the information contained in both.

""".format(**data)
    
    if not os.path.isdir(os.path.join(install_dir, "DEBIAN")):
        os.makedirs(os.path.join(install_dir, "DEBIAN"))
        os.chmod(os.path.join(install_dir, "DEBIAN"), 0775)
    
    with open(os.path.join(install_dir, "DEBIAN", "control"), "w") as fd:
        fd.write(control)
    
    subprocess.check_call([
        "fakeroot", "dpkg-deb", "-b", install_dir, 
        os.path.join(root, "{}_{}_{}.deb".format(
            data["package"], data["version"], data["architecture"]))
    ])
