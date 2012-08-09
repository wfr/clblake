#include "BlakeTreeGPU.h"
#include "BlakeTreeCPU.h"
#include <CL/opencl.h>
#include <assert.h>
#include <stdbool.h>
#include <glib.h>

#include "opencl-util.h"
#include "log.h"

// buffers
// > organized in a FIFO queue
#define NUM_BUFFERS 4

typedef struct {
	uint8_t* src;
	uint8_t* dst;
	cl_mem cm_src;
	cl_mem cm_dst;

	bool busy;
	cl_event ev_src_unmap; // src has been copied to the GPU
	cl_event ev_kernel;    // src has been hashed and dst can be read

	size_t dst_size;

	// Uneven remainders are hashed by the CPU
	// > when the result is acquired
	size_t global_work_items;
	size_t remainder;

} buffer_t;

static buffer_t buffers[NUM_BUFFERS];
GQueue *bufqueue;


// global OpenCL state
static cl_device_id device_id;
static cl_context context;
static cl_command_queue q_compute;
static cl_command_queue q_transfer;
static cl_program program;
static cl_kernel kernel;


void blakeTreeGPU_alloc_buffer(buffer_t* bp);
void blakeTreeGPU_free_buffer(buffer_t* bp);


void blakeTreeGPU_init()
{
	int err;

	// Determine platform
	// > use the first GPU
	//
	const int max_platforms = 16;
	cl_platform_id platforms[max_platforms];
	cl_uint num_platforms;
	err = clGetPlatformIDs(max_platforms, platforms, &num_platforms);
	ocl_assert(err);

	int success = 0;
	for(int i=0; i < num_platforms && !success; i++) {
		cl_platform_id pid = platforms[i];
		err = clGetDeviceIDs(pid, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
		if (err == CL_SUCCESS)
		{
			success = 1;
			char info[256];
			clGetPlatformInfo(pid, CL_PLATFORM_NAME, 256, info, NULL);
			loggerf(DEBUG, "Using platform: %s", info);
		}
	}
	if(!success) {
		loggerf(ERROR, "No OpenCL capable GPU found.");
		exit(1);
	}
	  
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	ocl_assert(err);

	// Read the programs
	char *sources[16];
	char *source_files[] = {
		"blake256.cl", 
	};
	int sources_count = sizeof(source_files)/sizeof(char*);
	for(int i=0; i<sources_count; i++) {
		loggerf(DEBUG, "Using OpenCL source: %s", source_files[i]);
		FILE *f = fopen(source_files[i], "r");
		assert(f);
		fseek(f, 0, SEEK_END);
		int flen = ftell(f);
		fseek(f, 0, SEEK_SET);
		sources[i] = malloc(flen + 1); 
		assert(sources[i]);
		fread(sources[i], 1, flen, f);
		sources[i][flen] = 0;
		fclose(f);
	}

	program = clCreateProgramWithSource(context, sources_count, 
		(const char **) sources, NULL, &err);
	ocl_assert(err);

	for(int i=0; i < sources_count; i++) {
		free(sources[i]);
	}

	err = clBuildProgram(program, 0, NULL, "-cl-mad-enable", NULL, NULL);
	if (err != CL_SUCCESS)
	{
		logger(ERROR, "Failed to build program executable!");
		size_t len;
		char buffer[0x100000];
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 
			sizeof(buffer), buffer, &len);
		logger(ERROR, buffer);
		exit(1);
	}

	// queues
	cl_command_queue_properties properties;
	#ifdef PROFILING
		properties = CL_QUEUE_PROFILING_ENABLE;
	#else
		properties = 0;
	#endif
	q_transfer = clCreateCommandQueue(context, device_id, properties, &err);
	ocl_assert(err);
	q_compute  = clCreateCommandQueue(context, device_id, properties, &err);
	ocl_assert(err);

	kernel = NULL;

	// buffers
	bufqueue = g_queue_new();
	for(int i=0; i < NUM_BUFFERS; i++) {
		blakeTreeGPU_alloc_buffer(&buffers[i]);
	}

	kernel = clCreateKernel(program, "blake256_hash_block", &err);
	ocl_assert(err);
}


void blakeTreeGPU_close() {
	for(int i=0; i < NUM_BUFFERS; i++) {
		blakeTreeGPU_free_buffer(&buffers[i]);
	}
	clReleaseProgram(program);
	clReleaseCommandQueue(q_compute);
	clReleaseCommandQueue(q_transfer);
	clReleaseContext(context);
	g_queue_free(bufqueue);
}


uint8_t* blakeTreeGPU_acquire_src() {
	//loggerf(DEBUG, "acquire_src, queue len: %d", g_queue_get_length(bufqueue));
	
	if(g_queue_get_length(bufqueue) >= NUM_BUFFERS) {
		buffer_t* h = g_queue_peek_head(bufqueue);
		clWaitForEvents(1, &h->ev_kernel);
		return NULL;
	}

	buffer_t* new = NULL;

	bool foundfree = false;
	for(int i=0; i < NUM_BUFFERS && !foundfree; i++) {
		if(!buffers[i].busy) {
			new = &buffers[i];
			new->busy = true;
			g_queue_push_tail(bufqueue, new);
			//loggerf(DEBUG, "push_tail");
			foundfree = true;
		}
	}

	if(!new) {
		loggerf(ERROR, "Logic error. No free buffer.");
		exit(1);
	}

	return new->src;
}


void blakeTreeGPU_enqueue_src(size_t length) {
	int err;

	if(length == 0)
	{
		g_queue_pop_tail(bufqueue);
		return;
	}

	size_t remainder  = length % BT_LEAF_SIZE;
	size_t global     = length / BT_LEAF_SIZE;
	size_t local;

	// avoid invalid work group sizes, round down to % 256, extend remainder
	if(global % 256 != 0) {
		size_t diff = global - ((global >> 8) << 8);
		loggerf(DEBUG, "Global work size reduced from %d to %d", global, diff);
		global    -= diff;
		remainder += diff * BT_LEAF_SIZE;
	}

	/*
	loggerf(DEBUG, "blakeTreeGPU_enqueue_src: pending: %d  global: %d items  remainder: %d byte", 
		blakeTreeGPU_pending(), global, remainder);
	*/

	buffer_t* new = g_queue_peek_tail(bufqueue);

	if(global > 0) {
		err = clEnqueueWriteBuffer( q_transfer,  new->cm_src, 
			CL_FALSE, 0, FILE_BUFFER_SIZE, new->src, 0, NULL, &new->ev_src_unmap);
		ocl_assert(err);
		
		cl_int bt_leaf_size = BT_LEAF_SIZE;
		err = 0;
		err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &new->cm_dst);
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &new->cm_src);
		err |= clSetKernelArg(kernel, 2, sizeof(cl_int), &bt_leaf_size);
		if (err != CL_SUCCESS)
		{
			loggerf(ERROR, "Failed to set kernel arguments!");
			exit(1);
		}
		
		err = clGetKernelWorkGroupInfo(kernel, device_id, 
			CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
		ocl_assert(err);
		local = MAX(32, (local >> 8) << 8);

		err = clEnqueueNDRangeKernel(q_compute, kernel, 1, NULL, &global, &local, 
				1, &new->ev_src_unmap, &new->ev_kernel);
		ocl_assert(err);
	}

	new->remainder = remainder;
	new->global_work_items = global;
	new->dst_size = global * HASH_LEN;
}


uint8_t* blakeTreeGPU_acquire_dst(size_t *dst_size) {
	int err;

	if(g_queue_get_length(bufqueue) == 0) {
		return NULL;
	}

	buffer_t* head = g_queue_peek_head(bufqueue);

	if(head->global_work_items > 0) {
		err = clEnqueueReadBuffer(
			q_transfer, 
			head->cm_dst, 
			CL_TRUE, 
			0, 
			STAGE1_SIZE, //head->dst_size,
			head->dst, 
			1, 
			&head->ev_kernel, 
			NULL);
		ocl_assert(err);
	}
	
	if(head->remainder)
	{
		size_t stage1_rem;
		blakeTreeCPU(
			&(head->src[head->global_work_items * BT_LEAF_SIZE]), 
			head->remainder,
			&(head->dst[head->global_work_items * HASH_LEN]),
			&stage1_rem);
		head->dst_size += stage1_rem;
	}

	if(dst_size) *dst_size = head->dst_size;
	return head->dst;
}


void blakeTreeGPU_release_dst() {
	buffer_t* head = g_queue_peek_head(bufqueue);
	
	head->busy = false;
	g_queue_pop_head(bufqueue);
	//loggerf(DEBUG, "pop_head");
}


void blakeTreeGPU_alloc_buffer(buffer_t* bp) {
	int err;

	bp->src = malloc(FILE_BUFFER_SIZE);
	bp->dst = malloc(STAGE1_SIZE);

	bp->cm_src = clCreateBuffer(context, 
		CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
		FILE_BUFFER_SIZE, bp->src, &err);
	ocl_assert(err);
	
	bp->cm_dst = clCreateBuffer(context,
		CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
		STAGE1_SIZE, bp->dst, &err);
	ocl_assert(err);
	
	bp->busy = false;
}


void blakeTreeGPU_free_buffer(buffer_t* bp) {
	clReleaseMemObject(bp->cm_src);
	clReleaseMemObject(bp->cm_dst);
	free(bp->src);
	free(bp->dst);
}


int blakeTreeGPU_pending()
{
	return g_queue_get_length(bufqueue);
}


int blakeTreeGPU_wait()
{
	if(g_queue_get_length(bufqueue) == 0) 
	{
		return 0;
	}
	else
	{
		buffer_t* h = g_queue_peek_head(bufqueue);
		clWaitForEvents(1, &h->ev_kernel);
		return 1;
	}	
}
