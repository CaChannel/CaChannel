#!/usr/bin/env python
"""
Setup file for Ca-Python using distutils package.
Python2.4 or later should be used.
"""

import os
import sys
import platform

# Use setuptools to include build_sphinx, upload/sphinx commands
try:
    from setuptools import setup
except:
    pass

from distutils.core import setup, Extension

EPICSBASE=os.path.join(os.getcwd(), 'epicsbase')

try:
    UNAME=platform.uname()[0]
    ARCH=platform.architecture()[0]
except:
    UNAME="Unknown"
    ARCH="Unknown"

lflags=[]
dlls=[]
if UNAME.lower() == "windows":
    UNAME="WIN32"
    if ARCH=="64bit":
        HOSTARCH="windows-x64"
    else:
        HOSTARCH="win32-x86"
    if sys.hexversion >= 0x02060000 and sys.hexversion < 0x03030000:
        HOSTARCH += "-vc9"
    elif sys.hexversion >= 0x03030000:
        HOSTARCH += "-vc10"
    lflags+=['/LTCG', '/NODEFAULTLIB:libcmt.lib',]
    libraries=["ca","Com","ws2_32","msvcrt","user32", "advapi32"]
elif UNAME.lower() == "darwin":
    HOSTARCH = 'darwin-x86'
    lflags+=['-stdlib=libstdc++',]
    libraries=["ca","Com","readline"]
elif UNAME.lower() == "linux":
    if ARCH=="64bit":
        HOSTARCH="linux-x86_64"
    else:
        HOSTARCH="linux-x86"
    libraries=["ca","Com","readline","rt"]
else:
    print("Platform", UNAME, ARCH, " Not Supported")
    sys.exit(1)

rev="2.3.0"

define_macros = [("PYCA_VERSION",'"\\"%s\\""'%rev), (UNAME, None)]
include_dirs = [os.path.join(EPICSBASE,"include"),
                os.path.join(EPICSBASE,"include", "os",UNAME),
                os.path.join(EPICSBASE,"include", "os"),
                ]

# guess numpy path
WITH_NUMPY = True
try:
    import numpy
except:
    WITH_NUMPY = False
else:
    numpy_header = os.path.join(os.path.dirname(numpy.__file__),'core','include')
    if not os.path.exists(os.path.join(numpy_header,'numpy','arrayobject.h')):
        WITH_NUMPY = False

if WITH_NUMPY:
    define_macros += [('WITH_NUMPY', None)]
    include_dirs += [numpy_header]

CA_SOURCE="src/_ca314.cpp" # for threaded version.
ca_module = Extension("_ca",[CA_SOURCE],
                      include_dirs=include_dirs,
                      define_macros=define_macros,
                      undef_macros=["_DLL"],      
                      extra_link_args = lflags,
                      libraries=libraries,
                      library_dirs=[os.path.join(EPICSBASE,"lib",HOSTARCH),])

if UNAME != "WIN32":
    ca_module.runtime_library_dirs=[os.path.join(EPICSBASE,"lib",HOSTARCH),]

setup(name="CaChannel",
      version=rev,
      author="Xiaoqiang Wang",
      author_email = "xiaoqiang.wang AT psi DOT ch",
      description="CaChannel Interface to EPICS",
      long_description=open('README').read(),
      url="http://cachannel.googlecode.com",
      license     = "BSD",
      platforms   = ["Windows","Linux", "Mac OS X"],
      classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Environment :: Console',
          'Intended Audience :: Developers',
          'Programming Language :: C',
          'Programming Language :: Python :: 2',
          'License :: OSI Approved :: BSD License',
          'Topic :: Scientific/Engineering',
      ],
      package_dir={"": "src"},
      py_modules=["ca", "caError", "cadefs","_ca_kek", "_ca_fnal", "CaChannel"],
      ext_modules=[ca_module,],
      data_files = [('', dlls)]
)
