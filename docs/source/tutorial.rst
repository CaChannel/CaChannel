Short Tutorials
===============

Synchronous Actions
-------------------

Each of the actions (search, put, get) ending with a “w” signify that the action completes before the function returns. 
In CA terms this means that a call to ca_pend_io() is issued to force the action to process and wait for the action to complete.
When an exception occurs the offending CA status return is printed using print ca.message(status).

::

    from CaChannel import CaChannel, CaChannelException
    try:
        chan = CaChannel('catest')
        chan.searchw()
        chan.putw(12)
        chan.getw()
    except CaChannelException as e:
        print(e)


Multiple Synchronous Actions
----------------------------

Connection
^^^^^^^^^^
Multiple channel access connection requests.

::

    from CaChannel import ca, CaChannel, CaChannelException
    try:
        chan1 = CaChannel('catest')
        chan1.search()
        chan2 = CaChannel('cabo')
        chan2.search()
        chan2.pend_io()
    except CaChannelException as e:
        print(e)

Write
^^^^^
Multiple channel access write requests.

::

    from CaChannel import ca, CaChannel, CaChannelException
    try:
        chan1 = CaChannel('catest')
        chan1.search()
        chan2 = CaChannel('cabo')
        chan2.search()
        chan2.pend_io()
        chan1.array_put(1.23)
        chan2.array_put(1)
        chan2.flush_io()
    except CaChannelException as e:
        print(e)

Asynchronouse Actions
---------------------
Asynchronous execution does not require that the user wait for completion of an action. 
Instead, a user specified callback function is executed when the action has completed.
Each callback takes two arguments:

- epics_args: arguments returned from epics.
- user_args: arguments specified by the user for use in the callback function.

Since we don’t need to wait for actions to complete we use flush_io() instead of pend_io() as in the synchronous examples. 
Flush_io() starts execution of actions and returns immediately. 

Note: The callback function is not executed in the main thread. It runs in an auxiliary thread managed by CA library.

::

    import time
    from CaChannel import ca, CaChannel, CaChannelException
    def connectCB(epics_args, user_args):
        print("connectCB: Python connect callback function")
        print(type(epics_args))
        print(epics_args)
        print(user_args)
        state = epics_args[1]
        if state == ca.CA_OP_CONN_UP:
            print("connectCB: Connection is up")
        elif state == ca.CA_OP_CONN_DOWN:
            print("connectCB: Connection is down")

    def putCB(epics_args, user_args):
        print("putCB: Python put callback function")
        print(type(epics_args))
        print(epics_args)
        print(ca.name(epics_args['chid']))
        print(epics_args['type'])
        print(epics_args['count'])
        print(epics_args['status'])
        print(user_args)

    chan = CaChannel()
    chan.search_and_connect('catest', connectCB)
    chan.flush_io()
    time.sleep(1)
    chan.array_put_callback(3.3, None, None, putCB)
    chan.flush_io()
    time.sleep(1)


Asynchronous Monitoring
-----------------------

Watch for changes in value or alarm state of a process variable. A callback is executed when a change is seen.

::

    import sys
    import time
    from CaChannel import ca, CaChannel, CaChannelException
    def eventCB(epics_args, user_args):
        print("eventCb: Python callback function"
        print(type(epics_args))
        print(epics_args)
        print(epics_args['status'])
        print("new value =", epics_args['pv_value'])
        print(epics_args['pv_severity'])
        print(epics_args['pv_status'])

    chan = CaChannel()
    chan.searchw('catest')
    chan.add_masked_array_event(
        ca.DBR_STS_DOUBLE,
        None,
        None,
        eventCB)
    chan.flush_io()
    time.sleep(5)

