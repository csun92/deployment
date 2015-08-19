import os

def check(cmd, mf):
    m = mf.findNode('medipy')
    if m is None or m.filename is None:
        return None
    else:
        import cStringIO
        medipy_Path = cStringIO.StringIO()
        medipy_Path.write("""_parent = '/'.join(__file__.split('/')[:-1])\n""")
        medipy_Path.write('medipy_lib_path = os.path.join(_parent, "lib", "python2.7", "lib")\n')
        medipy_Path.write('plugins_path = os.path.join(_parent, "lib", "python2.7", "plugins")\n')
        medipy_Path.write('sys.path.append(medipy_lib_path)\n')
        medipy_Path.write('os.environ["MEDIPY_PLUGINS_PATH"] = plugins_path\n')
        return dict(
            prescripts = [medipy_Path],
        )
