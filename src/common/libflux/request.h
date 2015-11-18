#ifndef _FLUX_CORE_REQUEST_H
#define _FLUX_CORE_REQUEST_H

#include <stdbool.h>
#include <stdarg.h>

#include "message.h"

/* Decode a request message with optional json payload.
 * If topic is non-NULL, assign the request topic string.
 * If json_str is non-NULL, assign the payload.  This argument indicates whether
 * payload is expected and it is an EPROTO error if expectations are not met.
 * Returns 0 on success, or -1 on failure with errno set.
 */
int flux_request_decode (const flux_msg_t *msg, const char **topic,
                         const char **json_str);

/* Decode a request message with optional raw payload.
 * If topic is non-NULL, assign the request topic string.
 * If data is non-NULL, assign the payload.  This argument indicates whether
 * payload is expected and it is an EPROTO error if expectations are not met.
 * Returns 0 on success, or -1 on failure with errno set.
 */
int flux_request_decode_raw (const flux_msg_t *msg, const char **topic,
                             void *data, int *len);

/* Encode a request message.
 * If json_str is non-NULL, assign the json payload.
 */
flux_msg_t *flux_request_encode (const char *topic, const char *json_str);

/* Encode a request message.
 * If data is non-NULL, assign the raw payload.
 */
flux_msg_t *flux_request_encode_raw (const char *topic,
                                     const void *data, int len);

#endif /* !_FLUX_CORE_REQUEST_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
