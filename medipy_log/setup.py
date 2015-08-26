##########################################################################
# MediPy - Copyright (C) Universite de Strasbourg, 2011-2012
# Distributed under the terms of the CeCILL-B license, as published by
# the CEA-CNRS-INRIA. Refer to the LICENSE file or to
# http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
# for details.
##########################################################################

import distutils.core
import glob
import imp
import os
import os.path
import re
import shutil
import subprocess
import sys
import platform
import _winreg
import matplotlib

import sys
sys.setrecursionlimit(5000)

import py2exe
import py2exe.mf

import medipy

def get_build_name():
    """ Return the build name, based on the platform and on the SVN revision.
    """

    command = ["hg", "tip", "--template", "{rev}"]
    revision = subprocess.Popen(command, stdout=subprocess.PIPE).communicate()[0]

    build_name = "%s-r%s"%(sys.platform, revision)

    return build_name

def patch_load_package(self, fqname, pathname):
    m = self.original_load_package(fqname, pathname)
    if fqname.startswith("medipy") :
        # See if there is a wrapitk module
        data = open(m.__file__).readlines()
        for line in data :
            match = re.match(r""".*load_wrapitk_module\(.*, "(.*)"\)""", line)
            if match :
                self.import_hook("{1}Config".format(fqname, match.group(1)))
                self.import_hook("{1}Python".format(fqname, match.group(1)))
    return m
py2exe.mf.ModuleFinder.original_load_package = py2exe.mf.ModuleFinder.load_package
py2exe.mf.ModuleFinder.load_package = patch_load_package

def patch_find_module(self, name, path, parent=None) :
    try :
        result = self.original_find_module(name, path, parent)
    except :
        # Look for a MediPy plugin
        return imp.find_module(
            name, os.environ["MEDIPY_PLUGINS_PATH"].split(os.pathsep))
    else :
        return result
py2exe.mf.ModuleFinder.original_find_module = py2exe.mf.ModuleFinder.find_module
py2exe.mf.ModuleFinder.find_module = patch_find_module

def setup(project_name, main_script, includes=None, medipy_plugins=None):

    includes = includes or []
    medipy_plugins = medipy_plugins or []
    if(platform.architecture()[0] == '32bit'):
        key = _winreg.OpenKey(
            _winreg.HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\VisualStudio\\9.0\\Setup\\VC")
    elif(platform.architecture()[0] == '64bit'):
        key = _winreg.OpenKey(
            _winreg.HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\9.0\\Setup\\VC")
    vs_root, key_type = _winreg.QueryValueEx(key, "ProductDir")
    key.Close()

    bin_directory = os.path.join("bin", "%s-%s"%(project_name, get_build_name()))

    # Remove destination dir if it exists
    if os.path.exists(bin_directory) :
        shutil.rmtree(bin_directory)

    # Process specific includes
    if "itk" in includes :
        import itk
        import itkTypes
        wrapitk_root = os.path.dirname(os.path.dirname(itkTypes.__file__))
        itk_root = 'C:/Program Files/ITK'
        for file in os.listdir(os.path.join(wrapitk_root, "Python")) :
            # Only load "library" modules
            if not file.startswith("_") and not file.startswith("itk") :
                swig_module = os.path.splitext(file)[0]
                if swig_module not in includes :
                    includes.append(swig_module)
        # Copy configuration files from WrapITK
        configuration_dir = os.path.join(wrapitk_root, "Python", "Configuration")
        shutil.copytree(configuration_dir, os.path.join(bin_directory, "Configuration"))
    if "openopt" in includes :
        includes.append("openopt.kernel")
        # import directives in the __init__.py file
        init_includes = ["GUI", "oologfcn", "nonOptMisc", "mfa"]
        includes.extend(["openopt.kernel.{0}".format(x) for x in init_includes])

        # import directives in the oo.py file
        oo_includes = ["LP", "LCP", "EIG", "SDP", "QP", "MILP", "STAB", "MCP",
                       "TSP", "NSP", "NLP", "MOP", "MINLP", "NLSP", "NLLSP",
                       "GLP", "SLE", "LLSP", "MMP", "LLAVP", "LUNP", "SOCP",
                       "DFP", "IP", "ODE"]
        includes.extend(["openopt.kernel.{0}".format(x) for x in oo_includes])

        import openopt
        solvers = ["openopt.solvers.{0}".format(solver)
                   for solver in openopt.kernel.nonOptMisc.solverPaths.values()]
        includes.extend(solvers)

    includes.extend(["medipy.{0}".format(plugin) for plugin in medipy_plugins])

    # Include Visual C runtime DLL
    data_files = [("Microsoft.VC90.CRT",
        glob.glob(os.path.join(vs_root,
                               "redist", "x86", "Microsoft.VC90.CRT", "*.*")))]

    # Include matplotlib data files
    data_files.extend(matplotlib.get_py2exe_datafiles())

    # Include the missing modules
    includes.remove('Configuration')
    includes.append('scipy')
    includes.append('scipy.integrate')
    includes.append('scipy.sparse.csgraph._validation')
    includes.append('scipy.special._ufuncs_cxx')

    # Main setup script
    sys.path.append(os.path.join(wrapitk_root, "lib"))
    distutils.core.setup(
        name = project_name,
        windows = [main_script],
        data_files=data_files,
        options = {
            "py2exe" : {
                "includes" : includes,
                "dist_dir" : str(bin_directory), # py2exe does not like unicode strings
                "verbose" : False,
                "excludes" : ["Tkconstants","Tkinter","tcl"],
                "packages" : ["gzip"],
                "skip_archive" : True,
                "custom_boot_script" : "envr.py"
            }
        },
    )
    sys.path.pop()

    # Copy solvers *_oo.py, explicitely required by openopt
    if "openopt" in includes :
        import openopt.solvers
        solvers = ["openopt.solvers.{0}".format(solver)
                   for solver in openopt.kernel.nonOptMisc.solverPaths.values()]
        solvers_root = os.path.dirname(openopt.solvers.__file__)
        for dirpath, dirnames, filenames in os.walk(solvers_root):
            for filename in filenames:
                if filename.endswith('_oo.py'):
                    destination = os.path.join(
                        bin_directory, "openopt", "solvers",
                        dirpath[1+len(solvers_root):], filename)
                    if not os.path.isdir(os.path.dirname(destination)) :
                        os.makedirs(os.path.dirname(destination))
                    shutil.copy(os.path.join(dirpath, filename), destination)

    # Copy resources
    roots = [os.path.join(medipy.__path__[0], "resources")]
    plugins = []
    for name in medipy_plugins :
        plugin = getattr(__import__("medipy.{0}".format(name)), name)
        plugins.append(plugin)
        roots.append(os.path.join(plugin.__path__[0], "resources"))
    roots.append("resources")

    for root in roots :
        print root[:-len("resources")-1], medipy.__path__[0]
        for dirpath, dirnames, filenames in os.walk(root) :
            if ".svn" in dirnames :
                del dirnames[dirnames.index(".svn")]
            for filename in filenames :
                if filename == "SConstruct" or filename.endswith(".fbp") :
                    continue

                source = os.path.join(dirpath, filename)
                if root[:-len("resources")-1] == medipy.__path__[0] :
                    destination_dir = os.path.join("medipy", "resources")
                elif root[:-len("resources")-1] in [x.__path__[0] for x in plugins] :
                    plugin = [x for x in plugins
                              if x.__path__[0]==root[:-len("resources")-1]][0]
                    destination_dir = os.path.join("medipy", plugin.__name__, "resources")
                else :
                    destination_dir = "resources"

                destination_dir = os.path.join(
                    bin_directory, destination_dir, dirpath[len(root)+1:])
                if not os.path.isdir(destination_dir) :
                    os.makedirs(destination_dir)

                destination = os.path.join(destination_dir, filename)
                print "copying {0} -> {1}".format(source, destination)
                shutil.copyfile(source, destination)
