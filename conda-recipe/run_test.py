#!/usr/bin/env python

import os
import platform
import socket
import sys
import re
import runpy
import subprocess
import doctest
import unittest

print('Launch softIoc')
try:
    UNAME=platform.uname()[0]
    ARCH=platform.architecture()[0]
except:
    UNAME="Unknown"
    ARCH="Unknown"

if UNAME.lower() == "windows":
    if ARCH=="64bit":
        HOSTARCH="windows-x64"
    else:
        HOSTARCH="win32-x86"
elif UNAME.lower() == "darwin":
    HOSTARCH = 'darwin-x86'
elif UNAME.lower() == "linux":
    if ARCH=="64bit":
        HOSTARCH="linux-x86_64"
    else:
        HOSTARCH="linux-x86"
else:
    print("Platform", UNAME, ARCH, " Not Supported")
    sys.exit(1)

ip = socket.gethostbyname(socket.gethostname())

for k,v in os.environ.items():
   print(k, v)

softIoc = subprocess.Popen(['softIoc', '-d', 'tests/test.db'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, 
        env = {'PATH': os.path.join(os.environ['PREFIX'], 'epics', 'bin', HOSTARCH),
            'EPICS_CA_AUTO_ADDR_LIST': 'NO',
            'EPICS_CAS_BEACON_ADDR_LIST': re.sub('.\d+$', '.255', ip)
            }
        )

print('Run ca_test.py')
try:
    if UNAME.lower() != 'linux':
        runpy.run_path('tests/ca_test.py', run_name='__main__')
except SystemExit as e:
    ecode = e.code
else:
    ecode = 0

if ecode != 0:
    softIoc.terminate()
    sys.exit(ecode)

print('Run CaChannel')
try:
    test = doctest.DocTestSuite('CaChannel.CaChannel')
    runner = unittest.TextTestRunner()
    runner.run(test)
except SystemExit as e:
    ecode = e.code
else:
    ecode = 0

if ecode != 0:
    softIoc.terminate()
    sys.exit(ecode)

softIoc.terminate()
sys.exit(0)

