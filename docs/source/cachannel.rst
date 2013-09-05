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
As such, :mod:`ca` module interface can change without notice. It is discouraged to reply on it.
However these macros defined in *cadef.h* and *db_access.h* are guaranteed. 

Data Types
^^^^^^^^^^

Each PV has a native EPICS type. The natives type are then onverted to Python types.

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

The one area where type conversion is extremely useful is dealing with fields of type ca.DBF_ENUM.
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



Class :class:`CaChannel`
------------------------

.. autoclass:: CaChannel.CaChannel

Connect
^^^^^^^
    .. automethod:: CaChannel.CaChannel.search
    .. automethod:: CaChannel.CaChannel.search_and_connect
    .. automethod:: CaChannel.CaChannel.searchw
    .. automethod:: CaChannel.CaChannel.clear_channel()

Read
^^^^
    .. automethod:: CaChannel.CaChannel.array_get
    .. automethod:: CaChannel.CaChannel.getValue
    .. automethod:: CaChannel.CaChannel.array_get_callback
    .. automethod:: CaChannel.CaChannel.getw

Write
^^^^^
    .. automethod:: CaChannel.CaChannel.array_put
    .. automethod:: CaChannel.CaChannel.array_put_callback
    .. automethod:: CaChannel.CaChannel.putw

Monitor
^^^^^^^
    .. automethod:: CaChannel.CaChannel.add_masked_array_event
    .. automethod:: CaChannel.CaChannel.clear_event

Execute
^^^^^^^
    .. automethod:: CaChannel.CaChannel.pend_io
    .. automethod:: CaChannel.CaChannel.pend_event
    .. automethod:: CaChannel.CaChannel.poll
    .. automethod:: CaChannel.CaChannel.flush_io

Information
^^^^^^^^^^^
    .. automethod:: CaChannel.CaChannel.field_type
    .. automethod:: CaChannel.CaChannel.element_count
    .. automethod:: CaChannel.CaChannel.name
    .. automethod:: CaChannel.CaChannel.state
    .. automethod:: CaChannel.CaChannel.host_name
    .. automethod:: CaChannel.CaChannel.read_access
    .. automethod:: CaChannel.CaChannel.write_access

