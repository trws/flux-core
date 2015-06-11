import os
import flux
import flux_internal as fi
import sys
import ctypes as ct

def pecho_impl(h, typemask, messages, arg):
    h.log(2, "in impl, args:{}".format((typemask, messages, arg)))
    req = ct.c_char_p()
    h.flux_event_decode(messages.contents, None, ct.byref(req))
    print ct.string_at(req)
    # set the return type to non-int
    h.json_tokener_parse.restype = ct.c_void_p
    request = h.json_tokener_parse(req)
    h.mrpc_create_fromevent.restype = ct.c_void_p
    rpc = h.mrpc_create_fromevent(request)
    inarg = ct.c_void_p()
    h.flux_mrpc_get_inarg(rpc, ct.byref(inarg))
    h.flux_mrpc_put_outarg(rpc, inarg)
    h.flux_mrpc_respond(rpc)
    h.json_object_put(request)
    h.json_object_put(inarg)
    h.flux_mrpc_destroy(rpc)
    h.zmsg_destroy(messages)
    return 0

pecho_callback = flux.MsgHandler(pecho_impl)

def mod_main(h, arg_dict):
    print "in mod main"
    h.log(2, "it made it!")
    print "passed log"
    if h.event_subscribe("mrpc.mecho") < 0:
        print "event subscription failed:"
        h.err()
    ret = 0
    ret = h.msghandler_add(flux.msgtype.EVENT, "mrpc.mecho", pecho_callback, None)
    print 'msghandler:', ret
    if ret < 0:
        h.err()
    if h.reactor_start() < 0:
        print "reactor start failed!:"
        h.err()
    h.log(4, "pecho unloading")

