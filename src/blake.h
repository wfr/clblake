#pragma once

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define U8TO32_BIG(p)         \
  (((uint32_t)((p)[0]) << 24) | ((uint32_t)((p)[1]) << 16) |  \
   ((uint32_t)((p)[2]) <<  8) | ((uint32_t)((p)[3])      ))

#define U32TO8_BIG(p, v)          \
  (p)[0] = (uint8_t)((v) >> 24); (p)[1] = (uint8_t)((v) >> 16); \
(p)[2] = (uint8_t)((v) >>  8); (p)[3] = (uint8_t)((v)      );

#define U8TO64_BIG(p) \
  (((uint64_t)U8TO32_BIG(p) << 32) | (uint64_t)U8TO32_BIG((p) + 4))

#define U64TO8_BIG(p, v) \
  U32TO8_BIG((p),     (uint32_t)((v) >> 32)); \
U32TO8_BIG((p) + 4, (uint32_t)((v)      ));

typedef struct
{
  uint32_t h[8], s[4], t[2];
  int buflen, nullt;
  uint8_t  buf[64];
} state256;

typedef state256 state224;

typedef struct
{
  uint64_t h[8], s[4], t[2];
  int buflen, nullt;
  uint8_t buf[128];
} state512;

typedef state512 state384;


// Reference implementation
void blake256_ref_init( state256 *S );
void blake256_ref_compress( state256 *S, const uint8_t *block );
void blake256_ref_update( state256 *S, const uint8_t *in, uint64_t inlen );
void blake256_ref_final( state256 *S, uint8_t *out );
void blake256_ref_hash( uint8_t *out, const uint8_t *in, uint64_t inlen );


#ifdef __SSSE3__
	void blake256_ssse3_init( state256 *S );
	void blake256_ssse3_compress( state256 *state, const uint8_t *block );
	void blake256_ssse3_update( state256 *S, const uint8_t *data, uint64_t inlen );
	void blake256_ssse3_final( state256 *S, uint8_t *digest );
	void blake256_ssse3_hash( uint8_t *out, const uint8_t *in, uint64_t inlen );
	#define BLAKE256_CPU_IMPL "SSSE3"
	#define blake256_init     blake256_ssse3_init
	#define blake256_compress blake256_ssse3_compress
	#define blake256_update   blake256_ssse3_update
	#define blake256_final    blake256_ssse3_final
	#define blake256_hash     blake256_ssse3_hash
#else
	#ifdef __SSE2__
		void blake256_sse2_init( state256 *S );
		void blake256_sse2_compress( state256 *state, const uint8_t *block );
		void blake256_sse2_update( state256 *S, const uint8_t *data, uint64_t inlen );
		void blake256_sse2_final( state256 *S, uint8_t *digest );
		void blake256_sse2_hash( uint8_t *out, const uint8_t *in, uint64_t inlen );
		#define BLAKE256_CPU_IMPL "SSE2"
		#define blake256_init     blake256_sse2_init
		#define blake256_compress blake256_sse2_compress
		#define blake256_update   blake256_sse2_update
		#define blake256_final    blake256_sse2_final
		#define blake256_hash     blake256_sse2_hash
	#else
		#define BLAKE256_CPU_IMPL "C"
		#define blake256_init     blake256_ref_init
		#define blake256_compress blake256_ref_compress
		#define blake256_update   blake256_ref_update
		#define blake256_final    blake256_ref_final
		#define blake256_hash     blake256_ref_hash
	#endif
#endif
