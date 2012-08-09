#pragma once

#include <CL/opencl.h>
#include <stdio.h>
#include <stdlib.h>

void ocl_strerror(cl_int err, char* buf, size_t buflen);

#define OCL_ERRBUF_SIZE 1024
extern char _ocl_errbuf[OCL_ERRBUF_SIZE];

#define ocl_assert(err) \
	if(err != CL_SUCCESS) { \
		ocl_strerror(err, _ocl_errbuf, OCL_ERRBUF_SIZE); \
		fprintf(stderr, "\nOpenCL assertion failed @ %s:%d | %s\n", __FILE__, __LINE__, _ocl_errbuf); \
		exit(1); \
	} 

