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
if UNAME.lower() == "windows":
    UNAME="WIN32"
    if ARCH=="64bit":
        HOSTARCH="windows-x64"
    else:
        HOSTARCH="win32-x86"
    lflags+=['/LTCG', '/NODEFAULTLIB:libcmt.lib',]
    libraries=["ca","Com","ws2_32","msvcrt","user32", "advapi32"]
elif UNAME.lower() == "darwin":
    HOSTARCH = 'darwin-x86'
    libraries=["ca","Com","readline"]
elif UNAME.lower() == "linux":
    if ARCH=="64bit":
        HOSTARCH="linux-x86_64"
    else:
        HOSTARCH="linux-x86"
    libraries=["ca","Com","readline","rt"]
else:
    print "Platform", UNAME, ARCH, " Not Supported"
    sys.exit(1)

rev="2.2.0"
CA_SOURCE="src/_ca314.cpp" # for threaded version.
ca_module = Extension("_ca",[CA_SOURCE],
                      include_dirs=[os.path.join(EPICSBASE,"include"),
                                    os.path.join(EPICSBASE,"include", "os",UNAME),
                                    os.path.join(EPICSBASE,"include", "os"),],
                      define_macros=[("PYCA_VERSION",'"\\"%s\\""'%rev), (UNAME, None)],
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
      ext_modules=[ca_module,])
