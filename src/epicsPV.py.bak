"""
This module defines the epicsPV class, which adds additional features to
Geoff Savage's CaChannel class.

Author:         Mark Rivers
Created:        Sept. 16, 2002.
Modifications:

- Mar. 25, 2014 Xiaoqiang Wang

  - Fix the call sequence inside getCallback

- Mar. 7, 2017 Xiaoqiang Wang

  - Reformat the docstring and code indent.

"""
from CaChannel import ca, CaChannel


class epicsPV(CaChannel):
    """
    This class subclasses CaChannel class to add the following
    features:

       - If a PV name is given then the class constructor will
         do a :meth:`CaChannel.CaChannel.searchw` by default.

       - :meth:`setMonitor` sets a generic callback routine for value change events.
         Subsequent :meth:`getw`, :meth:`getValue` or :meth:`array_get` calls will return the
         value from the most recent callback, and hence do not result in any
         network activity or latency. This can greatly improve performance.

       - :meth:`checkMonitor` returns a flag to indicate if a callback has occured
         since the last call to :meth:`checkMonitor`, :meth:`getw`, :meth:`getValue` or
         :meth:`array_get`. It can be used to increase efficiency in polling applications.

       - :meth:`getControl` reads the "control" and other information from an
         EPICS PV without having to use callbacks. In addition to the PV value,
         this will return the graphic, control and alarm limits, etc.

       - :meth:`putWait` calls :meth:`CaChannel.CaChannel.array_put_callback` and waits for the callback to
         occur before it returns. This allows programs to wait for record being processed
         synchronously and without user-written callbacks.
    """

    def __init__(self, pvName=None, wait=False):
        """
        Create an EPICS channel if pvName is specified, and optionally wait for connection.

        :param str pvName: An optional name of an EPICS Process Variable.
        :param bool wait: If wait is True and pvName is not None then this constructor will do a
                          :meth:`CaChannel.CaChannel.searchw` on the PV. If wait is False and pvName ist not None then
                          this constructor will do a :meth:`CaChannel.CaChannel.search` on the PV, and the user
                          must subsequently do a :meth:`CaChannel.CaChannel.pend_io` on this
                          or another :class:`epicsPV` or :class:`CaChannel` object.
        """
        # Invoke the base class initialization
        self.callBack = callBack()
        CaChannel.__init__(self)
        if pvName is not None:
            if wait:
                self.searchw(pvName)
            else:
                self.search(pvName)

    def setMonitor(self):
        """
        Sets a generic callback routine for value change events.

        Subsequent :meth:`getw`, :meth:`getValue` or :meth:`array_get` calls will return the
        value from the most recent callback, do not result in any network
        latency. This can greatly improve efficiency.
        """
        if self.callBack.monitorState != 0:
            return

        self.add_masked_array_event(None, None, ca.DBE_VALUE,
                                    getCallback, self.callBack)
        self.callBack.monitorState = 1

    def clearMonitor(self):
        """
        Cancels the effect of a previous call to :meth:`setMonitor`.

        Subsequent :meth:`getw`, :meth:`getValue` or :meth:`array_get` calls will no longer
        return the value from the most recent callback, but will actually result
        in channel access calls.
        """
        self.clear_event()
        self.callBack.monitorState = 0

    def checkMonitor(self):
        """
        Returns 1 to indicate if a value callback has occured
        since the last call to :meth:`checkMonitor`, :meth:`getw`, :meth:`getValue` or
        :meth:`array_get`, indicating that a new value is available.
        Returns 0 if no such callback has occurred.
        It can be used to increase efficiency in polling applications.
        """
        # This should be self.poll(), but that is generating errors
        self.pend_event(.0001)
        m = self.callBack.newMonitor
        self.callBack.newMonitor = 0
        return m

    def getControl(self, req_type=None, count=None, wait=1, poll=.01):
        """
        Provides a method to read the "control" and other information from an
        EPICS PV without having to use callbacks.

        It calls :meth:`CaChannel.CaChannel.CaChannel.array_get_callback`
        with a database request type of ca.dbf_type_to_DBR_CTRL(req_type).
        In addition to the PV value, this will return the graphic, control and
        alarm limits, etc.

        :param int req_type: request type. Default to field type.
        :param int count: number of elements. Default to native element count.
        :param bool wait: If this keyword is 1 (the default) then this routine waits for
                          the callback before returning. If this keyword is 0 then it is
                          the user's responsibility to wait or check for the callback
                          by calling :meth:`checkMonitor`.
        :param float poll: The timeout for :meth:`CaChannel.CaChannel.pend_event` calls, waiting for the callback
                           to occur. Shorter times reduce the latency at the price of CPU cycles.

        >>> pv = epicsPV('13IDC:m1')
        >>> pv.getControl()
        >>> for field in dir(pv.callBack):
        ...    print field, ':', getattr(pv.callBack, field)
            chid : _bfffec34_chid_p
            count : 1
            monitorState : 0
            newMonitor : 1
            putComplete : 0
            pv_loalarmlim : 0.0
            pv_loctrllim : -22.0
            pv_lodislim : -22.0
            pv_lowarnlim : 0.0
            pv_precision : 4
            pv_riscpad0 : 256
            pv_severity : 0
            pv_status : 0
            pv_units : mm
            pv_upalarmlim : 0.0
            pv_upctrllim : 28.0
            pv_updislim : 28.0
            pv_upwarnlim : 0.0
            pv_value : -15.0
            status : 1
            type : 34
        """
        if req_type is None: req_type = self.field_type()
        if wait: self.callBack.newMonitor = 0
        self.array_get_callback(ca.dbf_type_to_DBR_CTRL(req_type),
                                count, getCallback, self.callBack)
        if wait:
            while self.callBack.newMonitor == 0:
                self.pend_event(poll)

    def array_get(self, req_type=None, count=None, **keywords):
        """
        If :meth:`setMonitor` has not been called then this function simply calls
        :meth:`CaChannel.CaChannel.array_get`.  If :meth:`setMonitor` has been called then it calls
        :meth:`CaChannel.CaChannel.pend_event` with a very short timeout, and then returns the
        PV value from the last callback.
        """
        if self.callBack.monitorState != 0:
            # This should be self.poll(), but that is generating errors
            self.pend_event(.0001)
        if self.callBack.monitorState == 2:
            self.callBack.newMonitor = 0
            return self.callBack.pv_value
        else:
            return CaChannel.array_get(self, req_type, count, **keywords)

    def getw(self, req_type=None, count=None, **keywords):
        """
        If :meth:`setMonitor` has not been called then this function simply calls
        :meth:`CaChannel.CaChannel.getw`.  If :meth:`setMonitor` has been called then it calls
        :meth:`CaChannel.CaChannel.pend_event` with a very short timeout, and then returns the
        PV value from the last callback.
        """
        if self.callBack.monitorState != 0:
            # This should be self.poll(), but that is generating errors
            self.pend_event(.0001)
        if self.callBack.monitorState == 2:
            self.callBack.newMonitor = 0
            if count is None:
                return self.callBack.pv_value
            else:
                return self.callBack.pv_value[0:count]
        else:
            return CaChannel.getw(self, req_type, count, **keywords)

    def getValue(self):
        """
        If :meth:`setMonitor` has not been called then this function simply calls
        :meth:`CaChannel.CaChannel.getValue`.  If setMonitor has been called then it calls
        :meth:`CaChannel.CaChannel.pend_event` with a very short timeout, and then returns the
        PV value from the last callback.
        """
        if self.callBack.monitorState != 0:
            # This should be self.poll(), but that is generating errors
            self.pend_event(.0001)
        if self.callBack.monitorState == 2:
            self.callBack.newMonitor = 0
            return self.callBack.pv_value
        else:
            return CaChannel.getValue(self)

    def putWait(self, value, req_type=None, count=None, poll=.01):
        """
        Calls :meth:`CaChannel.CaChannel.array_put_callback` and waits for the callback to
        occur before it returns. This allows programs to wait for record being processed
        without having to handle asynchronous callbacks.

        :param value:      data to be written. For multiple values use a list or tuple
        :param req_type:   database request type (``ca.DBR_XXXX``). Defaults to be the native data type.
        :param int count:  number of data values to write. Defaults to be the native count.
        :param float poll: The timeout for :meth:`CaChannel.CaChannel.pend_event` calls, waiting for the callback to occur.
                           Shorter times reduce the latency at the price of CPU cycles.
        """
        self.callBack.putComplete = 0
        self.array_put_callback(value, req_type, count, putCallBack, self.callBack)
        while self.callBack.putComplete == 0:
            self.pend_event(poll)

class callBack:
    """
    This class is used by the epicsPV class to handle callbacks.  It is required
    to avoid circular references to the epicsPV object itself when dealing with
    callbacks, in order to allow the CaChannel destructor to be called.
    Users will only be interested in the fields that are copied to this class in
    the callback resulting from a call to epicsPV.getControl().
    """
    def __init__(self):
        self.newMonitor = 0
        self.putComplete = 0
        self.monitorState = 0
        self.pv_value = None
        # monitorState:
        #   0=not monitored
        #   1=monitor requested, but no callback yet
        #   2=monitor requested, callback has arrived


def putCallBack(epicsArgs, userArgs):
    """
    This is the generic callback function used by the epicsPV.putWait() method.
    It simply sets the callBack.putComplete flag to 1.
    """
    userArgs[0].putComplete = 1

def getCallback(epicsArgs, userArgs):
    """
    This is the generic callback function enabled by the epicsPV.setMonitor() method.
    It sets the callBack.monitorState flag to 2, indicating that a monitor has
    been received.  It copies all of the attributes in the epicsArgs dictionary
    to the callBack attribute of the epicsPV object.
    """
    for key in epicsArgs.keys():
        setattr(userArgs[0], key, epicsArgs[key])
    if userArgs[0].monitorState == 1: userArgs[0].monitorState = 2
    userArgs[0].newMonitor = 1
