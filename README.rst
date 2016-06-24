CaChannel - EPICS Channel Access in Python
==========================================

CaChannel is a Python interface to Channel Access. 
It was originally developed by Geoff Savage using `caPython extension <http://d0server1.fnal.gov/users/savage/www/caPython/caPython.html>`_

Before version 3.0, this CaChannel implementation uses module ``ca`` PythonCA extension by `Noboru Yamamoto <http://www-acc.kek.jp/EPICS_Gr/products.html>`_.
From version 3.0, the underlying ``_ca`` module has been rewritten from scratch using Python/C API. In addition,
it is also possible to use module ``caffi.ca`` as backend, by setting environment variable ``CACHANNEL_BACKEND=caffi``.
This is also the fallback backend when ``_ca`` extension is not available.

Installation
------------
EPICS base 3.14.12.4 headers and static libraries are packed under ``epicsbase`` 
for OS X (Intel 32/64 bit), Linux (Intel 32/64 bit) and Windows (Intel 32/64 bit).

Use pip::

    $ [sudo] pip install cachannel

Or build from source, by which you need to have an appropriate compiler for your platform.
::

    $ git clone https://github.com/xiaoqiangwang/cachannel
    $ python setup.py build
    $ [sudo] python setup.py install

Or build for Anaconda,
::
    
    $ hg clone http://github.com/xiaoqiangwang/cachannel
    $ cd conda
    $ conda build .

Documentation
-------------
Hosted at `Read the Docs <http://cachannel.readthedocs.io>`_
