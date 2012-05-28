# for debugging
def __bootstrap__():
    global __bootstrap__, __file__
    import sys, imp
    from os.path import join, dirname
    from distutils.util import get_platform
    from distutils.sysconfig import get_config_var
    build_base = join(dirname(dirname(dirname(__file__))), 'build')
    plat_specifier = ".%s-%s" % (get_platform(), sys.version[0:3])
    build_platlib = join(build_base, 'lib' + plat_specifier)
    __file__ = join(build_platlib, 'spike', 'ext' + get_config_var('SO'))
    del __bootstrap__
    imp.load_dynamic(__name__, __file__)
__bootstrap__()
