"""
Usage:
    python setup.py py2app
"""
import traceback
import os

includes = []
packages = []
plugins_root = os.environ["MEDIPY_PLUGINS_PATH"]
medipy_plugins = [entry
                  for entry in os.listdir(plugins_root)
                  if os.path.isdir(os.path.join(plugins_root, entry)) and
                     os.path.isfile(os.path.join(plugins_root, entry, "api.py")) and
                     entry != "medimax"
                 ]

if "hsqc" in medipy_plugins :
    # TODO : move this to the plugin directory
    packages.extend(["openopt"])


import medipy
medipy_lib_path = medipy.__path__[0].rsplit('/',1)[0]
plugins_path = medipy.__path__[1]
medipy_app_path = os.path.dirname(medipy.__file__)

packages.extend([medipy_lib_path, plugins_path])

from setuptools import setup

try:
    APP = ['medipy']
    DATA_FILES = []
    OPTIONS = {'argv_emulation': True, 'includes' : includes, 'resources' : ["resources"],
    'packages': packages }

    setup(
        app=APP,
        data_files=DATA_FILES,
        options={'py2app': OPTIONS},
        setup_requires=['py2app'],
    )
except:
    traceback.print_exc()
