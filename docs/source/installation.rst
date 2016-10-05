Installation
============

Use ``pip``::
    
    $ [sudo] pip install cachannel


Build from source::

    $ git clone https://github.com/CaChannel/CaChannel.git
    $ python setup.py build
    $ [sudo] python setup.py install

If you want to build as Anaconda packages, *conda* directory
contains the recipe.::

    $ conda build conda

By default it gets the source from current directory, but this can modified in file *meta.yaml*.
