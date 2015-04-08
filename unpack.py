import zipfile
               
def unpack():
    package_name = "ITK-4.6.zip"
    dest = "C:/" #Always absolute path
    dest_cmake = "C:/Program Files/CMake/" #Default cmake path
    dest_python = "C:/tools/python2/" #Default python path
    # Attention cmake path is considered by default "C:/Program Files/CMake/"
    pattern_cmake = re.compile(r'/Program Files/CMake/', re.I)
    pattern_py = re.compile(r'/tools/python2/', re.I)
    with zipfile.ZipFile(package_name, 'r') as itkpack:
        filelist = itkpack.namelist()
        for member in filelist:
            if re.search(pattern_cmake, member):
                fixpath = member.replace("/Program Files/CMake/","/")
                fullpath = os.path.join(dest_cmake, fixpath)
            elif re.search(pattern_py, member):
                fixpath = member.replace("/tools/python2/","/")
                fullpath = os.path.join(dest_py, fixpath)
            else:
                fullpath = os.path.join(dest, member)
            directory = fullpath.rsplit("/",1)[0]
            if not os.path.exists(directory):
                os.makedirs(directory)
            f_handle = open(fullpath, "wb")
            f_handle.write(itkpack.read(member))
            f_handle.close()

# Notes:
# To avoid import error: ImportError: DLL load failed: ...
# In terminal, add C:\ITK-4.6\bin (dll files location) to system path
# command: set PATH=C:\ITK-4.6\bin:%PATH%

    
