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
    chan = CaChannel('myPV')
    chan.searhw()
    print chan.getw(ca.dbf_type_to_DBR_CTRL(chan.field_type()))

Get Wavefrom as Numpy Array
---------------------------
- At function level 

  ::
    
    from CaChannel import ca, CaChannel
    chan = CaChanne('myWaveformPV')
    print chan.getw(use_numpy=True)

- At module level

  ::

    import CaChannel
    CaChannel.USE_NUMPY = True
    chan = CaChannel.CaChanne('myWaveformPV')
    print chan.getw()
