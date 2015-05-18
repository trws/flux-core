#ifndef _BROKER_OVERLAY_H
#define _BROKER_OVERLAY_H

typedef struct overlay_struct *overlay_t;
typedef void (*overlay_cb_f)(overlay_t ov, void *sock, void *arg);

overlay_t overlay_create (void);
void overlay_destroy (overlay_t ov);

/* These need to be set before connect/bind.
 */
void overlay_set_sec (overlay_t ov, flux_sec_t sec);
void overlay_set_zctx (overlay_t ov, zctx_t *zctx);
void overlay_set_rank (overlay_t ov, uint32_t rank);
void overlay_set_loop (overlay_t ov, zloop_t *zloop);
void overlay_set_heartbeat (overlay_t ov, heartbeat_t h);

/* All ranks but rank 0 connect to a parent to form the main TBON.
 * Internally there is a stack of parent URI's, with top as primary.
 * When we reparent (e.g. for failover), a new current parent is selected
 * and moved to the top.  Old parent sockets are not closed; they may
 * still trigger the parent callback, but only the primary is used for sends.
 *
 * The 'right' socket is an alternate topology (ring) used for rank-
 * addressed requests other than to self or rank 0.  It connects to
 * rank - 1, wrapped.
 */
void overlay_push_parent (overlay_t ov, const char *fmt, ...);
const char *overlay_get_parent (overlay_t ov);
void overlay_set_right (overlay_t ov, const char *fmt, ...);
void overlay_set_parent_cb (overlay_t ov, overlay_cb_f cb, void *arg);
int overlay_sendmsg_parent (overlay_t ov, zmsg_t **zmsg);
int overlay_sendmsg_right (overlay_t ov, zmsg_t **zmsg);
int overlay_keepalive_parent (overlay_t ov);

/* The child is where other ranks connect to send requests.
 * This is the ROUTER side of parent/right sockets described above.
 */
void overlay_set_child (overlay_t ov, const char *fmt, ...);
const char *overlay_get_child (overlay_t ov);
void overlay_set_child_cb (overlay_t ov, overlay_cb_f cb, void *arg);
int overlay_sendmsg_child (overlay_t ov, zmsg_t **zmsg);
/* We can "multicast" events to all child peers using mcast_child().
 * It walks the 'children' hash, finding overlay peers that have not
 * yet been "muted", and routes them a copy of zmsg.  The broker Cc's
 * events over the TBON using this until peers indicate that they are
 * receiving duplicate seq numbers through the normal event socket.
 */
int overlay_mcast_child (overlay_t ov, zmsg_t *zmsg);
void overlay_mute_child (overlay_t ov, const char *uuid);

/* Call when message is received from child 'uuid'.
 */
void overlay_checkin_child (overlay_t ov, const char *uuid);

/* Encode cmb.lspeer response payload.
 */
json_object *overlay_lspeer_encode (overlay_t ov);

/* The event socket is SUB for ranks > 0, and PUB for rank 0.
 * Internally, all events are routed to rank 0 before being published.
 */
void overlay_set_event (overlay_t ov, const char *fmt, ...);
const char *overlay_get_event (overlay_t ov);
void overlay_set_event_cb (overlay_t ov, overlay_cb_f cb, void *arg);
int overlay_sendmsg_event (overlay_t ov, zmsg_t *zmsg);
zmsg_t *overlay_recvmsg_event (overlay_t ov);

/* Since an epgm:// endpoint only allows one subscriber per node,
 * when there are multiple ranks per node, arrangements must be made
 * to forward events within a clique.  Only the relay itself has this
 * socket; other clique members would subscribe to the relay's URI
 * via their main event socket.  The PMI bootstrap sets this up if needed.
 */
void overlay_set_relay (overlay_t ov, const char *fmt, ...);
const char *overlay_get_relay (overlay_t ov);
int overlay_sendmsg_relay (overlay_t ov, zmsg_t *zmsg);

/* Establish connections.
 * These functions are idempotent as the bind may need to be called
 * early to resolve wildcard addresses (e.g. during PMI endpoint exchange).
 */
int overlay_bind (overlay_t ov);
int overlay_connect (overlay_t ov);

/* Switch parent DEALER socket to a new peer.  If the uri is already present
 * in the parent endpoint stack, reuse the existing socket ('recycled' set
 * to true).  The new parent is moved to the top of the parent stack.
 */
int overlay_reparent (overlay_t ov, const char *uri, bool *recycled);

#endif /* !_BROKER_OVERLAY_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
