CaChannel - EPICS Channel Access in Python
==========================================

CaChannel is a Python interface to Channel Access. 
It was developed in 2000 by Geoff Savage using `caPython extension <http://d0server1.fnal.gov/users/savage/www/caPython/caPython.html>`_
Later on the CA library becomes multi-threaded with EPICS base 3.14. The original developer did not address this change however.

In 2008 during the transition to EPICS 3.14, a new implementation of CaChannel interface, version 2.x,  was developed based on 
PythonCA extension from `Noboru Yamamoto <http://www-acc.kek.jp/EPICS_Gr/products.html>`_. It was highly backwards compatible with
the original implementation based on caPython.

In 2014, package `caffi <https://github.com/CaChannel/caffi>`_ was created in place of caPython or PythonCA extension to expose 
the `Channel Access API <http://www.aps.anl.gov/epics/base/R3-14/12-docs/CAref.html>`_ using `cffi <https://pypi.python.org/pypi/cffi>`_.
It aimed to create a Pythonic API of one-to-one map to the counterpart C functions.
After that has been accomplished, CaChannel interface was re-implemented using the ``caffi.ca`` module, versioned 3.x.

In 2015-16, with all previous experiences, the ``CaChannel.ca`` module was rewritten from scratch using Python/C API.
The new ``CaChannel.ca`` module has a compatible API with ``caffi.ca``. Because of that, the CaChannel interface can use
``caffi.ca`` by setting environment variable ``CACHANNEL_BACKEND=caffi``. This is also the fallback backend if no EPICS installation
exists or the C extension fails to import.

Check out `CaChannel documents <https://cachannel.readthedocs.org>`_ to get started.

* Downloads: https://pypi.python.org/pypi/CaChannel
* Source Repo: https://github.com/CaChannel/CaChannel
* Issue Tracker: https://github.com/CaChannel/CaChannel/issues
* Anaconda: https://anaconda.org/paulscherrerinstitute/cachannel

