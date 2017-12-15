ChangeLog
=========

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
