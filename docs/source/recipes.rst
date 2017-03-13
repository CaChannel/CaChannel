Recipes
=======

.. py:currentmodule:: CaChannel.CaChannel

Connect Multiple Channels
-------------------------
If there are multiple channels to connect, using :meth:`searchw` might not be efficient, because it connects each
channel sequentially. A better approach is to create the channels and flush the search request at once.

::

    from CaChannel import ca, CaChannel
    chans = {pvname: CaChannel(pvname) for pvname in ['pv1', 'pv2', 'pv3', ...]}
    for chan in chans.values():
        chan.search()
    # call pend_io on either of the channels and the it will flush the requests and wait for completion
    # if connection does not complete in 10 seconds, CaChannelException is raised with status ca.ECA_TIMEOUT
    chans['pv1'].pend_io(10)

Get String of Enum
------------------

::

    from CaChannel import ca, CaChannel
    chan = CaChannel('myEnumPV')
    chan.searhw()
    print(chan.getw(ca.DBR_STRING))

Get Control Information 
-----------------------

::

    from CaChannel import ca, CaChannel
    chan = CaChannel('myPV')
    chan.searhw()
    print(chan.getw(ca.dbf_type_to_DBR_CTRL(chan.field_type())))

Get Wavefrom as Numpy Array
---------------------------
- At function level 

  ::
    
    from CaChannel import ca, CaChannel
    chan = CaChanne('myWaveformPV')
    print(chan.getw(use_numpy=True))

- At module level

  ::

    import CaChannel
    CaChannel.USE_NUMPY = True
    chan = CaChannel.CaChanne('myWaveformPV')
    print(chan.getw())

