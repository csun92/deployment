import os

medipy_dir = "/Users/yosemiteretail/workspace/medipy"
exts = ['cmake', 'txt']
origin = "    itk_end_wrap_submodule()\n"
repl = ""
file_number = 0
check_number = 0
extension = ""

for dirpath,dirnames,filenames in os.walk(medipy_dir):
	for filename in filenames:
		if len(filename.split('.',1)) == 1:
			extension = None
		else:
			extension = filename.split('.',1)[1]
		if extension in exts:
			stat = 0
			dest = os.path.join(dirpath, filename)
			with open(dest, "r+") as f:
				data = f.read()
				while True:
					if data.find(origin) != -1:
						check_number+=1
						data = data.replace(origin, repl, 1)
						if stat == 0:
							file_number+=1
							stat =1
					else:
						break
				f.seek(0)
				f.truncate(0)
				f.write(data)
				
print "find " + str(check_number) +" objects in " + str(file_number) + " files"
