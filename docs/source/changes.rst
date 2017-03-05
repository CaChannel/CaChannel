ChangeLog
=========

3.0.0 (XX-03-2017)
------------------

- Rewrite low level :py:module:`ca` module with the same API as in package `caffi <https://pypi.python.org/pypi/caffi>`_.
- Configure continous integration/deployment on Travis/AppVeyor.

2.4.2
-----

- Fix chid crash on 64bit windows
- Add epics libs for python 3.5 on windows

2.4.1
-----

- All modules are compatible with Python 2.4+ including Python 3.
- conda build recipe bundle caRepeater program in the package

2.4.0
-----

- Add often used 3rd party module, ca_util, epicsPV and epicsMotor
- Add Anaconda build recipe
- Remove dependency of readline from Com library

2.3.0
-----

- Support Python 3
