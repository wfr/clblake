#pragma once

#include "BlakeTree.h"


void blakeTreeGPU_init();

void blakeTreeGPU_close();

// request a new buffer
// > if the queue is full, block until the head is done, return NULL
// > else initialize the buffer, return its address
uint8_t* blakeTreeGPU_acquire_src();

// add the most recently acquired buffer to the command queue
void     blakeTreeGPU_enqueue_src(size_t length); 

// fetch a completed buffer
// > returns NULL if the queue is empty
// > blocks and returns the head buffer otherwise
uint8_t* blakeTreeGPU_acquire_dst(size_t *dst_size); 

// release the output buffer
void     blakeTreeGPU_release_dst(); 


int blakeTreeGPU_pending();
//int blakeTreeGPU_wait();

// missing: resource cleanup 
