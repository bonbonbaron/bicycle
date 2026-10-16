#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Inflates a zlib stream (RFC 1950) into dst, which must be exactly the inflated size. Returns 1 on success
 * (adler32 checked), 0 otherwise. See src/data/tinfl.c. */
int bicycleInflate( const unsigned char* src, size_t srcLen, unsigned char* dst, size_t dstLen );
#ifdef __cplusplus
}
#endif
