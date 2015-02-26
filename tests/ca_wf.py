#! /bin/env python
#
# filename: ca_io.py
#
# Test CaChannel io using ca.pend_io()
#	search
#	array_put
#	array_get
#	pend_io
#
# Each group of actions (search, put, get) is executed by the call
# to pend_io().  When the actions have completed pend_io() returns
# and the code continues.  Each call to pend_io() applies to all the
# channel objects and not just the object whose method is being invoked.
# A timeout value (specified in seconds) is associated with a call to
# pend_io() if no timeout is given the default value of one second is used.
#
# Values returned from a get are stored for retrieval after pend_io()
# has returned.  The returned values are accessed using the getValue()
# method.  If pend_io() returns an error status then all values read
# during processing are not guaranteed to be correct.
#

from CaChannel import *

def getCallback(epicsArgs, userArgs):
    data=epicsArgs['pv_value']
    print 'In callback', data

def main():
    try:
	cawave = CaChannel()

	cawave.searchw('cawave')
	
	t = (0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19)
        l = []
        for i in range(0,500):
            l.append(i)
	#cawave.array_put(t)
	#cawave.pend_io()
	
        cawave.putw(l)
	print cawave.getw()
	
        cawave.add_masked_array_event(None, None, ca.DBE_VALUE, getCallback, 0)
        cawave.pend_event()
        #cawave.clear_event()

    except CaChannelException, status:
	print ca.message(status)
    
main()
