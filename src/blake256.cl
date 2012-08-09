/* Blake hash algorithm
 *
 * Naive port of the reference implementation to OpenCL
 * Restrictions:
 * - block size must be a multiple of 64 bytes
 * - block size must be < 2^32 bytes
 *
 * Problems:
 * - not endian-safe. Only tested on Little Endian.
 */

#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable 
#pragma OPENCL EXTENSION cl_nv_pragma_unroll


typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long  uint64_t;
typedef          char   int8_t;
typedef          short  int16_t;
typedef          int    int32_t;
typedef          long   int64_t;

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
  uint32_t h[8];
  uint32_t t;
} state256;


#define NB_ROUNDS32 14

constant uint8_t sigma[][16] =
{
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
  {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
  {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
  { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
  { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
  { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
  {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
  {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
  { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
  {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13 , 0 },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
  {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
  {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
  { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
  { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
  { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 }
};

constant uint32_t u256[16] =
{
  0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344,
  0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89,
  0x452821e6, 0x38d01377, 0xbe5466cf, 0x34e90c6c,
  0xc0ac29b7, 0xc97c50dd, 0x3f84d5b5, 0xb5470917
};



//// 256-bit
// #undef ROT
//#define ROT(x,n) (((x)<<(32-n))|( (x)>>(n)))
//#define G(a,b,c,d,e)          \
//  v[a] += (m[sigma[i][e]] ^ u256[sigma[i][e+1]]) + v[b]; \
//  v[d] = ROT( v[d] ^ v[a],16);        \
//  v[c] += v[d];           \
//  v[b] = ROT( v[b] ^ v[c],12);        \
//  v[a] += (m[sigma[i][e+1]] ^ u256[sigma[i][e]])+v[b]; \
//  v[d] = ROT( v[d] ^ v[a], 8);        \
//  v[c] += v[d];           \
//  v[b] = ROT( v[b] ^ v[c], 7);
//#define ROT32(x,n) (((x)<<(32-n))|( (x)>>(n)))
#define ROT32(x,n)   (rotate((uint)x, (uint)32-n))
#define ADD32(x,y)   ((uint)((x) + (y)))
#define XOR32(x,y)   ((uint)((x) ^ (y)))

#define G(a,b,c,d,i) \
  do {\
    v[a] = XOR32(m[sigma[r][i]], u256[sigma[r][i+1]])+ADD32(v[a],v[b]);\
    v[d] = ROT32(XOR32(v[d],v[a]),16);\
    v[c] = ADD32(v[c],v[d]);\
    v[b] = ROT32(XOR32(v[b],v[c]),12);\
    v[a] = XOR32(m[sigma[r][i+1]], u256[sigma[r][i]])+ADD32(v[a],v[b]); \
    v[d] = ROT32(XOR32(v[d],v[a]), 8);\
    v[c] = ADD32(v[c],v[d]);\
    v[b] = ROT32(XOR32(v[b],v[c]), 7);\
  } while (0)

// compress a block
// > if block == 0: finalize
void blake256_compress_block( private state256 *S, global const uint8_t *block )
{
  uint32_t v[16], m[16], i;

  if(block != 0)
  {
    //#pragma unroll 16
    for( i = 0; i < 16; ++i )  m[i] = U8TO32_BIG( block + i * 4 );
  }
  else
  {
    // Final 64-byte block:
    //  0  { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //  8    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 16    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 24    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 32    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 40    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 48    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    // 56    0x00, 0x00, 0x00, 0x00,  LEN,  LEN,  LEN,  LEN }
    /*uint32_t m[16] = {
      0x80000000, 0, 0, 0,  0, 0, 0, 0,
               0, 0, 0, 0,  0, 1, 0, S->t 
    };*/
    m[0] = 0x80000000;
    m[1] = m[2] = m[3] = m[4] = m[5] = m[6] = m[7] = 0;
    m[8] = m[9] = m[10] = m[11] = m[12] = 0; m[13] = 1; m[14] = 0;
    m[15] = S->t;
  }

  //#pragma unroll 8
  for( i = 0; i < 8; ++i )  v[i] = S->h[i];

  v[ 8] = u256[0];
  v[ 9] = u256[1];
  v[10] = u256[2];
  v[11] = u256[3];
  v[12] = u256[4];
  v[13] = u256[5];
  v[14] = u256[6];
  v[15] = u256[7];
 
  if(block != 0)
  {
    /* don't xor t when the block is only padding */
    v[12] ^= S->t;
    v[13] ^= S->t;
  }

  // #pragma unroll 2
  for(int r = 0; r < NB_ROUNDS32; ++r )
  {
    /* column step */
    G( 0,  4,  8, 12,  0 );
    G( 1,  5,  9, 13,  2 );
    G( 2,  6, 10, 14,  4 );
    G( 3,  7, 11, 15,  6 );
    /* diagonal step */
    G( 0,  5, 10, 15,  8 );
    G( 1,  6, 11, 12, 10 );
    G( 2,  7,  8, 13, 12 );
    G( 3,  4,  9, 14, 14 );
  }

  //#pragma unroll
  for( i = 0; i < 16; ++i )  S->h[i % 8] ^= v[i];
}


void blake256_init( private state256 *S )
{
  S->h[0] = 0x6a09e667;
  S->h[1] = 0xbb67ae85;
  S->h[2] = 0x3c6ef372;
  S->h[3] = 0xa54ff53a;
  S->h[4] = 0x510e527f;
  S->h[5] = 0x9b05688c;
  S->h[6] = 0x1f83d9ab;
  S->h[7] = 0x5be0cd19;
  S->t = 0;
}


void blake256_update( private state256 *S, global const uint8_t *in, uint32_t inlen )
{
  /* compress blocks of data received */
  while( inlen >= 64 )
  {
    S->t += 512;
    blake256_compress_block( S, in );
    in += 64;
    inlen -= 64;
  }
}


void blake256_final( private state256 *S, global uint8_t *out)
{
  blake256_compress_block( S, 0 );
  U32TO8_BIG( out + 0, S->h[0] );
  U32TO8_BIG( out + 4, S->h[1] );
  U32TO8_BIG( out + 8, S->h[2] );
  U32TO8_BIG( out + 12, S->h[3] );
  U32TO8_BIG( out + 16, S->h[4] );
  U32TO8_BIG( out + 20, S->h[5] );
  U32TO8_BIG( out + 24, S->h[6] );
  U32TO8_BIG( out + 28, S->h[7] );
}


kernel void blake256_hash_block( global uint8_t *out, global const uint8_t *in, const uint32_t chunk_size)
{
  const int gx = get_global_id(0);
  const int lx = get_local_id(0);

  global uint8_t* item_in  = &( in[gx * chunk_size]);
  global uint8_t* item_out = &(out[gx * 32]);
  private state256 S;
  
  blake256_init( &S );
  blake256_update( &S, item_in, chunk_size );
  blake256_final( &S, item_out );
}

// vim:set ft=c ts=2 sw=2 expandtab:
