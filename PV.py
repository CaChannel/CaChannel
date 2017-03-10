import threading
import time
import ca

class PV(object):
    """

    """

    def __init__(self, pvname, callback=None, form='time', auto_monitor=None, 
            connection_callback=None, connection_timeout=None, verbose=False):
        """

        >>> def onConnectionChange(pvname=None, conn=None, chid=None):
        ...     print('ca connection status changed:', pvname,  conn, chid)
        >>> pv = PV('catest', connection_callback=onConnectionChange)
        >>> pv.wait_for_connection() # doctest: +ELLIPSIS
        ca connection status changed: catest True <capsule object ...>
        """
        self._pvname = pvname
        self._chid = None

        self._connection_event = threading.Event()
        self._connected = False
        self.connection_callbacks = []

        self.callbacks = {}

        self._put_event = threading.Event()
        self.put_complete = False

        if connection_callback:
            self.connection_callbacks.append(connection_callback)

        status, self._chid = ca.create_channel(pvname, self._connectionCB)

    def __del__(self):
        self.disconnect()

    def _connectionCB(self, epics_args):
        self._connected = epics_args['op'] == ca.CA_OP_CONN_UP

        for callback in self.connection_callbacks:
            callback(pvname=self._pvname, conn=self._connected, chid=self._chid)

        if self._connected:
            self._connection_event.set()
        else:
            self._connection_event.clear()

    def wait_for_connection(self, timeout=None):
        if timeout is None:
            time = 3.0

        self._connection_event.wait(timeout)

    def disconnect(self):
        if self_chid:
            ca.clear_channel(self._chid)

    def _put_callback(self, epics_args, use_complete, callback, callback_data):
        self._put_event.set()
        if use_complete:
            self.put_complete = True
        if callback:
            callback(pvname=self._pvname, **callack_data)

    def put(self, value, wait=False, timeout=30.0, use_complete=False, 
            callback=None, callback_data=None):
        """
        >>> pv = PV('catest')
        >>> pv.wait_for_connection()
        >>> def onPutComplete(pvname=None, **kws):
        ...     print('put done for %' % pvname)
        >>> pv.put(1.0, callback=onPutComplete)
        >>> time.sleep(1)
        put done for catest
        """
        self._put_event.clear()
        self.put_complete = False

        status = ca.put(value, lambda args: self._put_callback(args, use_complete, callback, callack_data))
        if wait:
            self._put_event.wait(timeout)


if __name__ == '__main__':
    import doctest
    doctest.testmod()

