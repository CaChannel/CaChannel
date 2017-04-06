CaChannel - EPICS Channel Access in Python
==========================================

CaChannel is a Python interface to Channel Access. 
It was developed in 2000 by Geoff Savage using `caPython extension <http://d0server1.fnal.gov/users/savage/www/caPython/caPython.html>`_
In 2008 version 2.x gets re-implemented based on PythonCA extension from `Noboru Yamamoto <http://www-acc.kek.jp/EPICS_Gr/products.html>`_.

In version 3, the ``CaChannel.ca`` module was rewritten from scratch using Python/C API.
It has the same API as `caffi <https://pypi.python.org/pypi/caffi/>`_.
Because of that, the CaChannel interface can also use ``caffi.ca`` by setting environment variable ``CACHANNEL_BACKEND=caffi``.
This is also the fallback backend if no EPICS installation exists or the C extension fails to import.

Check out `CaChannel documents <https://cachannel.readthedocs.org>`_ to get started.

* Downloads: https://pypi.python.org/pypi/CaChannel
* Source Repo: https://github.com/CaChannel/CaChannel
* Issue Tracker: https://github.com/CaChannel/CaChannel/issues
* Anaconda: https://anaconda.org/paulscherrerinstitute/cachannel
