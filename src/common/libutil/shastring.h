#include <stdint.h>

int sha1_strtohash (const char *s, uint8_t hash[20]);
void sha1_hashtostr (const uint8_t hash[20], char s[41]);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
