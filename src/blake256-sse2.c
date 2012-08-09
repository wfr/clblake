// Blake-256, SSE2 assembler optimized
// adapted from the Supercop collection
//
// CAREFUL: unlike the reference implementation, the Supercop version measures
// buflen in bits, not bytes. I have changed this.

#include "blake.h"

#ifdef __SSE2__

#include <emmintrin.h>
#include <stddef.h>

#define U8TO32(p)         \
  (((uint32_t)((p)[0]) << 24) | ((uint32_t)((p)[1]) << 16) |  \
   ((uint32_t)((p)[2]) <<  8) | ((uint32_t)((p)[3])      ))
#define U32TO8(p, v)          \
  (p)[0] = (uint8_t)((v) >> 24); (p)[1] = (uint8_t)((v) >> 16); \
  (p)[2] = (uint8_t)((v) >>  8); (p)[3] = (uint8_t)((v)      ); 

#define LOADU(p)  _mm_loadu_si128( (__m128i *)(p) )
#define BSWAP32(r) do { \
   r = _mm_shufflehi_epi16(r, _MM_SHUFFLE(2, 3, 0, 1)); \
   r = _mm_shufflelo_epi16(r, _MM_SHUFFLE(2, 3, 0, 1)); \
   r = _mm_xor_si128(_mm_slli_epi16(r, 8), _mm_srli_epi16(r, 8)); \
} while(0)
  

static const uint8_t padding[] =
  {0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void blake256_sse2_init( state256 *S )
{
  S->h[0]=0x6A09E667;
  S->h[1]=0xBB67AE85;
  S->h[2]=0x3C6EF372;
  S->h[3]=0xA54FF53A;
  S->h[4]=0x510E527F;
  S->h[5]=0x9B05688C;
  S->h[6]=0x1F83D9AB;
  S->h[7]=0x5BE0CD19;
  S->t[0]=S->t[1]=S->buflen=S->nullt=0;
  S->s[0]=S->s[1]=S->s[2]=S->s[3] =0;
}



void blake256_sse2_compress( state256 * state, const uint8_t * datablock ) 
{
  __m128i row1,row2,row3,row4;
  __m128i buf1,buf2;
  
  #pragma GCC diagnostic ignored "-Wunused-variable"
  union {
    uint32_t     u32[16];
    __m128i u128[4];
  } m;
  int r;
  uint64_t t;

  static const int sig[][16] = {
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
    { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
    {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
    {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
    {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
    { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
    { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
    {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
    { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 } ,
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
    { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
    {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 }  
  };


  static const uint32_t z[16] = {
  0x243F6A88, 0x85A308D3, 0x13198A2E, 0x03707344,
  0xA4093822, 0x299F31D0, 0x082EFA98, 0xEC4E6C89,
  0x452821E6, 0x38D01377, 0xBE5466CF, 0x34E90C6C,
  0xC0AC29B7, 0xC97C50DD, 0x3F84D5B5, 0xB5470917 
  };
  #pragma GCC diagnostic push

  /* get message */
  m.u128[0] = LOADU(datablock +  0);
  m.u128[1] = LOADU(datablock + 16);
  m.u128[2] = LOADU(datablock + 32);
  m.u128[3] = LOADU(datablock + 48);
   
  BSWAP32(m.u128[0]);
  BSWAP32(m.u128[1]);
  BSWAP32(m.u128[2]);
  BSWAP32(m.u128[3]);

  row1 = _mm_set_epi32(state->h[ 3], state->h[ 2],
           state->h[ 1], state->h[ 0]);
  row2 = _mm_set_epi32(state->h[ 7], state->h[ 6],
           state->h[ 5], state->h[ 4]);
  row3 = _mm_set_epi32(0x03707344, 0x13198A2E, 0x85A308D3, 0x243F6A88);

  if (state->nullt) 
    row4 = _mm_set_epi32(0xEC4E6C89, 0x082EFA98, 0x299F31D0, 0xA4093822);
  else 
    row4 = _mm_set_epi32(0xEC4E6C89^state->t[1], 0x082EFA98^state->t[1],
       0x299F31D0^state->t[0], 0xA4093822^state->t[0]);

#define round(r) \
        /*        column step          */ \
        buf1 = _mm_set_epi32(m.u32[sig[r][ 6]], \
                            m.u32[sig[r][ 4]], \
                            m.u32[sig[r][ 2]], \
                            m.u32[sig[r][ 0]]); \
        buf2  = _mm_set_epi32(z[sig[r][ 7]], \
                            z[sig[r][ 5]], \
                            z[sig[r][ 3]], \
                            z[sig[r][ 1]]); \
        buf1 = _mm_xor_si128( buf1, buf2); \
        row1 = _mm_add_epi32( _mm_add_epi32( row1, buf1), row2 ); \
        buf1  = _mm_set_epi32(z[sig[r][ 6]], \
                            z[sig[r][ 4]], \
                            z[sig[r][ 2]], \
                            z[sig[r][ 0]]); \
        buf2 = _mm_set_epi32(m.u32[sig[r][ 7]], \
                            m.u32[sig[r][ 5]], \
                            m.u32[sig[r][ 3]], \
                            m.u32[sig[r][ 1]]); \
        row4 = _mm_xor_si128( row4, row1 ); \
        row4 = _mm_xor_si128(_mm_srli_epi32( row4, 16 ),_mm_slli_epi32( row4, 16 )); \
        row3 = _mm_add_epi32( row3, row4 );   \
        row2 = _mm_xor_si128( row2, row3 ); \
        buf1 = _mm_xor_si128( buf1, buf2); \
        row2 = _mm_xor_si128(_mm_srli_epi32( row2, 12 ),_mm_slli_epi32( row2, 20 )); \
        row1 = _mm_add_epi32( _mm_add_epi32( row1, buf1), row2 ); \
        row4 = _mm_xor_si128( row4, row1 ); \
        row4 = _mm_xor_si128(_mm_srli_epi32( row4,  8 ),_mm_slli_epi32( row4, 24 )); \
        row3 = _mm_add_epi32( row3, row4 );   \
        row4 = _mm_shuffle_epi32( row4, _MM_SHUFFLE(2,1,0,3) ); \
        row2 = _mm_xor_si128( row2, row3 ); \
        row2 = _mm_xor_si128(_mm_srli_epi32( row2,  7 ),_mm_slli_epi32( row2, 25 )); \
\
        row3 = _mm_shuffle_epi32( row3, _MM_SHUFFLE(1,0,3,2) ); \
        row2 = _mm_shuffle_epi32( row2, _MM_SHUFFLE(0,3,2,1) ); \
\
       /*       diagonal step         */ \
        buf1 = _mm_set_epi32(m.u32[sig[r][14]], \
                            m.u32[sig[r][12]], \
                            m.u32[sig[r][10]], \
                            m.u32[sig[r][ 8]]); \
        buf2  = _mm_set_epi32(z[sig[r][15]], \
                            z[sig[r][13]], \
                            z[sig[r][11]], \
                            z[sig[r][ 9]]); \
        buf1 = _mm_xor_si128( buf1, buf2); \
        row1 = _mm_add_epi32( _mm_add_epi32( row1, buf1 ), row2 ); \
        buf1  = _mm_set_epi32(z[sig[r][14]], \
                            z[sig[r][12]], \
                            z[sig[r][10]], \
                            z[sig[r][ 8]]); \
        buf2 = _mm_set_epi32(m.u32[sig[r][15]], \
                            m.u32[sig[r][13]], \
                            m.u32[sig[r][11]], \
                            m.u32[sig[r][ 9]]); \
        row4 = _mm_xor_si128( row4, row1 ); \
        buf1 = _mm_xor_si128( buf1, buf2); \
        row4 = _mm_xor_si128(_mm_srli_epi32( row4, 16 ),_mm_slli_epi32( row4, 16 )); \
        row3 = _mm_add_epi32( row3, row4 );   \
        row2 = _mm_xor_si128( row2, row3 ); \
        row2 = _mm_xor_si128(_mm_srli_epi32( row2, 12 ),_mm_slli_epi32( row2, 20 )); \
        row1 = _mm_add_epi32( _mm_add_epi32( row1, buf1 ), row2 ); \
        row4 = _mm_xor_si128( row4, row1 ); \
        row4 = _mm_xor_si128(_mm_srli_epi32( row4,  8 ),_mm_slli_epi32( row4, 24 )); \
        row3 = _mm_add_epi32( row3, row4 );   \
        row4 = _mm_shuffle_epi32( row4, _MM_SHUFFLE(0,3,2,1) ); \
        row2 = _mm_xor_si128( row2, row3 ); \
        row2 = _mm_xor_si128(_mm_srli_epi32( row2,  7 ),_mm_slli_epi32( row2, 25 )); \
\
        row3 = _mm_shuffle_epi32( row3, _MM_SHUFFLE(1,0,3,2) ); \
        row2 = _mm_shuffle_epi32( row2, _MM_SHUFFLE(2,1,0,3) ); \
\

  round( 0);
  round( 1);
  round( 2);
  round( 3);
  round( 4);
  round( 5);
  round( 6);
  round( 7);
  round( 8);
  round( 9);
  round(10);
  round(11);
  round(12);
  round(13);

  _mm_store_si128( (__m128i *)m.u32, _mm_xor_si128(row1,row3));
  state->h[0] ^= m.u32[ 0]; 
  state->h[1] ^= m.u32[ 1];    
  state->h[2] ^= m.u32[ 2];    
  state->h[3] ^= m.u32[ 3];
  _mm_store_si128( (__m128i *)m.u32, _mm_xor_si128(row2,row4));
  state->h[4] ^= m.u32[ 0];    
  state->h[5] ^= m.u32[ 1];    
  state->h[6] ^= m.u32[ 2];    
  state->h[7] ^= m.u32[ 3];
}

#if 0
void blake256_sse2_update( state256 *S, const uint8_t *data, uint64_t datalen )  // len in bits!
{

  int left=S->buflen >> 3; 
  int fill=64 - left;
   
  if( left && ( ((datalen >> 3) & 0x3F) >= fill ) ) {
    memcpy( (void*) (S->buf + left), (void*) data, fill );
    S->t[0] += 512;
    if (S->t[0] == 0) S->t[1]++;      
    blake256_sse2_compress( S, S->buf );
    data += fill;
    datalen  -= (fill << 3);       
    left = 0;
  }

  while( datalen >= 512 ) {
    S->t[0] += 512;
    if (S->t[0] == 0) S->t[1]++;
    blake256_sse2_compress( S, data );
    data += 64;
    datalen  -= 512;
  }
  
  if( datalen > 0 ) {
    memcpy( (void*) (S->buf + left), (void*) data, datalen>>3 );
    S->buflen = (left<<3) + datalen;
  }
  else S->buflen=0;
}

void blake256_sse2_final( state256 *S, uint8_t *digest ) 
{
  uint8_t msglen[8], zo=0x01, oo=0x81;
  uint32_t lo=S->t[0] + S->buflen, hi=S->t[1];
  if ( lo < S->buflen ) hi++;
  U32TO8(  msglen + 0, hi );
  U32TO8(  msglen + 4, lo );

  if ( S->buflen == 440 ) { /* one padding byte */
    S->t[0] -= 8;
    blake256_sse2_update( S, &oo, 8 );
  }
  else {
    if ( S->buflen < 440 ) { /* enough space to fill the block  */
      if ( !S->buflen ) S->nullt=1;
      S->t[0] -= 440 - S->buflen;
      blake256_sse2_update( S, padding, 440 - S->buflen );
    }
    else { /* need 2 compressions */
      S->t[0] -= 512 - S->buflen;
      blake256_sse2_update( S, padding, 512 - S->buflen );
      S->t[0] -= 440;
      blake256_sse2_update( S, padding+1, 440 );
      S->nullt = 1;
    }
    blake256_sse2_update( S, &zo, 8 );
    S->t[0] -= 8;
  }
  S->t[0] -= 64;
  blake256_sse2_update( S, msglen, 64 );

  U32TO8( digest + 0, S->h[0]);
  U32TO8( digest + 4, S->h[1]);
  U32TO8( digest + 8, S->h[2]);
  U32TO8( digest +12, S->h[3]);
  U32TO8( digest +16, S->h[4]);
  U32TO8( digest +20, S->h[5]);
  U32TO8( digest +24, S->h[6]);
  U32TO8( digest +28, S->h[7]);
}
#endif


// changed "buflen" from bit to byte
void blake256_sse2_update( state256 *S, const uint8_t *in, uint64_t inlen )
{
  int left = S->buflen;
  int fill = 64 - left;

  /* data left and data received fill a block  */
  if( left && ( inlen >= fill ) )
  {
    memcpy( ( void * ) ( S->buf + left ), ( void * ) in, fill );
    S->t[0] += 512;

    if ( S->t[0] == 0 ) S->t[1]++;

    blake256_sse2_compress( S, S->buf );
    in += fill;
    inlen  -= fill;
    left = 0;
  }

  /* compress blocks of data received */
  while( inlen >= 64 )
  {
    S->t[0] += 512;

    if ( S->t[0] == 0 ) S->t[1]++;

    blake256_sse2_compress( S, in );
    in += 64;
    inlen -= 64;
  }

  /* store any data left */
  if( inlen > 0 )
  {
    memcpy( ( void * ) ( S->buf + left ), ( void * ) in, ( size_t ) inlen );
    S->buflen = left + ( int )inlen;
  }
  else S->buflen = 0;
}


void blake256_sse2_final( state256 *S, uint8_t *out )
{
  uint8_t msglen[8], zo = 0x01, oo = 0x81;
  uint32_t lo = S->t[0] + ( S->buflen << 3 ), hi = S->t[1];

  /* support for hashing more than 2^32 bits */
  if ( lo < ( S->buflen << 3 ) ) hi++;

  U32TO8_BIG(  msglen + 0, hi );
  U32TO8_BIG(  msglen + 4, lo );

  if ( S->buflen == 55 )   /* one padding byte */
  {
    S->t[0] -= 8;
    blake256_ref_update( S, &oo, 1 );
  }
  else
  {
    if ( S->buflen < 55 )   /* enough space to fill the block  */
    {
      if ( !S->buflen ) S->nullt = 1;

      S->t[0] -= 440 - ( S->buflen << 3 );
      blake256_sse2_update( S, padding, 55 - S->buflen );
    }
    else   /* need 2 compressions */
    {
      S->t[0] -= 512 - ( S->buflen << 3 );
      blake256_sse2_update( S, padding, 64 - S->buflen );
      S->t[0] -= 440;
      blake256_sse2_update( S, padding + 1, 55 );
      S->nullt = 1;
    }

    blake256_sse2_update( S, &zo, 1 );
    S->t[0] -= 8;
  }

  S->t[0] -= 64;
  blake256_sse2_update( S, msglen, 8 );
  U32TO8_BIG( out + 0, S->h[0] );
  U32TO8_BIG( out + 4, S->h[1] );
  U32TO8_BIG( out + 8, S->h[2] );
  U32TO8_BIG( out + 12, S->h[3] );
  U32TO8_BIG( out + 16, S->h[4] );
  U32TO8_BIG( out + 20, S->h[5] );
  U32TO8_BIG( out + 24, S->h[6] );
  U32TO8_BIG( out + 28, S->h[7] );
}



void blake256_sse2_hash( uint8_t *out, const uint8_t *in, uint64_t inlen )
{
  state256 S;  
  blake256_sse2_init( &S );
  blake256_sse2_update( &S, in, inlen );
  blake256_sse2_final( &S, out );
}


#endif // __SSE2__

// vim:set sw=2 ts=2 sts=2 expandtab:
