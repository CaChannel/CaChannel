#!/usr/bin/env python
"""
Setup file for Ca-Python using distutils package.
Python2.4 or later should be used.
"""

import os
import platform
from distutils.core import setup, Extension

EPICSBASE=os.path.join(os.getcwd(), 'epicsbase')

HOSTARCH=os.popen('perl ' + os.path.join(EPICSBASE,"startup","EpicsHostArch.pl")).read()

try:
    UNAME=platform.uname()[0]
except:
    UNAME="Unknowon"

if UNAME.lower() == "alpha":
    libraries=["ca","As","Com"]
elif UNAME.lower() == "win32":
    libraries=["ca","Com","ws2_32","msvcrt","user32", "advapi32"]
elif UNAME.lower() == "darwin":
    libraries=["ca","Com","readline"]
else:
    libraries=["ca","Com","readline","rt"]


rev="2.0.1"
CA_SOURCE="src/_ca314.cpp" # for threaded version.
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
      ext_modules=[Extension("_ca",[CA_SOURCE],
                             include_dirs=[os.path.join(EPICSBASE,"include"),
                                           os.path.join(EPICSBASE,"include", "os",UNAME),
                                           os.path.join(EPICSBASE,"include", "os"),],
                             define_macros=[("PYCA_VERSION",'"\\"%s\\""'%rev),
                                            (UNAME, None)],
                             undef_macros="WITH_TK",
                             libraries=libraries,
                             library_dirs=[os.path.join(EPICSBASE,"lib",HOSTARCH),],
                             runtime_library_dirs=[os.path.join(EPICSBASE,"lib",HOSTARCH),],
                            )
                  ],
    )
