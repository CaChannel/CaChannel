import collections
import datetime
import itertools
import numbers
import sys
import threading

from .CaChannel import ca, CaChannel

#: channel object cache
_channels_ = {}


def _ints_to_string(integers):
    if isinstance(integers, collections.Sequence):
        stripped = itertools.takewhile(lambda x: x != 0, integers)
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


def _get_or_create_channel(name):
    """
    return the channel object associated with *name*. If nothing exists, create a new one.

    :param str name: pv name
    :return: channel object
    :rtype: :class:`CaChannel.CaChannel`
    """
    chan = _channels_.get(name)
    if chan is None:
        chan = CaChannel(name)
        chan.searchw()
        _channels_[name] = chan
    return chan


def caget(name, as_string=False, count=None):
    """
    Return PV's current value.

    For enum or char type PV, the string form is returned if *as_string* is True.
    If the PV is of multi-element array, *count* can be used to limit the number
    of elements.

    :param str name: pv name
    :param bool as_string: retrieve enum and char type as string
    :param int count: number of element to request
    :return: pv value

    >>> caget('catest')
    0.0
    >>> caget('cabo', as_string=True)
    'Done'
    >>> caget('cawavec', as_string=True)
    ''
    >>> caget('cawave', count=4)
    [0.0, 0.0, 0.0, 0.0]
    """
    chan = _get_or_create_channel(name)

    req_type = ca.dbf_type_to_DBR(chan.field_type())
    if as_string and req_type == ca.DBR_ENUM:
        req_type = ca.DBR_STRING

    value = chan.getw(req_type, count)

    if as_string and req_type == ca.DBR_CHAR:
        value = _ints_to_string(value)

    return value


def caput(name, value, wait=False, timeout=None):
    """

    :param str name: pv name
    :param value: value to write
    :param bool wait: wait for completion
    :param float timeout: seconds to wait

    >>> caput('catest', 0, wait=True)
    >>> caput('cabo', 'Done')
    >>> caput('cawavec', 'this can be a long string')
    >>> caput('cawave', [0, 0, 0 ,0])
    """
    chan = _get_or_create_channel(name)

    def put_callback(epics_args, user_args):
        user_args[0].set()

    if wait:
        event = threading.Event()
        chan.array_put_callback(value, None, None, put_callback, event)
        chan.flush_io()
        event.wait(timeout)
    else:
        chan.putw(value)


def caunmonitor(name):
    """
    remove installed callback

    :param name: pv name
    """
    chan = _channels_.get(name)
    if chan is not None:
        chan.clear_event()


def camonitor(name, as_string=False, count=None, callback=None):
    """
    set a *callback* to be invoked when pv value or alarm status change.

    :param str name: pv name
    :param bool as_string: retrieve enum and char type as string
    :param int count: number of element to request
    :param callback: callback function. This will remove any previous installed callback.
                     If *None* is specified, the default behavior is to print to the console.
    """
    chan = _get_or_create_channel(name)

    req_type = ca.dbf_type_to_DBR_TIME(chan.field_type())
    if as_string and req_type == ca.DBR_TIME_ENUM:
        req_type = ca.DBR_TIME_STRING

    def monitor_callback(epics_args, _):
        stamp = epics_args['pv_seconds'] + ca.POSIX_TIME_AT_EPICS_EPOCH + epics_args['pv_nseconds'] * 1e-9
        # time.strftime does not support microseconds
        strfmt = datetime.datetime.fromtimestamp(stamp).strftime('%Y-%m-%d %H:%M:%S.%f')
        value = epics_args['pv_value']

        if as_string and req_type == ca.DBR_CHAR:
            value = _ints_to_string(value)

        print('%s %s %s' % (name, strfmt, value))

    if callback is None:
        callback = monitor_callback

    chan.add_masked_array_event(req_type, count, None, callback)
    chan.flush_io()


def cainfo(name):
    """
    print pv information

    :param name: pv name
    """
    chan = _get_or_create_channel(name)

    r = chan.read_access()
    w = chan.write_access()
    if not r and not w:
        access = 'No access'
    else:
        access = ''
        if r:
            access += 'R'
        if w:
            access += 'W'

    message = \
        """
%s
    State:          %s
    Host:           %s
    Data type:      %s
    Element count:  %d
    Access:         %s""" % (
            name,
            ['Not connected', 'Connected'][chan.state() == ca.cs_conn],
            chan.host_name(),
            ca.dbf_text(chan.field_type()),
            chan.element_count(),
            access)

    if chan.state() == ca.cs_conn:
        ctrl = chan.getw(ca.dbf_type_to_DBR_CTRL(chan.field_type()))
        message += \
            """
    Status:         %s
    Severity:       %s""" % (ca.alarmStatusString(ctrl['pv_status']), ca.alarmSeverityString(ctrl['pv_severity']))

        if chan.field_type() == ca.DBF_ENUM:
            message += \
                """
    Enumerates:     %s""" % (ctrl['pv_statestrings'],)

    print(message)


if __name__ == '__main__':
    import doctest
    doctest.testmod()