import os
import shutil

source = "F:/medipy"
dest = "F:/medipy_install"

#copy test files
tests_dir = dest + '/lib/tests'
tests_src = source + '/lib/tests'
shutil.copytree(tests_src, tests_dir)

#copy all other necessary files
valid_exts = [None,'py','pyd','dll','lib', 'xrc', 'png']
for dirpath, dirnames, filenames in os.walk(source):
    for each in filenames:
        if len(each.split('.',1)) == 1:
            extension = None
        else:
            extension = each.split('.',1)[1]
        if extension in valid_exts:
            src = os.path.join(dirpath,each)
            dst = dest + dirpath.replace(source, "")
            if not os.path.exists(dst):
                os.makedirs(dst)
            shutil.copy(src, dst)
        

