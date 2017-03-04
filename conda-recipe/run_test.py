#!/usr/bin/env python

import os
import platform
import socket
import sys
import time
import re
import runpy
import subprocess
import doctest
import unittest
import pcaspy
import pcaspy.tools


pvdb = {
    'castr': {'type':'str'},
    'catest': {'lolo':-20,'low':-10,'high':10,'hihi':20,'prec':4,'unit':'mm'},
    'cabo': {
        'type':'enum',
        'enums': ['Done','Busy'],
        'states': [pcaspy.Severity.NO_ALARM, pcaspy.Severity.MINOR_ALARM]
        },
    'cawave': {'count':20, 'prec':4},
    'cawavec': {'type':'char', 'count':5},
    'cawaves': {'type':'str', 'count':3},
}

class myDriver(pcaspy.Driver):
    def __init__(self):
        pcaspy.Driver.__init__(self)


class TestCa(unittest.TestCase):
    def setUp(self):
        if platform.system() == 'Windows':
            self.start_pcas()
        else:
            self.start_ioc()

    def start_pcas(self):
        server = pcaspy.SimpleServer()
        server.createPV('', pvdb)
        driver = myDriver()
        self.server_thread = pcaspy.tools.ServerThread(server)
        self.server_thread.start()

    def stop_pcas(self):
        self.server_thread.stop()

    def start_ioc(self):
        try:
            UNAME=platform.uname()[0]
            ARCH=platform.architecture()[0]
        except:
            UNAME="Unknown"
            ARCH="Unknown"

        pflags = 0
        if UNAME.lower() == "windows":
            if ARCH=="64bit":
                HOSTARCH="windows-x64"
            else:
                HOSTARCH="win32-x86"
            pflags |= 0x00000008 #DETACHED_PROCESS
        elif UNAME.lower() == "darwin":
            HOSTARCH = 'darwin-x86'
        elif UNAME.lower() == "linux":
            if ARCH=="64bit":
                HOSTARCH="linux-x86_64"
            else:
                HOSTARCH="linux-x86"
        else:
            raise RuntimeError("Platform % is not supported"%UNAME)

        EPICS_BIN = os.path.join(os.environ['PREFIX'], 'epics', 'bin', HOSTARCH)
        EPICS_DBD = os.path.join(os.environ['PREFIX'], 'epics', 'dbd', 'softIoc.dbd')

        environ = os.environ.copy()
        environ['PATH'] += os.pathsep + EPICS_BIN
        self.softIoc = subprocess.Popen([os.path.join(EPICS_BIN, 'softIoc'),
                    '-D', EPICS_DBD,
                    '-S',
                    '-d', 'tests/test.db'],
                env = environ,
                creationflags = pflags,
        )
        time.sleep(2)

    def stop_ioc(self):
        self.softIoc.kill()
        self.softIoc.wait()

    def test_ca(self):
        try:
            if platform.system() != 'Linux':
                runpy.run_path('tests/ca_test.py', run_name='__main__')
        except SystemExit as e:
            ecode = e.code
        else:
            ecode = 0
        self.assertEqual(ecode, 0)

    def test_CaChannel(self):
        test = doctest.DocTestSuite('CaChannel.CaChannel')
        runner = unittest.TextTestRunner()
        runner.run(test)

    def tearDown(self):
        time.sleep(2)
        if platform.system() == 'Windows':
            self.stop_pcas()
        else:
            self.stop_ioc()
        time.sleep(2)

if __name__ == '__main__':
    unittest.main()
