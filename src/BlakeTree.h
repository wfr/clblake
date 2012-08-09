#pragma once

#include "blake.h"

#define HASH_LEN 32

#define BT_LEAF_SIZE 2048
#define FILE_BUFFER_SIZE (1 << 23)
#define STAGE1_SIZE ((FILE_BUFFER_SIZE * HASH_LEN) / BT_LEAF_SIZE)

//#define PROFILING 1

//void calculateWorkGroups(size_t length, size_t *global, size_t *remainder);
