import os
import flux
import flux_internal as fi
import sys
import ctypes as ct

def pecho_impl(h, typemask, messages, arg):
    h.log(2, "in impl, args:{}".format((typemask, messages, arg)))
    req = ct.c_char_p
    h.flux_event_decode(messages.contents, None, ct.POINTER(req))
    print req
    return 0

pecho_callback = flux.MsgHandler(pecho_impl)

def mod_main(h, arg_dict):
    print "in mod main"
    h.log(2, "it made it!")
    print "passed log"
    if h.event_subscribe("ptest.pecho") < 0:
        print "event subscription failed:"
        h.err()
    ret = 0
    ret = h.msghandler_add(flux.msgtype.EVENT, "ptest.pecho", pecho_callback, None)
    print 'msghandler:', ret
    if ret < 0:
        h.err()
    if h.flux_reactor_start() < 0:
        print "reactor start failed!:"
        h.err()
    h.log(4, "pecho unloading")

