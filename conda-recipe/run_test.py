#!/usr/bin/env python

import os
import platform
import sys
import time
import runpy
import subprocess
import doctest
import unittest


class TestCa(unittest.TestCase):
    def setUp(self):
        self.start_ioc()

    def start_ioc(self):
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
            raise RuntimeError("Platform % is not supported"%UNAME)

        EPICS_BIN = os.path.join(os.environ['PREFIX'], 'epics', 'bin', HOSTARCH)
        EPICS_DBD = os.path.join(os.environ['PREFIX'], 'epics', 'dbd', 'softIoc.dbd')
        os.environ.update({
            'EPICS_CA_AUTO_ADDR_LIST': 'NO',
            'EPICS_CA_ADDR_LIST': 'localhost',
            })

        self.caRepeater = subprocess.Popen([os.path.join(EPICS_BIN, 'caRepeater')])
        self.softIoc = subprocess.Popen([os.path.join(EPICS_BIN, 'softIoc'),
                    '-D', EPICS_DBD,
                    '-S',
                    '-d', 'tests/test.db'],
        )
        time.sleep(2)
        # check for early crashes
        if self.softIoc.poll() is not None:
            self.softIoc = None

    def stop_ioc(self):
        time.sleep(2)
        if self.softIoc:
            self.softIoc.kill()
            self.softIoc.wait()
        time.sleep(1)
        self.caRepeater.kill()
        self.caRepeater.wait()

    def test_ca(self):
        if self.softIoc is None:
            print("softIoc has not been started, skip the test")
            return

        try:
            runpy.run_path('tests/ca_test.py', run_name='__main__')
        except SystemExit as e:
            ecode = e.code
        else:
            ecode = 0
        self.assertEqual(ecode, 0)

    def test_CaChannel(self):
        if self.softIoc is None:
            print("softIoc has not been started, skip the test")
            return

        # increase default timeout to 10 seconds
        import CaChannel.CaChannel
        CaChannel.CaChannel.ca_timeout = 10.
        test = doctest.DocTestSuite('CaChannel.CaChannel')
        results = unittest.TextTestRunner().run(test)
        self.assertEqual(len(results.failures), 0)

    def tearDown(self):
        self.stop_ioc()

if __name__ == '__main__':
    unittest.main()
