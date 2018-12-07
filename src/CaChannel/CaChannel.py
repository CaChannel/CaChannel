"""
CaChannel class having identical API as of caPython/CaChannel class,
based on `caffi.ca API <https://caffi.readthedocs.io/en/latest/api.html>`.
"""
# python 2 -> 3 compatible layer
from __future__ import print_function, absolute_import
from functools import wraps
import itertools
import numbers
import sys
import traceback
if sys.hexversion < 0x03000000:
    from collections import Sequence
else:
    from collections.abc import Sequence

import CaChannel as PACKAGE
from . import ca


class CaChannelException(Exception):
    def __init__(self, status):
        self.status = status

    def __int__(self):
        return self.status

    def __str__(self):
        return ca.message(self.status)


class CaChannel:
    """CaChannel: A Python class with identical API as of caPython/CaChannel.

    This class implements the methods to operate on channel access so that you can find
    their  C library counterparts ,
    http://www.aps.anl.gov/epics/base/R3-14/12-docs/CAref.html#Function.
    Therefore an understanding of C API helps much.

    To get started easily, convenient methods are created for often used operations,

    ==========    ======
    Operation     Method
    ==========    ======
    connect       :meth:`searchw`
    read          :meth:`getw`
    write         :meth:`putw`
    ==========    ======

    They have shorter names and default arguments. It is recommended to start with these methods.
    Study the other C alike methods when necessary.

    >>> chan = CaChannel('catest')
    >>> chan.searchw()
    >>> chan.putw(12.5)
    >>> chan.getw()
    12.5
    >>> chan.searchw('cabo')
    >>> chan.putw('Done')
    >>> chan.getw(ca.DBR_STRING)
    'Done'
    """

    __context = None
    __callbacks = {}

    ca_timeout = 3.0

    # A wrapper to automatically attach to default CA context
    def attach_ca_context(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            if CaChannel.__context is None:
                ca.create_context(True)
                CaChannel.__context = ca.current_context()
            else:
                ca.attach_context(CaChannel.__context)
            return func(*args, **kwargs)

        return wrapper

    def __init__(self, pvName=None):
        self._pvname = pvName
        self._chid = None
        self._evid = None
        self._timeout = None
        self._dbrvalue = None
        self._callbacks = {}

    def __del__(self):
        try:
            self.clear_channel()
            self.flush_io()
        except:
            pass

    @staticmethod
    def version():
        return PACKAGE.__version__

    #
    # Class helper methods
    #
    def setTimeout(self, timeout):
        """Set the timeout for this channel object. It overrides the class timeout.

        :param float timeout:  timeout in seconds

        >>> chan = CaChannel()
        >>> chan.setTimeout(10.)
        >>> chan.getTimeout()
        10.0
        """
        if timeout >= 0 or timeout is None:
            self._timeout = timeout
        else:
            raise ValueError

    def getTimeout(self):
        """Retrieve the timeout set for this channel object.

        :return: timeout in seconds for this channel instance
        :rtype: float

        >>> chan = CaChannel()
        >>> chan.getTimeout() == CaChannel.ca_timeout
        True
        """
        if self._timeout is None:
            timeout = CaChannel.ca_timeout
        else:
            timeout = self._timeout

        return timeout

    @classmethod
    @attach_ca_context
    def replace_printf_handler(cls, callback=None, user_args=None):
        """
        Install or replace the callback used for formatted CA diagnostic message output.
        The default is to send to stderr.

        :param callable callback: function called.
        :param user_args: user provided arguments that are passed to callback when it is invoked.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> def printfCB(message, _):
        ...     print('CA message:', message)
        >>> chan.replace_printf_handler(printfCB)  # add callback # doctest: +SKIP
        >>> chan.replace_printf_handler() # clear the callback # doctest: +SKIP

        .. versionadded:: 3.1
        """
        if callable(callback):
            cls.__callbacks['printfCB'] = (callback, user_args)
            ca.replace_printf_handler(cls._printf_callback)
        else:
            cls.__callbacks['printfCB'] = None
            ca.replace_printf_handler(None)

    @classmethod
    @attach_ca_context
    def add_exception_event(cls, callback=None, user_args=None):
        """
        Install or replace the currently installed CA context global exception handler callback.

        When an error occurs in the server asynchronous to the clients thread then information
        about this type of error is passed from the server to the client in an exception message.
        When the client receives this exception message an exception handler callback is called.
        The default exception handler prints a diagnostic message on the client's standard out and
        terminates execution if the error condition is severe.

        Note that certain fields returned in the callback args are not applicable in the context of
        some error messages. For instance, a failed get will supply the address in the client task
        where the returned value was requested to be written. For other failed operations the value
        of the addr field should not be used.

        :param callable callback: function called.
        :param user_args: user provided arguments that are passed to callback when it is invoked.

        The possible fields available are as defined in the C "struct exception_handler_args"
        and are: chid, type, count, state, op, ctx, file, lineNo

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> def exceptionCB(epicsArgs, _):
        ...     print('op:', epicsArgs['op'], 'file:', epicsArgs['file'], 'line:', epicsArgs['lineNo'])
        >>> chan.add_exception_event(exceptionCB) # add callback # doctest: +SKIP
        >>> chan.add_exception_event() # clear the callback # doctest: +SKIP

        .. versionadded:: 3.1
        """
        if callable(callback):
            cls.__callbacks['exceptionCB'] = (callback, user_args)
            ca.add_exception_event(cls._exception_callback)
        else:
            cls.__callbacks['exceptionCB'] = None
            ca.add_exception_event(None)

    def replace_access_rights_event(self, callback=None, user_args=None):
        """
        Install or replace the access rights state change callback handler for the specified channel.

        The callback handler is called in the following situations.

            - whenever CA connects the channel immediately before the channel's connection handler is called
            - whenever CA disconnects the channel immediately after the channel's disconnect callback is called
            - once immediately after installation if the channel is connected
            - whenever the access rights state of a connected channel changes

        When a channel is created no access rights handler is installed.

        :param callable callback: function called when access rights change. If None is given,
                                  remove the access rights event callback.
        :param user_args: user provided arguments that are passed to callback when it is invoked.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> def accessCB(epicsArgs, _):
        ...     print('read:', epicsArgs['read_access'], 'write:', epicsArgs['write_access'])
        >>> chan.replace_access_rights_event(accessCB)
        read: True write: True
        >>> chan.replace_access_rights_event() # clear the callback

        .. versionadded:: 3.0
        """
        if callable(callback):
            self._callbacks['accessCB'] = (callback, user_args)
            ca.replace_access_rights_event(self._chid, self._access_callback)
        else:
            self._callbacks['accessCB'] = None
            ca.replace_access_rights_event(self._chid)

    #
    # *************** Channel access method ***************
    #
    #
    # Connection methods
    #   search_and_connect
    #   search
    #   clear_channel
    #   change_connection_event

    @attach_ca_context
    def search_and_connect(self, pvName, callback, *user_args):
        """Attempt to establish a connection to a process variable.

        :param pvName:     process variable name
        :param callback:   function called when connection completes and connection status changes later on.
        :param user_args:  user provided arguments that are passed to callback when it is invoked.
        :type pvName:   bytes, str
        :type callback: callable
        :raises CaChannelException: if error happens

        The user arguments are returned to the user in a tuple in the callback function.
        The order of the arguments is preserved.

        Each Python callback function is required to have two arguments.
        The first argument is a tuple containing the results of the action.
        The second argument is a tuple containing any user arguments specified by *user_args*.
        If no arguments were specified then the tuple is empty.


        .. note:: All remote operation requests such as the above are accumulated (buffered) and not forwarded to
           the IOC until one of execution methods (:meth:`pend_io`, :meth:`poll`, :meth:`pend_event`, :meth:`flush_io`)
           is called. This allows several requests to be efficiently sent over the network in one message.

        >>> chan = CaChannel('catest')
        >>> def connCB(epicsArgs, _):
        ...     chid = epicsArgs[0]
        ...     connection_state = epicsArgs[1]
        ...     if connection_state == ca.CA_OP_CONN_UP:
        ...         print(ca.name(chid), "is connected")
        >>> chan.search_and_connect(None, connCB, chan)
        >>> status = chan.pend_event(3) # doctest: +SKIP
        catest is connected
        >>> chan.search_and_connect('cabo', connCB, chan)
        >>> status = chan.pend_event(3) # doctest: +SKIP
        cabo is connected
        >>> chan.clear_channel()
        """
        if self._chid is not None:
            self.clear_channel()

        if pvName is None:
            pvName = self._pvname
        else:
            self._pvname = pvName

        self._callbacks['connCB'] = (callback, user_args)

        status, self._chid = ca.create_channel(pvName, self._conn_callback)
        if status != ca.ECA_NORMAL:
            raise CaChannelException(status)

    @attach_ca_context
    def search(self, pvName=None):
        """Attempt to establish a connection to a process variable.

        :param pvName: process variable name
        :type pvName: bytes, str
        :raises CaChannelException: if error happens

        .. note:: All remote operation requests such as the above are accumulated (buffered) and not forwarded to
           the IOC until one of execution methods (:meth:`pend_io`, :meth:`poll`, :meth:`pend_event`, :meth:`flush_io`)
           is called. This allows several requests to be efficiently sent over the network in one message.

        >>> chan = CaChannel()
        >>> chan.search('catest')
        >>> status = chan.pend_io()
        >>> chan.state()
        <ChannelState.CONN: 2>
        """
        if self._chid is not None:
            self.clear_channel()

        if pvName is None:
            pvName = self._pvname
        else:
            self._pvname = pvName

        status, self._chid = ca.create_channel(pvName)
        if status != ca.ECA_NORMAL:
            raise CaChannelException(status)

    def clear_channel(self):
        """Close a channel created by one of the search functions.

        Clearing a channel does not cause its connection handler to be called.
        Clearing a channel does remove any monitors registered for that channel.
        If the channel is currently connected then resources are freed only some
        time after this request is flushed out to the server.

        .. note:: All remote operation requests such as the above are accumulated (buffered) and not forwarded to
           the IOC until one of execution methods (:meth:`pend_io`, :meth:`poll`, :meth:`pend_event`, :meth:`flush_io`)
           is called. This allows several requests to be efficiently sent over the network in one message.

        """
        if self._chid is not None:
            chid = self._chid
            self._chid = None
            self._dbrvalue = None
            self._callbacks = {}

            self.clear_event()
            ca.clear_channel(chid)

    def change_connection_event(self, callback, *user_args):
        """Change the connection callback function

        :param callback:   function called when connection completes and connection status changes later on.
                           The previous connection callback will be replaced. If an invalid callback is given,
                           no connection callback will be used.
        :param user_args:  user provided arguments that are passed to callback when it is invoked.
        :type callback:    callable

        >>> chan = CaChannel('catest')
        >>> chan.search() # connect without callback
        >>> def connCB(epicsArgs, _):
        ...     chid = epicsArgs[0]
        ...     connection_state = epicsArgs[1]
        ...     if connection_state == ca.CA_OP_CONN_UP:
        ...         print(ca.name(chid), "is connected")
        >>> chan.change_connection_event(connCB) # install connection callback
        >>> status = chan.pend_event(3) # doctest: +SKIP
        catest is connected
        >>> chan.change_connection_event(None) # remove connection callback

        """

        if callable(callback):
            self._callbacks['connCB'] = (callback, user_args)
            status = ca.change_connection_event(self._chid, self._conn_callback)
        else:
            self._callbacks['connCB'] = None
            status = ca.change_connection_event(self._chid)

        if status != ca.ECA_NORMAL:
            raise CaChannelException(status)

    #
    # Write methods
    #   array_put
    #   array_put_callback
    #

    def array_put(self, value, req_type=None, count=None):
        """Write a value or array of values to a channel

        :param value:    data to be written. For multiple values use a list or tuple
        :param req_type: database request type (``ca.DBR_XXXX``). Defaults to be the native data type.
        :param count:    number of data values to write. Defaults to be the native count.
        :type value: int, float, bytes, str, list, tuple, array
        :type req_type: int, None
        :type count: int, None

        .. note:: All remote operation requests such as the above are accumulated (buffered) and not forwarded to
           the IOC until one of execution methods (:meth:`pend_io`, :meth:`poll`, :meth:`pend_event`, :meth:`flush_io`)
           is called. This allows several requests to be efficiently sent over the network in one message.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.array_put(123)
        >>> chan.flush_io()
        >>> chan.getw()
        123.0
        >>> chan = CaChannel('cabo')
        >>> chan.searchw()
        >>> chan.array_put('Busy', ca.DBR_STRING)
        >>> chan.flush_io()
        >>> chan.getw()
        1
        >>> chan = CaChannel('cawavec')
        >>> chan.searchw()
        >>> chan.array_put([1,2,3])
        >>> chan.flush_io()
        >>> chan.getw()
        [1, 2, 3, 0, 0]
        >>> chan.getw(count=3, use_numpy=True)
        array([1, 2, 3], dtype=uint8)
        >>> chan = CaChannel('cawavec')
        >>> chan.searchw()
        >>> chan.array_put('1234',count=3)
        >>> chan.flush_io()
        >>> chan.getw(count=4)
        [49, 50, 51, 0]
        """
        status = ca.put(self._chid, value, req_type, count)
        if status != ca.ECA_NORMAL:
            raise CaChannelException(status)

    def array_put_callback(self, value, req_type, count, callback, *user_args):
        """Write a value or array of values to a channel and execute the user
        supplied callback after the put has completed.

        :param value:           data to be written. For multiple values use a list or tuple.
        :param req_type:        database request type (``ca.DBR_XXXX``). Defaults to be the native data type.
        :param count:           number of data values to write, Defaults to be the native count.
        :param callback:        function called when the write is completed.
        :param user_args:       user provided arguments that are passed to callback when it is invoked.
        :type value: int, float, bytes, str, list, tuple, array
        :type req_type: int, None
        :type count: int, None
        :type callback: callable
        :raises CaChannelException: if error happens

        Each Python callback function is required to have two arguments.
        The first argument is a dictionary containing the results of the action.

        =======  =======    =======
        field    type       comment
        =======  =======    =======
        chid     capsule    channels id structure
        type     int        database request type (ca.DBR_XXXX)
        count    int        number of values to transfered
        status   int        CA status return code (ca.ECA_XXXX)
        =======  =======    =======

        The second argument is a tuple containing any user arguments specified by *user_args*.
        If no arguments were specified then the tuple is empty.

        .. note:: All remote operation requests such as the above are accumulated (buffered) and not forwarded to
           the IOC until one of execution methods (:meth:`pend_io`, :meth:`poll`, :meth:`pend_event`, :meth:`flush_io`)
           is called. This allows several requests to be efficiently sent over the network in one message.

        >>> def putCB(epicsArgs, _):
        ...     print(ca.name(epicsArgs['chid']), 'put completed')
        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.array_put_callback(145, None, None, putCB)
        >>> status = chan.pend_event(1)
        catest put completed
        >>> chan = CaChannel('cabo')
        >>> chan.searchw()
        >>> chan.array_put_callback('Busy', ca.DBR_STRING, None, putCB)
        >>> status = chan.pend_event(1)
        cabo put completed
        >>> chan = CaChannel('cawave')
        >>> chan.searchw()
        >>> chan.array_put_callback([1,2,3], None, None, putCB)
        >>> status = chan.pend_event(1)
        cawave put completed
        >>> chan = CaChannel('cawavec')
        >>> chan.searchw()
        >>> chan.array_put_callback('123', None, None, putCB)
        >>> status = chan.pend_event(1)
        cawavec put completed
        """
        status = ca.put(self._chid, value, req_type, count, lambda epics_args: callback(epics_args, user_args))
        if status != ca.ECA_NORMAL:
            raise CaChannelException(status)
    #
    # Read methods
    #   getValue
    #   array_get
    #   array_get_callback
    #

    def getValue(self):
        """
        Return the value(s) after :meth:`array_get` has completed.

        :return: the value returned from the last array_get
        :rtype: int, float, str, list, array, dict

        If the *req_type* was not a plain type, the returned value is of dict type. It contains the same keys as in :meth:`array_get_callback`.

        .. seealso:: :meth:`array_get`

        >>> chan = CaChannel('cabo')
        >>> chan.searchw()
        >>> chan.putw(1)
        >>> chan.array_get(req_type=ca.DBR_CTRL_ENUM)
        >>> chan.pend_io()
        >>> for k,v in sorted(chan.getValue().items()):
        ...    print(k, v)
        pv_nostrings 2
        pv_severity AlarmSeverity.Minor
        pv_statestrings ('Done', 'Busy')
        pv_status AlarmCondition.State
        pv_value 1
        """
        if self._dbrvalue is None:
            return

        dbrvalue = self._dbrvalue.get()
        if isinstance(dbrvalue, dict):
            value = {}
            CaChannel._format_value(dbrvalue, value, self._dbrvalue.use_numpy)
        else:
            value = dbrvalue

        if not self._dbrvalue.use_numpy:
            if isinstance(value, dict):
                if hasattr(value['pv_value'], 'tolist'):
                    value['pv_value'] = value['pv_value'].tolist()
            else:
                if hasattr(value, 'tolist'):
                    value = value.tolist()
        return value

    def array_get(self, req_type=None, count=None, **keywords):
        """Read a value or array of values from a channel.

        The new value is not available until a subsequent :meth:`pend_io` returns :data:`ca.ECA_NORMAL`.
        Then it can be retrieved by a call to :meth:`getValue`.

        :param req_type:    database request type (:data:`ca.DBR_XXXX`). Defaults to be the native data type.
        :param count:       number of data values to read, Defaults to be the native count.
        :param keywords:    optional arguments assigned by keywords

                            ===========   ===================================================
                            keyword       value
                            ===========   ===================================================
                            use_numpy     True if waveform should be returned as numpy array.
                                          Default :data:`CaChannel.USE_NUMPY`.
                            ===========   ===================================================
        :type req_type: int, None
        :type count: int, None
        :raises CaChannelException: if error happens

        .. note:: All remote operation requests such as the above are accumulated (buffered) and not forwarded to
           the IOC until one of execution methods (:meth:`pend_io`, :meth:`poll`, :meth:`pend_event`, :meth:`flush_io`)
           is called. This allows several requests to be efficiently sent over the network in one message.

        .. seealso:: :meth:`getValue`

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.putw(123)
        >>> chan.array_get()
        >>> chan.pend_io()
        >>> chan.getValue()
        123.0
        """
        use_numpy = keywords.get('use_numpy', PACKAGE.USE_NUMPY)
        status, self._dbrvalue = ca.get(self._chid, req_type, count, None, use_numpy)
        if status != ca.ECA_NORMAL:
            raise CaChannelException(status)

    def array_get_callback(self, req_type, count, callback, *user_args, **keywords):
        """Read a value or array of values from a channel and execute the user
        supplied callback after the get has completed.

        :param req_type:    database request type (``ca.DBR_XXXX``). Defaults to be the native data type.
        :param count:       number of data values to read, Defaults to be the native count.
        :param callback:    function called when the get is completed.
        :param user_args:   user provided arguments that are passed to callback when it is invoked.
        :param keywords:    optional arguments assigned by keywords

                            ===========   ===================================================
                            keyword       value
                            ===========   ===================================================
                            use_numpy     True if waveform should be returned as numpy array.
                                          Default :data:`CaChannel.USE_NUMPY`.
                            ===========   ===================================================
        :type req_type:     int, None
        :type count:        int, None
        :type callback:     callable

        :raises CaChannelException: if error happens

        Each Python callback function is required to have two arguments.
        The first argument is a dictionary containing the results of the action.

        +-----------------+---------------+------------------------------------+-------------------------+---------------+-------------+---------------+
        | field           |  type         |  comment                           |       request type                                                    |
        |                 |               |                                    +----------+--------------+---------------+-------------+---------------+
        |                 |               |                                    | DBR_XXXX | DBR_STS_XXXX | DBR_TIME_XXXX | DBR_GR_XXXX | DBR_CTRL_XXXX |
        +=================+===============+====================================+==========+==============+===============+=============+===============+
        | chid            |   int         |   channels id number               |    X     |       X      |     X         |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | type            |   int         |   database request type            |    X     |       X      |     X         |   X         | X             |
        |                 |               |   (ca.DBR_XXXX)                    |          |              |               |             |               |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | count           |   int         |   number of values to transfered   |    X     |       X      |     X         |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | status          |   int         |   CA status return code            |    X     |       X      |     X         |   X         | X             |
        |                 |               |   (ca.ECA_XXXX)                    |          |              |               |             |               |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_value        |               |   PV value                         |    X     |       X      |     X         |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_status       |   int         |   PV alarm status                  |          |       X      |     X         |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_severity     |   int         |   PV alarm severity                |          |       X      |     X         |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_seconds      |   int         |   seconds part of timestamp        |          |              |     X         |             |               |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_nseconds     |   int         |   nanoseconds part of timestamp    |          |              |     X         |             |               |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_nostrings    |   int         |   ENUM PV's number of states       |          |              |               |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_statestrings |   string list |   ENUM PV's states string          |          |              |               |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_units        |   string      |   units                            |          |              |               |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_precision    |   int         |   precision                        |          |              |               |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_updislim     |   float       |   upper display limit              |          |              |               |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_lodislim     |   float       |   lower display limit              |          |              |               |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_upalarmlim   |   float       |   upper alarm limit                |          |              |               |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_upwarnlim    |   float       |   upper warning limit              |          |              |               |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_loalarmlim   |   float       |   lower alarm limit                |          |              |               |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_lowarnlim    |   float       |   lower warning limit              |          |              |               |   X         | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_upctrllim    |   float       |   upper control limit              |          |              |               |             | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+
        | pv_loctrllim    |   float       |   lower control limit              |          |              |               |             | X             |
        +-----------------+---------------+------------------------------------+----------+--------------+---------------+-------------+---------------+

        The second argument is a tuple containing any user arguments specified by *user_args*.
        If no arguments were specified then the tuple is empty.

        .. note:: All remote operation requests such as the above are accumulated (buffered)
           and not forwarded to the IOC until one of execution methods (:meth:`pend_io`, :meth:`poll`, :meth:`pend_event`, :meth:`flush_io`)
           is called. This allows several requests to be efficiently sent over the network in one message.

        >>> def getCB(epicsArgs, _):
        ...     for item in sorted(epicsArgs.keys()):
        ...         if item.startswith('pv_'):
        ...             print(item,epicsArgs[item])
        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.putw(145)
        >>> chan.array_get_callback(ca.DBR_CTRL_DOUBLE, 1, getCB)
        >>> status = chan.pend_event(1)
        pv_loalarmlim -20.0
        pv_loctrllim 0.0
        pv_lodislim -20.0
        pv_lowarnlim -10.0
        pv_precision 4
        pv_severity AlarmSeverity.Major
        pv_status AlarmCondition.HiHi
        pv_units mm
        pv_upalarmlim 20.0
        pv_upctrllim 0.0
        pv_updislim 20.0
        pv_upwarnlim 10.0
        pv_value 145.0
        >>> chan = CaChannel('cabo')
        >>> chan.searchw()
        >>> chan.putw(0)
        >>> chan.array_get_callback(ca.DBR_CTRL_ENUM, 1, getCB)
        >>> status = chan.pend_event(1)
        pv_nostrings 2
        pv_severity AlarmSeverity.No
        pv_statestrings ('Done', 'Busy')
        pv_status AlarmCondition.No
        pv_value 0
        """
        use_numpy = keywords.get('use_numpy', PACKAGE.USE_NUMPY)
        self._callbacks['getCB'] = (callback, user_args, use_numpy)
        status, _ = ca.get(self._chid, req_type, count, self._get_callback, use_numpy)
        if status != ca.ECA_NORMAL:
            raise CaChannelException(status)

    #
    # Monitor methods
    #   add_masked_array_event
    #   clear_event
    #
    def add_masked_array_event(self, req_type, count, mask, callback, *user_args, **keywords):
        """Specify a callback function to be executed whenever changes occur to a PV.

        Creates a new event id and stores it on self.__evid.  Only one event registered per CaChannel object.
        If an event is already registered the event is cleared before registering a new event.

        :param req_type:    database request type (``ca.DBR_XXXX``). Defaults to be the native data type.
        :param count:       number of data values to read, Defaults to be the native count.
        :param mask:        logical or of ``ca.DBE_VALUE``, ``ca.DBE_LOG``, ``ca.DBE_ALARM``.
                            Defaults to be ``ca.DBE_VALUE|ca.DBE_ALARM``.
        :param callback:    function called when the get is completed.
        :param user_args:   user provided arguments that are passed to callback when it is invoked.
        :param keywords:    optional arguments assigned by keywords

                            ===========   ===================================================
                            keyword       value
                            ===========   ===================================================
                            use_numpy     True if waveform should be returned as numpy array.
                                          Default :data:`CaChannel.USE_NUMPY`.
                            ===========   ===================================================
        :type req_type: int, None
        :type count: int, None
        :type mask: int, None
        :type callback: callable

        :raises CaChannelException: if error happens

        .. note:: All remote operation requests such as the above are accumulated (buffered) and not forwarded to
           the IOC until one of execution methods (:meth:`pend_io`, :meth:`poll`, :meth:`pend_event`, :meth:`flush_io`)
           is called. This allows several requests to be efficiently sent over the network in one message.

        >>> def eventCB(epicsArgs, _):
        ...     print('pv_value', epicsArgs['pv_value'])
        ...     print('pv_status', epicsArgs['pv_status'])
        ...     print('pv_severity', epicsArgs['pv_severity'])
        >>> chan = CaChannel('cabo')
        >>> chan.searchw()
        >>> chan.putw(1)
        >>> chan.add_masked_array_event(ca.DBR_STS_ENUM, None, None, eventCB)
        >>> status = chan.pend_event(1)
        pv_value 1
        pv_status AlarmCondition.State
        pv_severity AlarmSeverity.Minor
        >>> chan.add_masked_array_event(ca.DBR_STS_STRING, None, None, eventCB)
        >>> status = chan.pend_event(1)
        pv_value Busy
        pv_status AlarmCondition.State
        pv_severity AlarmSeverity.Minor
        >>> chan.clear_event()
        """
        if self._evid is not None:
            self.clear_event()
            self.flush_io()

        if mask is None:
            mask = ca.DBE_ALARM | ca.DBE_VALUE

        if req_type is None:
            req_type = self.field_type()

        if count is None:
            count = self.element_count()

        use_numpy = keywords.get('use_numpy', PACKAGE.USE_NUMPY)
        self._callbacks['eventCB'] = (callback, user_args, use_numpy)

        status, self._evid = ca.create_subscription(self._chid, self._event_callback, req_type, count, mask, use_numpy)

        if status != ca.ECA_NORMAL:
            raise CaChannelException(status)

    def clear_event(self):
        """Remove previously installed callback function.

        .. note:: All remote operation requests such as the above are accumulated (buffered) and not forwarded to
           the IOC until one of execution methods (:meth:`pend_io`, :meth:`poll`, :meth:`pend_event`, :meth:`flush_io`)
           is called. This allows several requests to be efficiently sent over the network in one message.
        """
        if self._evid is not None:
            status = ca.clear_subscription(self._evid)
            self._evid = None
            self._callbacks['eventCB'] = None
            if status != ca.ECA_NORMAL:
                raise CaChannelException(status)

    #
    # Execute methods
    #   pend_io
    #   pend_event
    #   poll
    #   flush_io
    #

    @attach_ca_context
    def pend_io(self, timeout=None):
        """
        Flush the send buffer and wait until outstanding queries (:meth:`search`, :meth:`array_get`) complete
        or the specified timeout expires.

        :param float timeout: seconds to wait
        :raises CaChannelException: if timeout or other error happens

        """
        if timeout is None:
            timeout = self.getTimeout()
        status = ca.pend_io(float(timeout))
        if status != ca.ECA_NORMAL:
            raise CaChannelException(status)

    @attach_ca_context
    def pend_event(self, timeout=None):
        """Flush the send buffer and process background activity (connect/get/put/monitor callbacks) for ``timeout`` seconds.

        It will not return before the specified timeout expires and all unfinished channel access labor has been processed.

        :param float timeout: seconds to wait
        :return: :data:`ca.ECA_TIMEOUT`
        """
        if timeout is None:
            timeout = 0.1
        status = ca.pend_event(timeout)
        # status is always ECA_TIMEOUT
        return status

    @attach_ca_context
    def poll(self):
        """Flush the send buffer and execute any outstanding background activity.

        :return: :data:`ca.ECA_TIMEOUT`

        .. note:: It is an alias to ``pend_event(1e-12)``.
        """
        status = ca.poll()
        # status is always ECA_TIMEOUT
        return status

    @attach_ca_context
    def flush_io(self):
        """
        Flush the send buffer and does not execute outstanding background activity.

        :raises CaChannelException: if error happens
        """
        status = ca.flush_io()
        if status != ca.ECA_NORMAL:
            raise CaChannelException(status)

    #
    # Channel Access Macros
    #   field_type
    #   element_count
    #   name
    #   state
    #   host_name
    #   read_access
    #   write_access
    #

    def field_type(self):
        """
        Native type of the PV in the server, :data:`ca.DBF_XXXX`.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> ftype = chan.field_type()
        >>> ftype
        <DBF.DOUBLE: 6>
        >>> ca.dbf_text(ftype)
        'DBF_DOUBLE'
        >>> ca.DBF_DOUBLE == ftype
        True
        """
        return ca.field_type(self._chid)

    def element_count(self):
        """
        Maximum array element count of the PV in the server.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.element_count()
        1
        """
        return ca.element_count(self._chid)

    def name(self):
        """
        Channel name specified when the channel was created.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.name()
        'catest'
        """
        return ca.name(self._chid)

    def state(self):
        """
        Current state of the CA connection.

            ==================  =====  =============
            States              Value  Meaning
            ==================  =====  =============
            ca.cs_never_conn    0      PV not found
            ca.cs_prev_conn     1      PV was found but unavailable
            ca.cs_conn          2      PV was found and available
            ca.cs_closed        3      PV not closed
            ca.cs_never_search  4      PV not searched yet
            ==================  =====  =============

        >>> chan = CaChannel('catest')
        >>> chan.state()
        <ChannelState.NEVER_SEARCH: 4>
        >>> chan.searchw()
        >>> chan.state()
        <ChannelState.CONN: 2>
        """
        return ca.state(self._chid)

    def host_name(self):
        """
        Host name that hosts the process variable.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> host_name = chan.host_name()

        """
        return ca.host_name(self._chid)

    def read_access(self):
        """Access right to read the channel.

        :return: True if the channel can be read, False otherwise.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.read_access()
        True
         """
        return ca.read_access(self._chid)

    def write_access(self):
        """Access right to write the channel.

        :return: True if the channel can be written, False otherwise.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.write_access()
        True
        """
        return ca.write_access(self._chid)

    #
    # Wait functions
    #
    # These functions wait for completion of the requested action.
    def searchw(self, pvName=None):
        """
        Attempt to establish a connection to a process variable.

        :param pvName: process variable name
        :type pvName: str, None
        :raises CaChannelException: if timeout or error happens

        .. note:: This method waits for connection to be established or fail with exception.

        >>> chan = CaChannel('non-exist-channel')
        >>> chan.searchw() # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
            ...
        CaChannelException: User specified timeout on IO operation expired
        """
        self.search(pvName)
        timeout = self.getTimeout()
        self.pend_io(timeout)

    def putw(self, value, req_type=None):
        """
        Write a value or array of values to a channel.

        If the request type is omitted the data is written as the Python type corresponding to the native format.
        Multi-element data is specified as a tuple or a list.
        Internally the sequence is converted to a list before inserting the values into a C array.
        Access using non-numerical types is restricted to the first element in the data field.
        Mixing character types with numerical types writes bogus results but is not prohibited at this time.
        :data:`ca.DBF_ENUM` fields can be written using :data:`ca.DBR_ENUM` and :data:`ca.DBR_STRING` types.
        :data:`ca.DBR_STRING` writes of a field of type :data:`ca.DBF_ENUM` must be accompanied by a valid string
        out of the possible enumerated values.

        :param value:    data to be written. For multiple values use a list or tuple
        :param req_type: database request type (:data:`ca.DBR_XXXX`). Defaults to be the native data type.
        :type value:     int, float, bytes, str, tuple, list, array
        :type req_type:  int, None
        :raises CaChannelException: if timeout or error happens

        .. note:: This method does flush the request to the channel access server.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.putw(145)
        >>> chan.getw()
        145.0
        >>> chan = CaChannel('cabo')
        >>> chan.searchw()
        >>> chan.putw('Busy', ca.DBR_STRING)
        >>> chan.getw()
        1
        >>> chan.getw(ca.DBR_STRING)
        'Busy'
        >>> chan = CaChannel('cawave')
        >>> chan.searchw()
        >>> chan.putw([1,2,3])
        >>> chan.getw(req_type=ca.DBR_LONG,count=4)
        [1, 2, 3, 0]
        >>> chan = CaChannel('cawavec')
        >>> chan.searchw()
        >>> chan.putw('123')
        >>> chan.getw(count=4)
        [49, 50, 51, 0]
        >>> chan.getw(req_type=ca.DBR_STRING)
        '123'
        >>> chan = CaChannel('cawaves')
        >>> chan.searchw()
        >>> chan.putw(['string 1','string 2'])
        >>> chan.getw()
        ['string 1', 'string 2', '']
        """
        self.array_put(value, req_type)
        self.flush_io()

    def getw(self, req_type=None, count=None, **keywords):
        """Read the value from a channel.

        If the request type is omitted the data is returned to the user as the Python type corresponding to the native format.
        Multi-element data has all the elements returned as items in a list and must be accessed using a numerical type.
        Access using non-numerical types is restricted to the first element in the data field.

        :data:`ca.DBF_ENUM` fields can be read using :data:`ca.DBR_ENUM` and :data:`ca.DBR_STRING` types.
        :data:`ca.DBR_STRING` reads of a field of type :data:`ca.DBF_ENUM` returns the string corresponding
        to the current enumerated value.

        :data:`ca.DBF_CHAR` fields can be read using :data:`ca.DBR_CHAR` and :data:`ca.DBR_STRING` types.
        :data:`ca.DBR_CHAR` returns a scalar or a sequnece of integers. :data:`ca.DBR_STRING` assumes each integer as a
        character and assemble a string.

        :param req_type:    database request type. Defaults to be the native data type.
        :param count:       number of data values to read, Defaults to be the native count.
        :param keywords:    optional arguments assigned by keywords

                            ===========   ===================================================
                            keyword       value
                            ===========   ===================================================
                            use_numpy     True if waveform should be returned as numpy array.
                                          Default :data:`CaChannel.USE_NUMPY`.
                            ===========   ===================================================
        :type req_type:     int, None
        :type count:        int, None
        :return:            If *req_type* is plain request type, only the value is returned.
                            Otherwise a dict returns with information depending on the request type,
                            same as the first argument passed to user's callback by :meth:`array_get_callback`.

        :raises CaChannelException: if timeout error happens

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.putw(0)
        >>> value = chan.getw(ca.DBR_TIME_DOUBLE)
        >>> for k,v in sorted(value.items()): # doctest: +ELLIPSIS
        ...    print(k, v)
        pv_nseconds ...
        pv_seconds ...
        pv_severity AlarmSeverity.No
        pv_status AlarmCondition.No
        pv_value 0.0

        .. versionchanged:: 3.0
           If *req_type* is DBR_XXX_STRING for a char type PV, a string will be returned from composing
           each element as a character.

        """
        # char waveform can be requested as string
        dbr_string_to_char = {
            ca.DBR_STRING: ca.DBR_CHAR,
            ca.DBR_STS_STRING: ca.DBR_STS_CHAR,
            ca.DBR_TIME_STRING: ca.DBR_TIME_CHAR,
            ca.DBR_GR_STRING: ca.DBR_GR_CHAR,
            ca.DBR_CTRL_STRING: ca.DBR_CTRL_CHAR
        }
        char_as_string = False
        if self.field_type() == ca.DBF_CHAR and req_type in dbr_string_to_char:
            req_type = dbr_string_to_char[req_type]
            char_as_string = True

        self.array_get(req_type, count, **keywords)
        timeout = self.getTimeout()
        self.pend_io(timeout)

        value = self.getValue()

        # convert char
        if char_as_string:
            if isinstance(value, dict):
                value['pv_value'] = CaChannel._ints_to_string(value['pv_value'])
            else:
                value = CaChannel._ints_to_string(value)

        return value

    #
    # Callback functions
    #
    # These functions hook user supplied callback functions to CA extension

    def _access_callback(self, epicsArgs):
        callback = self._callbacks.get('accessCB')
        if callback is None:
            return
        callbackFunc, userArgs = callback
        try:
            callbackFunc(epicsArgs, userArgs)
        except:
            traceback.print_exc()

    @classmethod
    def _printf_callback(cls, message):
        callback = cls.__callbacks.get('printfCB')
        if callback is None:
            return
        callbackFunc, userArgs = callback
        try:
            callbackFunc(message, userArgs)
        except:
            traceback.print_exc()

    @classmethod
    def _exception_callback(cls, epicsArgs):
        callback = cls.__callbacks.get('exceptionCB')
        if callback is None:
            return
        callbackFunc, userArgs = callback
        try:
            callbackFunc(epicsArgs, userArgs)
        except:
            traceback.print_exc()

    def _conn_callback(self, epicsArgs):
        callback = self._callbacks.get('connCB')
        if callback is None:
            return
        callbackFunc, userArgs = callback
        epicsArgs = (epicsArgs['chid'], epicsArgs['op'])
        try:
            callbackFunc(epicsArgs, userArgs)
        except:
            traceback.print_exc()

    def _put_callback(self, epicsArgs):
        callback = self._callbacks.get('putCB')
        if callback is None:
            return
        callbackFunc, userArgs = callback
        try:
            callbackFunc(epicsArgs, userArgs)
        except:
            traceback.print_exc()
        finally:
            self._callbacks['putCB'] = None

    def _get_callback(self, epicsArgs):
        callback = self._callbacks.get('getCB')
        if callback is None:
            return
        callbackFunc, userArgs, use_numpy = callback
        epicsArgs = CaChannel._format_cb_args(epicsArgs, use_numpy)
        try:
            callbackFunc(epicsArgs, userArgs)
        except:
            traceback.print_exc()
        finally:
            self._callbacks['getCB'] = None

    def _event_callback(self, epicsArgs):
        callback = self._callbacks.get('eventCB')
        if callback is None:
            return
        callbackFunc, userArgs, use_numpy = callback
        epicsArgs = CaChannel._format_cb_args(epicsArgs, use_numpy)
        try:
            callbackFunc(epicsArgs, userArgs)
        except:
            traceback.print_exc()

    @staticmethod
    def _format_cb_args(args, use_numpy):
        epicsArgs = {
            'chid': args['chid'],
            'type': args['type'],
            'count': args['count'],
            'status': args['status']
        }
        CaChannel._format_value(args['value'], epicsArgs, use_numpy)

        return epicsArgs

    @staticmethod
    def _format_value(value, epicsArgs, use_numpy):
        # dbr fields get renamed for CaChannel API
        key_map = {
            'lower_alarm_limit':   'loalarmlim',
            'lower_ctrl_limit':    'loctrllim',
            'lower_disp_limit':    'lodislim',
            'lower_warning_limit': 'lowarnlim',
            'upper_alarm_limit':   'upalarmlim',
            'upper_ctrl_limit':    'upctrllim',
            'upper_disp_limit':    'updislim',
            'upper_warning_limit': 'upwarnlim',
            'no_str': 'nostrings',
            'strs':   'statestrings'
        }
        if isinstance(value, dict):
            for key in value:
                # convert stamp dict
                if key == 'stamp':
                    epicsArgs['pv_seconds'] = value['stamp']['seconds'] - ca.POSIX_TIME_AT_EPICS_EPOCH
                    epicsArgs['pv_nseconds'] = value['stamp']['nanoseconds']
                else:
                    # dbr fields get renamed and 'pv_' prefix
                    new_key = key_map.get(key, key)
                    epicsArgs['pv_' + new_key] = value[key]
        else:
            epicsArgs['pv_value'] = value

        if not use_numpy and hasattr(epicsArgs['pv_value'], 'tolist'):
            epicsArgs['pv_value'] = epicsArgs['pv_value'].tolist()

        return epicsArgs

    @staticmethod
    def _ints_to_string(integers):
        if isinstance(integers, str):
            value = integers
        elif isinstance(integers, Sequence):
            stripped = itertools.takewhile(lambda c: c != 0, integers)
            if sys.hexversion < 0x03000000:
                value = ''.join([chr(c) for c in stripped])
            else:
                value = bytes(stripped).decode()
        elif isinstance(integers, numbers.Integral):
            if integers == 0:
                value = ''
            elif sys.hexversion < 0x03000000:
                value = chr(integers)
            else:
                value = bytes([integers]).decode()
        else:
            value = None

        return value

if __name__ == "__main__":
    import doctest
    doctest.testmod()
