Channel Access API
==================

EPICS channel access (CA) is the communication protocol used to transfer information between EPICS servers and clients.
Process variables (PV) are accessible though channel access. Interactions with EPICS PV include:

- Connect - create a connection between your application and a PV. This
  must be done before any other communication with the PV.
- Read - read data (and the meta info, limits, units, precision,
  statestrings) held in the PV.
- Write - write data to the PV.
- Monitor - notification when a PV's value or alarm state changes.
- Close - close the connection to the PV.


:mod:`ca` --- Low level interface
---------------------------------

.. module:: ca
   :synopsis: Low level operation to channel access C library

This is a module to present the interface of low level channel access C library.
It has the same API as module :mod:`caffi.ca`.

Data Types
^^^^^^^^^^

Each PV has a native EPICS type. The native types are then converted to Python types.

This table also lists the EPICS request types.
Users can request that the type of the read or write value be changed internally by EPICS.
Typically this adds a time penalty and is not recommended.

========================   ========================  ===============    ============
Native Type                Request Type              C Type             Python Type
========================   ========================  ===============    ============
.. data:: ca.DBF_INT       .. data:: ca.DBR_INT      16bit short        Integer
.. data:: ca.DBF_SHORT     .. data:: ca.DBR_SHORT    16bit short        Integer
.. data:: ca.DBF_LONG      .. data:: ca.DBR_LONG     32bit int          Integer
.. data:: ca.DBF_CHAR      .. data:: ca.DBR_CHAR     8bit char          Integer
.. data:: ca.DBF_STRING    .. data:: ca.DBR_STRING   array of chars     String
                                                     (max 40)
.. data:: ca.DBF_ENUM      .. data:: ca.DBR_ENUM     16bit short        Integer
.. data:: ca.DBF_FLOAT     .. data:: ca.DBR_FLOAT    32bit float        Float
.. data:: ca.DBF_DOUBLE    .. data:: ca.DBR_DOUBLE   64bit double       Float
========================   ========================  ===============    ============

The one area where type conversion is extremely useful is dealing with fields of type :data:`ca.DBF_ENUM`.
An ENUM value can only be one from a predefined list.
A list consists of a set of string values that correspond to the ENUM values (similar to the C enum type).
It is easier to remember the list in terms of the strings instead of the numbers corresponding to each string.

Error Code
^^^^^^^^^^
Error codes defined in header *caerr.h* are supported.

Element Count
^^^^^^^^^^^^^
Each data field can contain one or more data elements.
The number of data elements is referred to as the native element count for a field.
The number of data elements written to or read from a data field with multiple elements is user controllable.
All or some data elements can be read.
When some data elements are accessed the access is always started at the first element.
It is not possible to read part of the data and then read the rest of the data.



:mod:`CaChannel`
----------------

.. module:: CaChannel
   :synopsis: High level interface to EPICS channel access.

:mod:`CaChannel` module is a (relatively) high level interface to operate on channel access.
It provides almost one to one function map to the channel access C API.
So basic knowledge of channel access is assumed. 

But it does make it pythonic in other ways, single :class:`CaChannel` object, flexible parameter input
and value return.

.. data:: USE_NUMPY

    If numpy support is enabled at compiling time and numpy package is available at runtime,
    numeric data types can be returned as numpy arrays when `USE_NUMPY=True`. 
    This boosts performance on large size arrays (>1M elements).


Exception :class:`CaChannelException`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. exception:: CaChannelException

    This is the exception type throwed by any channel access operations. 
    Its string representation shows the descriptive message.

Class :class:`CaChannel`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. autoclass:: CaChannel

Connect
~~~~~~~
    .. automethod:: CaChannel.search
    .. automethod:: CaChannel.search_and_connect
    .. automethod:: CaChannel.searchw
    .. automethod:: CaChannel.clear_channel
    .. automethod:: CaChannel.change_connection_event

Read
~~~~
    .. automethod:: CaChannel.array_get
    .. automethod:: CaChannel.getValue
    .. automethod:: CaChannel.array_get_callback
    .. automethod:: CaChannel.getw

Write
~~~~~
    .. automethod:: CaChannel.array_put
    .. automethod:: CaChannel.array_put_callback
    .. automethod:: CaChannel.putw

Monitor
~~~~~~~
    .. automethod:: CaChannel.add_masked_array_event
    .. automethod:: CaChannel.clear_event

Execute
~~~~~~~
    .. automethod:: CaChannel.pend_io
    .. automethod:: CaChannel.pend_event
    .. automethod:: CaChannel.poll
    .. automethod:: CaChannel.flush_io

Information
~~~~~~~~~~~
    .. automethod:: CaChannel.field_type
    .. automethod:: CaChannel.element_count
    .. automethod:: CaChannel.name
    .. automethod:: CaChannel.state
    .. automethod:: CaChannel.host_name
    .. automethod:: CaChannel.read_access
    .. automethod:: CaChannel.write_access

Misc
~~~~
    .. automethod:: CaChannel.setTimeout
    .. automethod:: CaChannel.getTimeout
    .. automethod:: CaChannel.replace_access_rights_event

:mod:`epicsPV`
--------------

.. automodule:: epicsPV

Class :class:`epicsPV`
^^^^^^^^^^^^^^^^^^^^^^

.. autoclass:: epicsPV

Constructor
~~~~~~~~~~~
    .. automethod:: epicsPV.__init__

Read
~~~~
    .. automethod:: epicsPV.array_get
    .. automethod:: epicsPV.getValue
    .. automethod:: epicsPV.getw
    .. automethod:: epicsPV.getControl

Write
~~~~~
    .. automethod:: epicsPV.putWait

Monitor
~~~~~~~
    .. automethod:: epicsPV.setMonitor
    .. automethod:: epicsPV.clearMonitor
    .. automethod:: epicsPV.checkMonitor

:mod:`epicsMotor`
-----------------

.. automodule:: epicsMotor

Exception :class:`epicsMotorException`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. exception:: epicsMotorException

    This exception is raised when :meth:`epicsMotor.check_limits` method detects a soft limit
    or hard limit violation. The :meth:`epicsMotor.move` and :meth:`epicsMotor.wait` methods call
    :meth:`epicsMotor.check_limits` before they return, unless they are called with the
    ``ignore_limits=True`` keyword set.

Class :class:`epicsMotor`
^^^^^^^^^^^^^^^^^^^^^^^^^

.. autoclass:: epicsMotor

Constructor
~~~~~~~~~~~
    .. automethod:: epicsMotor.__init__

Move
~~~~
    .. automethod:: epicsMotor.move
    .. automethod:: epicsMotor.stop
    .. automethod:: epicsMotor.wait

Readback
~~~~~~~~
    .. automethod:: epicsMotor.get_position
    .. automethod:: epicsMotor.check_limits

Config
~~~~~~
    .. automethod:: epicsMotor.set_position


:mod:`CaChannel.util`
---------------------

.. automodule:: CaChannel.util

.. autofunction:: caget
.. autofunction:: caput
.. autofunction:: camonitor
.. autofunction:: cainfo
