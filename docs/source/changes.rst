ChangeLog
=========

3.2.0 (22-11-2022)
------------------

- Fix build on Linux/macOS when only shared epics libraries exist. Although epics base always builds the static libraries
  along with shared libraries on Linux/macOS, some epics base packges (conda/pypi) choose to exclude static libraries
  to reduce package size.
- Change :py:meth:`ca.create_context` optional argument to keyword argument. The new method signature is compatible with
  `caffi <https://pypi.python.org/pypi/caffi>`_. This change is backwards compatible.
- Support Python limited API 3.11. It is not enabled by default.

3.1.4 (20-05-2022)
------------------

- Fix :py:meth:`ca.put` and :py:meth:`ca.get` to accept numpy scalar number as *count* argument via number protocol.
- Remove deprecated function calls of PyEval_ThreadsInitialized and PyEval_InitThreads for Python 3.9+.

3.1.3 (01-10-2020)
------------------

- Fix various places where conversion exceptions are not handled.
- Improve Python 3 compatibility according to PEP 384.

3.1.2 (29-01-2019)
------------------

- Fix epicsPV defaults to wait for connection completion.

3.1.1 (07-12-2018)
------------------

- Fix compilation error on Python 3.7.
- Fix compilation error on epics base > 3.14.
- Change to use buffer object instead of numpy/c api to create numpy array.

3.1.0 (15-10-2018)
------------------

- Added class methods :py:meth:`CaChannel.CaChannel.add_exception_event` and :py:meth:`CaChannel.CaChannel.replace_printf_handler`.
  They are just thin wrapper over the low level functions :py:meth:`ca.add_exception_event` and :py:meth:`ca.replace_printf_handler` respectively.

3.0.4 (15-12-2017)
------------------

- Change to link EPICS dynamic libraries if environment variable ``EPICS_SHARED`` is defined.

3.0.3 (08-12-2017)
------------------

- Fix :py:meth:`ca.put` with non-ascii input string.
- Change that it returns a :py:class:`bytes` object from non-utf8 C string. It fails with an obscure exception message before.
- Change TravisCI to use conda-forge/linux-anvil docker image, but give the defaults channel higher priority.

3.0.2 (23-10-2017)
------------------

- Fix conda build on Linux by pinning conda-build to version 2.

3.0.1 (23-10-2017)
------------------

- Allow *count=0* in :py:meth:`ca.get` if callback is provided.
- Dereference user supplied callbacks
  - get/put callbacks after being called.
  - event callback in :py:meth:`CaChannel.CaChannel.clear_event`.

3.0.0 (06-04-2017)
------------------

- Rewrite low level :py:mod:`ca` module with the same API as in package `caffi <https://pypi.python.org/pypi/caffi>`_.
- Added method :py:meth:`CaChannel.CaChannel.replace_access_rights_event`
- Added method :py:meth:`CaChannel.CaChannel.change_connection_event`
- Added :class:`ca.ECA`, :class:`ca.DBF`, :class:`ca.DBR`, :class:`ca.ChannelState` to represent their C macros :data:`ca.ECA_XXX`,
  :data:`ca.DBF_XXX`, :data:`ca.DBR_XXX`, :data:`ca.cs_xxx`.
  For Python < 3.4,  this requires module `enum34 <https://pypi.python.org/pypi/enum34>`_.
- Changed method :py:meth:`CaChannel.CaChannel.getw` to return string if *req_type* is DBR_STRING for a char waveform.
- Configure continous integration/deployment on Travis/AppVeyor.
- Drop Python 2.4 and 2.5 support.

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
