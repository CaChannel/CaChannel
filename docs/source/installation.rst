Installation
============

Binary Installers
-----------------

Anaconda
^^^^^^^^
Packages for Anaconda can be installed via::

    conda install -c paulscherrerinstitute cachannel

Wheel
^^^^^
The binary packages are distributed at `PyPI <https://pypi.python.org/pypi/CaChannel>`_.
They have EPICS 3.14.12.6 libraries statically builtin. Make sure you have `pip <https://pypi.python.org/pypi/pip>`_
and `wheel <https://pypi.python.org/pypi/wheel>`_  installed, ::

    $ sudo pip install cachannel # macOS
    > C:\Python27\Scripts\pip.exe install cachannel :: Windows

Egg
^^^
PyPI does not allow upload linux-specific wheels package, yet (as of 2014).
The old *egg* format is used then::

    $ sudo easy_install cachannel

Or install only for the current user::

    $ easy_install --user cachannel

Source
------
If a binary package is not available for your system, you can build from source.
If you have a valid EPICS base installation, as described by :ref:`getting-epics`,
the C extension will be compiled. Otherwise it will instead use `caffi <https://pypi.python.org/pypi/caffi>`_
as the backend.

The source can be downloaded in various ways:

  * The released source tarballs can be found at `PyPI <https://pypi.python.org/pypi/CaChannel>`_.
  * From the `git repository <https://github.com/CaChannel/CaChannel/releases>`_,
    each release can be downloaded as a zip package.
  * Clone the repository if you feel adventurous::

    $ git clone https://github.com/CaChannel/CaChannel.git

On Linux, the python header files are normally provided by package like *python-devel* or *python-dev*.

``numpy`` is optional, but it can boost the performace when reading large waveform PVs,
which are common for areaDetector images.


.. _getting-epics:

Getting EPICS
^^^^^^^^^^^^^
In general please follow `the official installation instruction <http://www.aps.anl.gov/epics/base/R3-14/12-docs/README.html>`_.
Here is a short guide,

- Get the source tarball from http://www.aps.anl.gov/epics/base/R3-14/12.php.
- Unpack it to a proper path.
- Set the following environment variables:

  - EPICS_BASE : the path containing the EPICS base source tree.
  - EPICS_HOST_ARCH : EPICS is built into static libraries on Windows.

    +---------+-------+--------------------+
    |    OS   | Arch  | EPICS_HOST_ARCH    |
    +=========+=======+====================+
    |         | 32bit | linux-x86          |
    | Linux   +-------+--------------------+
    |         | 64bit | linux-x86_64       |
    +---------+-------+--------------------+
    |         | 32bit | win32-x86          |
    | Windows +-------+--------------------+
    |         | 64bit | windows-x64        |
    +---------+-------+--------------------+
    |         | PPC   | darwin-ppcx86      |
    |         +-------+--------------------+
    | macOS   | Intel | darwin-x86         |
    |         +-------+--------------------+
    |         | Arm   | darwin-aarch64     |
    +---------+-------+--------------------+

- Run ``make``.

Build
^^^^^
As soon as the epics base libraries are ready, it is simple,
    
- On Widnows::

    > C:\Python27\python.exe setup.py install

- On Linux/macOS::

    $ [sudo] python setup.py install


.. note:: You might need to pass *-E* flag to sudo to preserve the EPICS environment variables. If your user account
          is not allowed to do so, a normal procedure should be followed, ::

              $ su -
              # export EPICS_BASE=<epics base path>
              # export EPICS_HOST_ARCH=<epics host arch>
              # python setup.py install

Package
-------
After the build succeeds, you may want to create a package for distribution.

Anaconda
^^^^^^^^
Conda recipe is included::

    $ conda build -c paulscherrerinstitute conda-recipe

Wheel
^^^^^
::

    $ python setup.py bdist_wheel

RPM
^^^
The spec file *python-CaChannel.spec* is included. Get the source tarball either from PyPI or create it by
``python setup.py sdist``, and run::

    $ rpmbuild -ta CaChannel-3.0.0.tar.gz

The binary and source RPM will be created. The package name is *python-CaChannel*.
