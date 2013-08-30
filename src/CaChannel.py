"""
CaChannel class having identical API as of caPython/CaChannel class, 
based on PythonCA ( > 1.20.1beta2)

Author:     Xiaoqiang Wang
Created:    Sep. 22, 2008
Changes:     
"""

import ca, time, types

ca.cs_never_search = 4

class CaChannelException(Exception):
    def __init__(self, status):
        self.status = str(status)
    def __str__(self):
        return self.status

class CaChannel:
    """CaChannel: A Python class with identical API as of caPython/CaChannel 

    Example:
        import CaChannel
        chan = CaChannel.CaChannel('catest')
        chan.searchw()
        print chan.getw()
    """

    ca_timeout = 1.0

    dbr_d = {}
    dbr_d[ca.DBR_SHORT] = int
    dbr_d[ca.DBR_INT]   = int
    dbr_d[ca.DBR_LONG]  = int
    dbr_d[ca.DBR_FLOAT] = float
    dbr_d[ca.DBR_DOUBLE]= float
    dbr_d[ca.DBR_CHAR]  = int
    dbr_d[ca.DBR_STRING]= str
    dbr_d[ca.DBR_ENUM]  = int

    def __init__(self, pvName=None):
        self.pvname = pvName
        self.__chid = None
        self.__evid = None
        self.__timeout = None
        self._field_type = None
        self._element_count = None
        self._puser = None
        self._conn_state = None
        self._host_name = None
        self._raccess = None
        self._waccess = None

        self._callbacks={}

    def __del__(self):
        try:
            self.clear_event()
            self.clear_channel()
            self.flush_io()
        except:
            pass

    def version(self):
        print "CaChannel, version v24-01-11"
#
# Class helper methods
#
    def setTimeout(self, timeout):
        """Set the timeout for this channel."""
        if (timeout>=0 or timeout == None):
            self.__timeout = timeout
        else:
            raise ValueError
    def getTimeout(self):
        """Retrieve the timeout set for this channel."""
        return self.__timeout


#
# *************** Channel access medthod ***************
#

#
# Connection methods
#   search_and_connect
#   search
#   clear_channel

    def search_and_connect(self, pvName, callback, *user_args):
        """Attempt to establish a connection to a process variable.
        Parameters:
            pvName: process variable name
            callback: function called when connection completes and connection
            status changes later on.
            *user_args: user provided arguments that are passed to callback when
            it is invoked.

        >>> chan = CaChannel('catest')
        >>> def connCB(epicsArgs, userArgs):
        ...     if epicsArgs[1] == 6:
        ...         print ca.name(epicsArgs[0]), "is connected"
        >>> chan.search_and_connect(None, connCB, chan)
        >>> status = chan.pend_event(2)
        catest is connected
        """
        if pvName == None:
            pvName = self.pvname
        else:
            self.pvname = pvName
        self._callbacks['connCB']=(callback, user_args)
        try:
            self.__chid = ca.search(pvName, self._conn_callback)
        except ca.error, msg:
            raise CaChannelException,msg 

    def search(self, pvName=None):
        """Attempt to establish a connection to a process variable.
        Parameters:
            pvName: process variable name

        >>> chan = CaChannel()
        >>> chan.search('catest')
        >>> status = chan.pend_io(1)
        >>> chan.state()
        2
        """
        if pvName == None:
            pvName = self.pvname
        else:
            self.pvname = pvName
        try:
            self.__chid = ca.search(pvName, None)
        except ca.error, msg:
            raise CaChannelException,msg

    def clear_channel(self):
        """Close a channel created by one of the search functions"""
        if(self.__chid is not None):
            try:
                status = ca.clear(self.__chid)
            except ca.error,msg:
                raise CaChannelException,msg 

#
# Write methods
#   array_put
#   array_put_callback
#

    def _setup_put(self,value, req_type, count = None):
        if count is None:
            count = self.element_count()
        else:
            count = max(1, min(self.element_count(), count) )

        if req_type == -1:
            req_type = self.field_type()

        # single numeric value
        if (isinstance(value, int) or 
            isinstance(value, long) or 
            isinstance(value, float) or
            isinstance(value, bool)):
            pval = (CaChannel.dbr_d[req_type](value),)
        # single string value
        #   if DBR_CHAR, split into chars
        #   otherwise convert to field type
        elif isinstance(value, str):
            if req_type == ca.DBR_CHAR:
                if len(value) < count:
                    count = len(value)
                pval = [ord(x) for x in value[:count]]
            else:
                pval = (CaChannel.dbr_d[req_type](value),)
        # assumes other sequence type
        else:
            if len(value) < count:
                count = len(value)
            pval = [CaChannel.dbr_d[req_type](x) for x in value[:count]]

        return pval

    def array_put(self, value, req_type=None, count=None):
        """Write a value or array of values to a channel
        Parameters:
            value: data to be written. For multiple values use a list or tuple
            req_type: database request type. Defaults to be the native data type.
            count: number of data values to write, Defaults to be the native count.

        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.array_put(123)
        >>> chan.pend_io(1)
        >>> chan.getw()
        123.0
        >>> chan = CaChannel('cabo')
        >>> chan.searchw()
        >>> chan.array_put('Busy', ca.DBR_STRING)
        >>> chan.pend_io(1)
        >>> chan.getw()
        1
        >>> chan = CaChannel('cawave')
        >>> chan.searchw()
        >>> chan.array_put([1,2,3])
        >>> chan.pend_io(1)
        >>> chan.getw()
        [1.0, 2.0, 3.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
        >>> chan = CaChannel('cawavec')
        >>> chan.searchw()
        >>> chan.array_put('1234',count=3)
        >>> chan.pend_io(1)
        >>> chan.getw(count=4)
        [49, 50, 51, 0]
        """
        if req_type is None: req_type = -1
        val = self._setup_put(value, req_type, count)
        try:
            ca.put(self.__chid, val, None, None, req_type)
        except ca.error,msg:
            raise CaChannelException,msg

    def array_put_callback(self, value, req_type, count, callback, *user_args):
        """Write a value or array of values to a channel and execute the user
        supplied callback after the put has completed.
        Parameters:
            value: data to be written. For multiple values use a list or tuple.
            req_type: database request type. Defaults to be the native data type.
            count: number of data values to write, Defaults to be the native count.
            callback: function called when the write is completed.
            *user_args: user provided arguments that are passed to callback when
            it is invoked.

        >>> def putCB(epicsArgs, userArgs):
        ...     print ca.name(epicsArgs['chid']), 'put completed'
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
        if req_type is None: req_type = -1
        val = self._setup_put(value, req_type, count)
        self._callbacks['putCB']=(callback, user_args)
        try:
            ca.put(self.__chid, val, None, self._put_callback, req_type)
        except ca.error,msg:
            raise CaChannelException,msg
#
# Read methods
#   getValue
#   array_get
#   array_get_callback
#

    # Obtain read value after ECA_NORMAL is returned on an array_get(). 
    def getValue(self):
        """Return the value(s) after array_get has completed"""
        return self.val

    # Simulate with a synchronous getw function call
    def array_get(self, req_type=None, count=None):
        """Read a value or array of values from a channel. The new value is
        retrieved by a call to getValue method.
        Parameters:
            req_type: database request type. Defaults to be the native data type.
            count: number of data values to read, Defaults to be the native count.
        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.putw(123)
        >>> chan.array_get()
        >>> chan.getValue()
        123.0
        """
        self.val = self.getw(req_type, count)

    def array_get_callback(self, req_type, count, callback, *user_args):
        """Read a value or array of values from a channel and execute the user
        supplied callback after the get has completed.
        Parameters:
            req_type: database request type. Defaults to be the native data type.
            count: number of data values to read, Defaults to be the native count.
            callback: function called when the get is completed.
            *user_args: user provided arguments that are passed to callback when
            it is invoked.
        >>> def getCB(epicsArgs, userArgs):
        ...     for item in sorted(epicsArgs.keys()):
        ...         if item.startswith('pv_'):
        ...             print item,epicsArgs[item]
        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.putw(145)
        >>> chan.array_get_callback(ca.DBR_CTRL_DOUBLE, 1, getCB)
        >>> status = chan.pend_event(1)
        pv_loalarmlim -20.0
        pv_loctrllim 0.0
        pv_lodislim 0.0
        pv_lowarnlim -10.0
        pv_precision 3
        pv_seconds 0.0
        pv_severity 2
        pv_status 3
        pv_units mm
        pv_upalarmlim 20.0
        pv_upctrllim 0.0
        pv_updislim 0.0
        pv_upwarnlim 10.0
        pv_value 145.0
        >>> chan = CaChannel('cabo')
        >>> chan.searchw()
        >>> chan.putw(0)
        >>> chan.array_get_callback(ca.DBR_CTRL_ENUM, 1, getCB)
        >>> status = chan.pend_event(1)
        pv_nostrings 2
        pv_seconds 0.0
        pv_severity 0
        pv_statestrings ('Done', 'Busy')
        pv_status 0
        pv_value 0
        """
        if req_type is None: req_type = ca.dbf_type_to_DBR(self.field_type())
        if count is None: count = 0
        self._callbacks['getCB']=(callback, user_args)
        try:
            status=ca.get(self.__chid, self._get_callback, req_type, count)
        except ca.error,msg:
            raise CaChannelException,msg

#
# Event methods
#   add_masked_array_event
#   clear_event
#

    # Creates a new event id and stores it on self.__evid.  Only one event registered
    # per CaChannel object.  If an event is already registered the event is cleared
    # before registering a new event.
    def add_masked_array_event(self, req_type, count, mask, callback, *user_args):
        """Specify a callback function to be executed whenever changes occur to a PV.
        Parameters:
            req_type: database request type. Defaults to be the native data type.
            count: number of data values to read, Defaults to be the native count.
            mask: logical or of ca.DBE_VALUE, ca.DBE_LOG, ca.DBE_ALARM. Defaults to
            be ca.DBE_VALUE|ca.DBE_ALARM.
            callback: function called when the get is completed.
            *user_args: user provided arguments that are passed to callback when
            it is invoked.
        
        >>> def eventCB(epicsArgs, userArgs):
        ...     print 'pv_value', epicsArgs['pv_value']
        ...     print 'pv_status', epicsArgs['pv_status'], ca.alarmStatusString(epicsArgs['pv_status'])
        ...     print 'pv_severity', epicsArgs['pv_severity'], ca.alarmSeverityString(epicsArgs['pv_severity'])
        >>> chan = CaChannel('cabo')
        >>> chan.searchw()
        >>> chan.putw(1)
        >>> chan.add_masked_array_event(ca.DBR_STS_ENUM, None, None, eventCB)
        >>> status = chan.pend_event(1)
        pv_value 1
        pv_status 7 STATE
        pv_severity 1 MINOR
        >>> chan.clear_event()
        """
        if req_type is None: req_type = ca.dbf_type_to_DBR(self.field_type())
        if count is None: count = 0
        if mask is None: mask = ca.DBE_VALUE|ca.DBE_ALARM
        if self.__evid is not None:
            self.clear_event()
            self.pend_io()
        self._callbacks['eventCB']=(callback, user_args)
        try:
            self.__evid = ca.monitor(self.__chid, self._event_callback, count, mask)
        except ca.error,msg:
            raise CaChannelException,msg

    def clear_event(self):
        """Remove previously installed callback function."""
        if self.__evid is not None:
            try:
                status=ca.clear_monitor(self.__evid)
                self.__evid = None
            except ca.error,msg:
               raise CaChannelException,msg 

#
# Execute methods
#   pend_io
#   pend_event
#   poll
#   flush_io
#

    def pend_io(self,timeout=None):
        """Flush the send buffer and wait until outstanding queries complete
        or the specified timeout expires.
        Parameters:
            timeout: seconds to wait
        """
        if timeout is None:
            if self.__timeout is None:
                timeout = CaChannel.ca_timeout
            else:
                timeout = self.__timeout
        status = ca.pend_io(float(timeout))
        if status != 0:
            raise CaChannelException, ca.caError._caErrorMsg[status]

    def pend_event(self,timeout=None):
        """Flush the send buffer and wait for timeout seconds.
        Parameters:
            timeout: seconds to wait
        """
        if timeout is None:
            timeout = 0.1
        status = ca.pend_event(timeout)
        # status is always ECA_TIMEOUT
        return status

    def poll(self):
        """Flush the send buffer and execute outstanding background activity."""
        status = ca.poll() 
        # status is always ECA_TIMEOUT
        return status

    def flush_io(self):
        """Flush the send buffer and does not execute outstanding background activity."""
        status = ca.flush()
        if status != 0:
            raise CaChannelException, ca.caError._caErrorMsg[status]

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
    def get_info(self):
        try:
            info=(self._field_type, self._element_count, self._puser,
                  self._conn_state, self._host_name, self._raccess,
                  self._waccess) = ca.ch_info(self.__chid)
        except ca.error,msg:
            raise CaChannelException,msg
        return info


    def field_type(self):
        """Native field type.
        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> ftype = chan.field_type()
        >>> ftype
        6
        >>> ca.dbf_text(ftype)
        'DBF_DOUBLE'
        """
        self.get_info()
        return self._field_type

    def element_count(self):
        """Native element count.
        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.element_count()
        1
        """
        self.get_info()
        return self._element_count

    def name(self):
        """Channel name specified when the channel was connected.
        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.name()
        'catest'
        """
        return ca.name(self.__chid)

    def state(self):
        """Current state of the connections.
        Possible channel states:
            ca.cs_never_conn    PV not found
            ca.cs_prev_conn     PV was found but unavailable
            ca.cs_conn          PV was found and available
            ca.cs_closed        PV not closed
            ca.cs_never_search  PV not searched yet
        >>> chan = CaChannel('catest')
        >>> chan.searchw()
        >>> chan.state()
        2
        """
        if self.__chid is None:
            return ca.cs_never_search
        else:
            self.get_info()
            return self._conn_state

    def host_name(self):
        """Host name that hosts the process variable."""
        self.get_info()
        return self._host_name

    def read_access(self):
        """Right to read the channel."""
        self.get_info()
        return self._raccess

    def write_access(self):
        """Right to write the channel."""
        self.get_info()
        return self._waccess
#
# Wait functions
#
# These functions wait for completion of the requested action.
    def searchw(self, pvName=None):
        """Attempt to establish a connection to a process variable.
        Parameters:
            pvName: process variable name
        >>> chan = CaChannel('non-exist-channel')
        >>> chan.searchw()
        Traceback (most recent call last):
            ...
        CaChannelException: User specified timeout on IO operation expired
        """
        if pvName is None:
            pvName = self.pvname
        else:
            self.pvname = pvName
        self.__chid = ca.search(pvName, None)
        if self.__timeout is None:
            timeout = CaChannel.ca_timeout
        else:
            timeout = self.__timeout
        status = ca.pend_io(timeout)
        if status != 0:
            raise CaChannelException, ca.caError._caErrorMsg[status]

    def putw(self, value, req_type=None):
        """Write a value or array of values to a channel
        Parameters:
            value: data to be written. For multiple values use a list or tuple
            req_type: database request type. Defaults to be the native data type.

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
        >>> chan = CaChannel('cawaves')
        >>> chan.searchw()
        >>> chan.putw(['string 1','string 2'])
        >>> chan.getw()
        ['string 1', 'string 2', '']
        """
        if req_type is None: req_type = -1
        val = self._setup_put(value, req_type)
        try:
            ca.put(self.__chid, val, None, None, req_type)
        except ca.error,msg:
            raise CaChannelException,msg
        if self.__timeout is None:
            timeout = CaChannel.ca_timeout
        else:
            timeout = self.__timeout
        status = ca.pend_io(timeout)
        if status != 0:
            raise CaChannelException, ca.caError._caErrorMsg[status]

    def getw(self, req_type=None, count=None):
        """Return a value or array of values from a channel.
        Parameters:
            req_type: database request type. Defaults to be the native data type.
            count: number of data values to read, Defaults to be the native count.
        """
        updated = [False]
        value = [0]
        def update_value(valstat):
            if valstat is None:
                return
            try:
                value[0] = valstat[0]
            finally:
                updated[0] = True
        if req_type is None: req_type = ca.dbf_type_to_DBR(self.field_type())
        if count is None: count = 0
        try:
            ca.get(self.__chid, update_value, req_type, count)
        except ca.error,msg:
            raise CaChannelException,msg
        if self.__timeout is None:
            timeout = CaChannel.ca_timeout
        else:
            timeout = self.__timeout
        status = ca.pend_io(timeout)
        n = 0
        while n*0.02 < timeout and not updated[0]:
            ca.pend_event(0.02)
            n+=1
        if not updated[0]:
            raise CaChannelException, ca.caError._caErrorMsg[10] # ECA_TIMEOUT
        return value[0]

#
# Callback functions
#
# These functions hook user supplied callback functions to CA extension

    def _conn_callback(self):
        try:
            callback, userArgs = self._callbacks.get('connCB')
        except:
            return
        if self.state() == 2: OP=6
        else: OP=7
        epicsArgs = (self.__chid, OP)
        callback(epicsArgs, userArgs)

    def _put_callback(self, args):
        try:
            callback, userArgs = self._callbacks.get('putCB')
        except:
            return
        epicsArgs={}
        epicsArgs['chid']=self.__chid
        epicsArgs['type']=self.field_type()
        epicsArgs['count']=self.element_count()
        epicsArgs['status']=args[1]
        callback(epicsArgs, userArgs)

    def _get_callback(self, args):
        try:
            callback, userArgs = self._callbacks.get('getCB')
        except:
            return
        epicsArgs = self._format_cb_args(args)
        callback(epicsArgs, userArgs)

    def _event_callback(self, args): 
        try:
            callback, userArgs = self._callbacks.get('eventCB')
        except:
            return
        epicsArgs = self._format_cb_args(args)
        callback(epicsArgs, userArgs)

    def _format_cb_args(self, args):
        epicsArgs={}
        epicsArgs['chid']   = self.__chid
        # dbr_type is not returned
        # use dbf_type instead
        epicsArgs['type']   = self.field_type()
        epicsArgs['count']  = self.element_count()
        # status flag is not returned,
        # args[1] is alarm status
        # assume ECA_NORMAL
        epicsArgs['status'] = 1
        if len(args)==2:          # Error
            epicsArgs['pv_value']   = args[0] # always None
            epicsArgs['status']     = args[1]
        if len(args)>=3:          # DBR_Plain
            epicsArgs['pv_value']   = args[0]
            epicsArgs['pv_severity']= args[1]
            epicsArgs['pv_status']  = args[2]
        if len(args)>=4:          # DBR_TIME, 0.0 for others
            epicsArgs['pv_seconds'] = args[3]
        if len(args)==5:
            if len(args[4])==2:   # DBR_CTRL_ENUM
                epicsArgs['pv_nostrings']   = args[4][0]
                epicsArgs['pv_statestrings']= args[4][1]
            if len(args[4])>=7:   # DBR_GR
                epicsArgs['pv_units']       = args[4][0]
                epicsArgs['pv_updislim']    = args[4][1]
                epicsArgs['pv_lodislim']    = args[4][2]
                epicsArgs['pv_upalarmlim']  = args[4][3]
                epicsArgs['pv_upwarnlim']   = args[4][4]
                epicsArgs['pv_loalarmlim']  = args[4][5]
                epicsArgs['pv_lowarnlim']   = args[4][6]
            if len(args[4])==8:   # DBR_GR_FLOAT or DBR_GR_DOUBLE
                epicsArgs['pv_precision']   = args[4][7]
            if len(args[4])>=9:   # DBR_CTRL
                epicsArgs['pv_upctrllim']   = args[4][7]
                epicsArgs['pv_loctrllim']   = args[4][8]
            if len(args[4])==10:  # DBR_CTRL_FLOAT or DBR_CTRL_DOUBLE
                epicsArgs['pv_precision']   = args[4][9]
        return epicsArgs

if __name__ == "__main__":
    import doctest
    doctest.testmod()
