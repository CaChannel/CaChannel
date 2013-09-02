Channel Access API
==================

Channel Access Basics
---------------------
EPICS channel access (CA) is the communication protocol used to transfer information between EPICS servers and clients.
Process variables (PV) are accessible though channel access. Interactions with EPICS PV include:

- Connect - create a connection between your application and a PV. This
  must be done before any other communication with the PV.
- Read - read data (and the meta info, limits, units, precision,
  statestrings) held in the PV.
- Write - write data to the PV.
- Monitor - notification when a PV's value or alarm state changes.
- Close - close the connection to the PV.

Data Type
^^^^^^^^^
Each PV has a native EPICS type. The natives type are then onverted to Python types.

=============   =============  ===============    ============
Native Type     Request Type   C Type             Python Type
=============   =============  ===============    ============
ca.DBF_INT      ca.DBR_INT     16bit short        Integer
ca.DBF_SHORT    ca.DBR_SHORT   16bit short        Integer
ca.DBF_LONG     ca.DBR_LONG    32bit int          Integer
ca.DBF_CHAR     ca.DBR_CHAR    8bit char          Integer
ca.DBF_STRING   ca.DBR_STRING  array of chars     String
                                (max 40)
ca.DBF_ENUM     ca.DBR_ENUM    16bit short        Integer
ca.DBF_FLOAT    ca.DBF_FLOAT   32bit float        Float
ca.DBF_DOUBLE   ca.DBF_DOUBLE  64bit double       Float
=============   =============  ===============    ============

This table also lists the EPICS request types.
Users can request that the type of the read or write value be changed internally by EPICS.
Typically this adds a time penalty and is not recommended.

The one area where type conversion is extremely useful is dealing with fields of type ca.DBF_ENUM.
An ENUM value can only be one from a predefined list.
A list consists of a set of string values that correspond to the ENUM values (similar to the C enum type).
It is easier to remember the list in terms of the strings instead of the numbers corresponding to each string.

Element counts
^^^^^^^^^^^^^^
Each data field can contain one or more data elements.
The number of data elements is referred to as the native element count for a field.
The number of data elements written to or read from a data field with multiple elements is user controllable.
All or some data elements can be read.
When some data elements are accessed the access is always started at the first element.
It is not possible to read part of the data and then read the rest of the data.

Module :mod:`ca`
----------------
This is a module to present the interface of low level channel access C library. 
As such, ``ca`` module interface can change without notice. It is discouraged to reply on it.

However these macros defined in *cadef.h* and *db_access.h* are guaranteed. 
``DBF_XXXX``, ``DBR_XXXX``, ``DBR_STS_XXXX``, ``DBR_TIME_XXXX``, ``DBR_GR_XXXX``, 
``DBR_CTRL_XXXX``, ``dbr_type_is_XXXX``, ``dbf_type_to_DBR_XXXX``.


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

