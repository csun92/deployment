import zipfile
import argparse
import re
import os

description="""\nUse -p to specify python path, use -c to specify cmake path\n
The default python path is: C:/tools/python2/\n
The default cmake path is: C:/tools/python2/\n
Attention the format of path!\n
For example: prog -p C:/python27/ specify a new path for Python.\n"""

dest_cmake = "F:/Program Files/CMake" #Default cmake path
dest_python = "F:/tools/python2" #Default python path

parser = argparse.ArgumentParser(description)
parser.add_argument('-p', '--pypath', type=str, help = 'The path of python')
parser.add_argument('-c', '--cmakepath', type=str, help = 'The path of CMake')
args = parser.parse_args()
if args.pypath != None:
    dest_python = args.pypath
if args.cmakepath != None:
    dest_cmake = args.cmakepath

# This function is to fix path format
# For example / or \ or something like this c:/tools//python27 
def comb_path(part1,part2):
    path = part1 + part2
    path = path.replace("\\", "/")
    path = path.replace("//", "/")
    return path
               
def unpack():
    package_name = "ITK-4.6.zip"
    dest = "F:/" #Always absolute path, do not change
    # Attention cmake path is considered by default "C:/Program Files/CMake/"
    pattern_cmake = re.compile(r'Program Files/CMake/', re.I)
    pattern_py = re.compile(r'tools/python2/', re.I)
    with zipfile.ZipFile(package_name, 'r') as itkpack:
        filelist = itkpack.namelist()
        for member in filelist:
            if re.search(pattern_cmake, member):
                fixpath = member.replace("Program Files/CMake/","/")
                fullpath = comb_path(dest_cmake, fixpath)
            elif re.search(pattern_py, member):
                fixpath = member.replace("tools/python2/","/")
                fullpath = comb_path(dest_python, fixpath)
            else:
                fullpath = os.path.join(dest, member)
            directory = fullpath.rsplit("/",1)[0]
            if not os.path.exists(directory):
                os.makedirs(directory)
            f_handle = open(fullpath, "wb")
            f_handle.write(itkpack.read(member))
            f_handle.close()

unpack()
# Notes:
# To avoid import error: ImportError: DLL load failed: ...
# In terminal, add C:\ITK-4.6\bin (dll files location) to system path
# command: set PATH=C:\ITK-4.6\bin:%PATH%

    
