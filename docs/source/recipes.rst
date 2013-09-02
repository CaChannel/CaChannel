Recipes
=======

Get String of Enum
------------------

::

    from CaChannel import ca, CaChannel
    chan = CaChannel('myEnumPV')
    chan.searhw()
    print chan.getw(ca.DBR_STRING)

Get Control Information 
-----------------------

::

    from CaChannel import ca, CaChannel
    chan = CaChannel.CaChannel('myPV')
    chan.searhw()
    print chan.getw(ca.dbf_type_to_DBR_CTRL(chan.field_type()))

