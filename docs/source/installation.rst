Installation
============

Anaconda
--------
Packages for Anaconda can be installed via::

    conda install -c paulscherrerinstitute cachannel

Binary Installers
-----------------
The binary packages are distributed at `PyPI <https://pypi.python.org/pypi/CaChannel>`_.
They have EPICS 3.14.12.6 libraries statically builtin.
If we do not have a *wheel* or *egg* package for your system, *pip* or *easy_install* will try to
build from source. And then you would need EPICS base installed, see :ref:`getting-epics`.

OS X
~~~~

Make sure you have `pip <https://pypi.python.org/pypi/pip>`_ and 
`wheel <https://pypi.python.org/pypi/wheel>`_  installed, and run::

    $ sudo pip install cachannel

Windows
~~~~~~~

Make sure you have `pip <https://pypi.python.org/pypi/pip>`_ and
`wheel <https://pypi.python.org/pypi/wheel>`_  installed, and run::

    > C:\Python27\Scripts\pip.exe install cachannel

Linux
~~~~~
PyPI does not allow upload linux-specific wheels package, yet (as of 2014).
The old *egg* format is used then::

    $ sudo easy_install cachannel

Or install only for the current user::

    $ easy_install --user cachannel

Source
------
The source can be downloaded in various ways:

  * The released source tarballs can be found at `PyPI <https://pypi.python.org/pypi/CaChannel>`_.

  * From the `git repository <https://github.com/CaChannel/CaChannel>`_, 
    the source can be downloaded as a zip package. 

  * Clone the repository if you feel adventurous::

    $ git clone https://github.com/CaChannel/CaChannel.git

Getting EPICS
~~~~~~~~~~~~~
In general please follow `the official installation instruction <http://www.aps.anl.gov/epics/base/R3-14/12-docs/README.html>`_. Here is a short guide,

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
    |  OS X   +-------+--------------------+
    |         | Intel | darwin-x86         |
    +---------+-------+--------------------+

- Run ``make``.

Build
~~~~~
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
 
Build Anaconda Package
~~~~~~~~~~~~~~~~~~~~~~
If you want to build as Anaconda packages, *conda* directory
contains the recipe.::

    $ conda build -c paulscherrerinstitute conda-recipe

