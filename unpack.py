import zipfile
               
def unpack():
    package_name = "ITK-4.6.zip"
    dest = "C:/" #Always absolute path
    # Attention cmake path is considered by default "C:/Program Files/CMake/"
    # pattern_prog = re.compile(r'Program Files/', re.I)
    # pattern_py = re.compile(r'tools/', re.I)
    with zipfile.ZipFile(package_name, 'r') as itkpack:
        filelist = itkpack.namelist()
        for member in filelist:
            itkpack.extract(member, dest)

# Notes:
# To avoid import error: ImportError: DLL load failed: ...
# In terminal, add C:\ITK-4.6\bin (dll files location) to system path
# command: set PATH=C:\ITK-4.6\bin:%PATH%

    
