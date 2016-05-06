#!/bin/env python
#
# filename: ca_cb.py
#
# Test CaChannel callbacks
#       search_and_connect
#       array_put_callback
#       array_get_callback
#       flush_io
#       pend_event
#
# Asynchronous execution does not require that the user wait for
# completion of an action.  Instead, a user specified callback
# function is executed when the action has completed.  Each callback
# takes two arguments:
#    + epics_args - arguments returned from epics.
#    + user_args - arguments specified by the user for use in the
#                  callback function.
# Since we don't need to wait for actions to complete we use
# flush_io() instead of pend_io() as in the synchronous examples.
# Flush_io()starts execution of actions and returns immediately.
# To allow the callback mechanism to function the user must call
# pend_event() periodically.  Pend_event() interrupts the programs
# main thread to allow completion of outstanding CA activity.
#

from CaChannel import *
import time
import sys

def connectCb(epics_args, user_args):
    print "connectCb: Python connect callback function"
    print type(epics_args)
    print epics_args
    print user_args
    
def putCb(epics_args, user_args):
    print "putCb: Python put callback function"
    print type(epics_args)
    print epics_args
    print ca.name(epics_args['chid'])
    print ca.dbr_text(epics_args['type'])
    print epics_args['count']
    print ca.message(epics_args['status'])
    print user_args

def getCb(epics_args, user_args):
    print "getCb: Python get callback function"
    print type(epics_args)
    print epics_args
    print "pvName = ", ca.name(epics_args['chid'])
    print "type = ", ca.dbr_text(epics_args['type'])
    print "count = ", epics_args['count']
    print "status = ", ca.message(epics_args['status'])
    print "user args = ", user_args
    print "value(s) = ", epics_args['pv_value']
#    print ca.alarmSeverityString(epics_args['pv_severity'])
#    print ca.alarmStatusString(epics_args['pv_status'])

def getCb1(epics_args, user_args):
    print "getCb: Python get callback function"
    print type(epics_args)
    print epics_args
    print "pvName = ", ca.name(epics_args['chid'])
    print "type = ", ca.dbr_text(epics_args['type'])
    print "count = ", epics_args['count']
    print "status = ", ca.message(epics_args['status'])
    print "user args = ", user_args
    print "value(s) = ", epics_args['pv_value']

def getCb2(epics_args, user_args):
    print "getCb: Python get callback function"
    print type(epics_args)
    print epics_args
    print "pvName = ", ca.name(epics_args['chid'])
    print "type = ", ca.dbr_text(epics_args['type'])
    print "count = ", epics_args['count']
    print "status = ", ca.message(epics_args['status'])
    print "user args = ", user_args
    print "value(s) = ", epics_args['pv_value']
    print ca.alarmSeverityString(epics_args['pv_severity'])
    print ca.alarmStatusString(epics_args['pv_status'])


def main():
    try:
        chan = CaChannel()
        print "search_and_connect"
        chan.search_and_connect('catest', connectCb)
        chan.flush_io()
        for i in range(20):
            chan.pend_event()
        print "put_callback"
        chan.array_put_callback(3.3, None, None, putCb)
        chan.flush_io()
        for i in range(20):
            chan.pend_event()
        print "get_callback"
        chan.array_get_callback(None, None, getCb1)
        chan.flush_io()
        for i in range(20):
            chan.pend_event()
        print "get_callback with status"
        chan.array_get_callback(ca.dbf_type_to_DBR_CTRL(chan.field_type()), None, getCb2)
        chan.flush_io()
        for i in range(20):
            chan.pend_event()
    except CaChannelException, status: 
        print ca.message(status)


    try:
        cawave = CaChannel()
        print "cawave: search_and_connect"
        cawave.search_and_connect('cawave', connectCb)
        cawave.flush_io()
        for i in range(20):
            cawave.pend_event()
        print "cawave: array_put_callback"
        l = [0,1,2,3,4,5,6,7,8,9]
        cawave.array_put_callback(l, None, None, putCb)
        cawave.flush_io()
        for i in range(20):
            cawave.pend_event()
        print "cawave: array_get_callback"
        cawave.array_get_callback(ca.dbf_type_to_DBR_CTRL(cawave.field_type()), None, getCb2)
        cawave.flush_io()
        for i in range(20):
            cawave.pend_event()
    except CaChannelException, status: 
        print ca.message(status)

main()
