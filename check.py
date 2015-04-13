import hashlib
import argparse

BLOCKSIZE = 65536
filename = ""

parser = argparse.ArgumentParser()
group = parser.add_mutually_exclusive_group()
group.add_argument("--md5", type=str, help="Calculate md5 value.")
group.add_argument("--sha1", type=str, help="Calculate sha1 value.")
args = parser.parse_args()

if args.md5 != None:
    filename = args.md5
    hasher = hashlib.md5()
elif args.sha1 != None:
    filename = args.sha1()
else:
     raise Exception("Filename must be non vide.")

with open(filename, 'rb') as fichier:
    buf = fichier.read(BLOCKSIZE)
    while len(buf) > 0:
        hasher.update(buf)
        buf = fichier.read(BLOCKSIZE)
    print hasher.hexdigest()


