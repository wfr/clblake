#include "BlakeTreeCPU.h"

#include "blake.h"
#include "log.h"

void blakeTreeCPU(uint8_t* in, size_t length, uint8_t* out, size_t* out_size)
{
	size_t remainder  = length % BT_LEAF_SIZE;
	size_t global     = length / BT_LEAF_SIZE;

	size_t gx; 

	uint8_t *in_p, *out_p;
	
	/*
	loggerf(DEBUG, "blakeTreeCPU: global: %d items  remainder: %d byte", 
		global, remainder);
	*/

	#pragma omp parallel for
	for(gx=0; gx < global; gx++) {
		in_p  = &( in[gx * BT_LEAF_SIZE]);
		out_p = &(out[gx * HASH_LEN]);
		blake256_hash(out_p, in_p, BT_LEAF_SIZE);
	}
	
	*out_size = global * HASH_LEN;

	if(remainder) {
		blake256_hash(
			&(out[global * HASH_LEN]),
			&(in[global * BT_LEAF_SIZE]), 
			remainder);


		*out_size = (global + 1) * HASH_LEN;
	}
}
