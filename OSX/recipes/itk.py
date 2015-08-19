import os

def check(cmd, mf):
    m = mf.findNode('itk')
    if m is None or m.filename is None:
        return None
    else:
        import itkTypes
        wrapitk_root = os.path.dirname(os.path.dirname(itkTypes.__file__))
        import cStringIO
        addPath = cStringIO.StringIO()
        addPath.write("""_parent = '/'.join(__file__.split('/')[:-1])\n""")
        addPath.write('WrapITK_Path = os.path.join(_parent, "lib", "python2.7", "ITK-4.6", "Python")\n')
        addPath.write('sys.path.append(WrapITK_Path)\n')
        return dict(
            packages = [wrapitk_root],
            prescripts = [addPath],
        )
