Recipes
=======

.. py:currentmodule:: CaChannel.CaChannel

Connect Multiple Channels And Get Values
----------------------------------------
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
    # if the previous pend_io succeed without exception, we can issue the read request
    for chan in chans.values():
        chan.array_get()
    # again call pend_io to wait the read requests to succeed or timeout
    chans['pv1'].pend_io(10)
    # if the previous pend_io succeed without exception, the values can be retrieved with getValue
    for chan in chans.values():
        print(chan.getValue()

Connect Multiple Channels And Monitor Changes
---------------------------------------------
Similiar to the above recipe, but instead of reading the values once, here is to monitor the value changes.

::

    from CaChannel import ca, CaChannel

    # value change callback
    def monitor_callback(epics_arg, user_arg):
        chan = user_arg[0]
        value = epics_arg['pv_value']
        print(chan.name(), value)

    # in the connection callback we will subscribe for value changes
    def connection_callback(epics_arg, user_arg):
        chan = user_arg[0]
        if epics_arg[1] == ca.CA_OP_CONN_UP:
            chan.add_masked_array_event(None, None, None, monitor_callback, chan)
            chan.flush_io()

    # create channels and connect asynchronously
    chans = {pvname: CaChannel(pvname) for pvname in ['pv1', 'pv2', 'pv3', ...]}
    for chan in chans.values():
        chan.search_and_connect(None, connection_callback, chan)
    # flush the channel connection requests
    chans['pv1'].flush_io()

    # because the callbacks happen in auxiliary threads, the main thread
    # is free to do other important stuff, like sleep 10 seconds ;)
    time.sleep(10)

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

