#!/usr/bin/env python
"""
Setup file for Ca-Python using distutils package.
Python2.6 or later should be used.
"""

import os
import sys
import platform
import imp
import shutil
import subprocess
import warnings

# Use setuptools to include build_sphinx, upload/sphinx commands
try:
    from setuptools import setup, Extension
except:
    from distutils.core import setup, Extension

build_ca_ext=True
# define EPICS base path and host arch
EPICSBASE = os.environ.get("EPICS_BASE")
HOSTARCH = os.environ.get("EPICS_HOST_ARCH")
SHARED = os.environ.get("EPICS_SHARED")
# guess from EPICS root environment variable
if not EPICSBASE:
    EPICSROOT = os.environ.get("EPICS")
    if EPICSROOT:
        EPICSBASE = os.path.join(EPICSROOT, 'base')

if not EPICSBASE or not os.path.exists(EPICSBASE) or not HOSTARCH:
    build_ca_ext = False
    warnings.warn("""
    !!! No valid EPICS_BASE, EPICS_HOST_ARCH environment variables are defined !!!
    !!!                 Switch to caffi as backend                             !!!
    """)


def create_exension():
    global EPICSBASE, HOSTARCH
    umacros = []
    macros = []
    cflags = []
    lflags = []
    dlls = []
    extra_objects = []
    libraries = ["ca", "Com"]
    CMPL = 'gcc'
    UNAME = platform.system()
    ARCH = platform.architecture()[0]
    # platform dependent libraries and macros
    if UNAME.lower() == "windows":
        UNAME = "WIN32"
        static = False
        if HOSTARCH in ['win32-x86', 'windows-x64', 'win32-x86-debug', 'windows-x64-debug']:
            if not SHARED:
                dlls = ['Com.dll', 'ca.dll']
                for dll in dlls:
                    dllpath = os.path.join(EPICSBASE, 'bin', HOSTARCH, dll)
                    if not os.path.exists(dllpath):
                        static = True
                        break
                    shutil.copy(dllpath,
                                os.path.join(os.path.dirname(os.path.abspath(__file__)), 'src', 'CaChannel'))
            macros += [('_CRT_SECURE_NO_WARNINGS', 'None'), ('EPICS_CALL_DLL', '')]
            cflags += ['/Z7']
            CMPL = 'msvc'
        if HOSTARCH in ['win32-x86-static', 'windows-x64-static'] or static:
            libraries += ['ws2_32', 'user32', 'advapi32']
            macros += [('_CRT_SECURE_NO_WARNINGS', 'None'), ('EPICS_DLL_NO', '')]
            umacros += ['_DLL']
            cflags += ['/EHsc', '/Z7']
            lflags += ['/LTCG']
            if HOSTARCH[-5:] == 'debug':
                libraries += ['msvcrtd']
                lflags += ['/NODEFAULTLIB:libcmtd.lib']
            else:
                libraries += ['msvcrt']
                lflags += ['/NODEFAULTLIB:libcmt.lib']
            CMPL = 'msvc'
        # GCC compiler
        if HOSTARCH in ['win32-x86-mingw', 'windows-x64-mingw']:
            macros += [('_MINGW', ''), ('EPICS_DLL_NO', '')]
            lflags += ['-static']
            CMPL = 'gcc'
        if HOSTARCH == 'windows-x64-mingw':
            macros += [('MS_WIN64', '')]
            CMPL = 'gcc'
    elif UNAME.lower() == "darwin":
        CMPL = 'clang'
        HOSTARCH = 'darwin-x86'
        if not SHARED:
            extra_objects = [os.path.join(EPICSBASE, 'lib', HOSTARCH, 'lib%s.a' % lib) for lib in libraries]
            libraries = []
    elif UNAME.lower() == "linux":
        CMPL = 'gcc'
        if not SHARED:
            extra_objects = [os.path.join(EPICSBASE, 'lib', HOSTARCH, 'lib%s.a' % lib) for lib in libraries]
            libraries = ['rt']
            if subprocess.call('nm %s | grep -q rl_' % os.path.join(EPICSBASE, 'lib', HOSTARCH, 'libCom.a'), shell=True) == 0:
                libraries += ['readline']
    else:
        print("Platform", UNAME, ARCH, " Not Supported")
        sys.exit(1)

    include_dirs = [os.path.join(EPICSBASE, "include"),
                    os.path.join(EPICSBASE, "include", "os", UNAME),
                    os.path.join(EPICSBASE, "include", "compiler", CMPL),
                    ]

    # guess numpy path
    try:
        import numpy
    except:
        warnings.warn('numpy is not present. Large array read could suffer from low efficiency.')
    else:
        macros += [('WITH_NUMPY', None)]
        include_dirs += [numpy.get_include()]

    ca_module = Extension('CaChannel._ca',
                          sources=['src/CaChannel/_ca.cpp'],
                          extra_compile_args=cflags,
                          include_dirs=include_dirs,
                          define_macros=macros,
                          undef_macros=umacros,
                          extra_link_args=lflags,
                          extra_objects=extra_objects,
                          libraries=libraries,
                          library_dirs=[os.path.join(EPICSBASE, "lib", HOSTARCH)])

    if UNAME == "Linux" and SHARED:
        ca_module.runtime_library_dirs = [os.path.join(EPICSBASE, "lib", HOSTARCH)]

    return [ca_module], dlls

_version = imp.load_source('_version', 'src/CaChannel/_version.py')

if build_ca_ext:
    ext_module, package_data = create_exension()
    requirements = []
    if sys.hexversion < 0x03040000:
        requirements += ['enum34']
else:
    ext_module = []
    package_data = []
    requirements = ['caffi']

setup(name="CaChannel",
      version=_version.__version__,
      author="Xiaoqiang Wang",
      author_email="xiaoqiang.wang@psi.ch",
      description="CaChannel Interface to EPICS",
      long_description=open('README.rst').read(),
      url="http://pypi.python.org/pypi/cachannel",
      license="BSD",
      platforms=["Windows", "Linux", "Mac OS X"],
      classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Environment :: Console',
          'Intended Audience :: Developers',
          'Programming Language :: C',
          'Programming Language :: Python :: 2',
          'Programming Language :: Python :: 2.6',
          'Programming Language :: Python :: 2.7',
          'Programming Language :: Python :: 3',
          'License :: OSI Approved :: BSD License',
          'Topic :: Scientific/Engineering',
      ],
      packages=["CaChannel"],
      package_dir={"": "src", "CaChannel": "src/CaChannel"},
      py_modules=["ca", "epicsPV", "epicsMotor"],
      ext_modules=ext_module,
      package_data={'CaChannel': package_data},
      install_requires=requirements
      )
