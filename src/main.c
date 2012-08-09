// Calling this implementation tree hashing is borderline to lying.
// Cake hashing maybe?
//
// Ideas:
// - consider using mmap() instead of fread() for real files
#define _BSD_SOURCE

#include "BlakeTreeCPU.h"
#include "BlakeTreeGPU.h"
#include "log.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <stdbool.h>

#include <sys/time.h>

void usage() {
	fprintf(stderr, "Usage: blaketree [-c] [-t] name\n");
	fprintf(stderr, "  -C   Use CPU\n");
	fprintf(stderr, "  -t   Test mode\n");
	fprintf(stderr, "  -v   Verbose\n");
	exit(EXIT_FAILURE);
}
void action_file_cpu(char* filename);
void action_file_gpu(char* filename);

void action_test();
void test_gpu();


typedef struct {
	struct timeval t_start, t_end;
	double msec, sec;
} stopwatch_t;

void stopwatch_start(stopwatch_t *sw) {
	gettimeofday(&sw->t_start, NULL);
	sw->msec = sw->sec = 0;
}
double stopwatch_peek(stopwatch_t *sw) {
	struct timeval t_diff;
	gettimeofday(&sw->t_end, NULL);
	timersub(&sw->t_end, &sw->t_start, &t_diff);
	sw->msec = t_diff.tv_sec * 1000 + t_diff.tv_usec / 1000.0;
	sw->sec  = sw->msec / 1000.0;
	return sw->msec;
}


int main(int argc, char** argv) {
	int flags, opt;

	enum cmd_opts {
		FLAG_TEST    = 0x01,
		FLAG_CPU     = 0x02,
	};

	flags = 0;
	while ((opt = getopt(argc, argv, "tcvh")) != -1) 
	{
		switch (opt) 
		{
		case 't':
			flags |= FLAG_TEST;
			break;
		case 'c':
			flags |= FLAG_CPU;
			break;
		case 'v':
			logger_level = DEBUG;
			break;
		default: /* '?' */
			usage();
		}
	}

	loggerf(DEBUG, "Blake-256 CPU implementation: %s", BLAKE256_CPU_IMPL);

	if (!(flags & FLAG_TEST)) 
	{
		if(optind >= argc) 
		{
			usage();
		}
		else 
		{
			if(flags & FLAG_CPU) {
				action_file_cpu(argv[optind]);
			}
			else {
				action_file_gpu(argv[optind]);
			}
		}
	}
	else 
	{
		action_test();
	}

	return 0;
}


void hash2str(uint8_t* hash, char* out) {
	int i;
	char buf[HASH_LEN * 2 + 1];
	for(i=0; i<HASH_LEN; i++) {
		sprintf(&(buf[i*2]), "%02x", (int)hash[i]);
	}
	buf[HASH_LEN * 2] = 0;
	memcpy(out, buf, HASH_LEN * 2 + 1);
}


void action_file_cpu(char* filename) {
	state256 master_state;
	uint8_t  master_hash[HASH_LEN];
	char     master_hash_str[HASH_LEN * 2 + 1];
	uint8_t *src, *dst;
	size_t  dst_size;

	FILE* f;
	size_t bytes_read;
	uint64_t total_bytes_read;

	stopwatch_t sw;

	f = fopen(filename, "r");
	assert(f);
	total_bytes_read = 0;

	blake256_init(&master_state);

	src = malloc(FILE_BUFFER_SIZE);
	dst = malloc(STAGE1_SIZE);

	stopwatch_start(&sw);

	while( (bytes_read = fread(src, 1, FILE_BUFFER_SIZE, f)) )
	{
		total_bytes_read += bytes_read;
		blakeTreeCPU(src, bytes_read, dst, &dst_size);
		blake256_update(&master_state, dst, dst_size);
	}

	fclose(f);

	blake256_final(&master_state, master_hash);
	hash2str(master_hash, master_hash_str);
	logger(INFO, master_hash_str);
	stopwatch_peek(&sw);
	loggerf(DEBUG, "%.1f MiB/s", (total_bytes_read >> 20) / sw.sec);
}


void action_file_gpu(char* filename) {
	state256 master_state;
	uint8_t  master_hash[HASH_LEN];
	char     master_hash_str[HASH_LEN * 2 + 1];
	uint8_t *src, *dst;

	FILE* f;
	size_t bytes_read;
	uint64_t total_bytes_read;
	bool eof, done;

	stopwatch_t sw;

	f = fopen(filename, "r");
	assert(f);
	
	blake256_init(&master_state);
	blakeTreeGPU_init();

	total_bytes_read = 0;
	done = eof = false;
	stopwatch_start(&sw);

	while(!done) 
	{
		// read result
		size_t dst_size; 
		dst = blakeTreeGPU_acquire_dst(&dst_size);
		if(dst) {
			blake256_update(&master_state, dst, dst_size);
			blakeTreeGPU_release_dst();
		} else {
			if(eof) 
			{
				done = true;
			}
		}
			

		// push N new buffers
		// blocks if queue is full
		while(!eof && (src = blakeTreeGPU_acquire_src()))
		{
			bytes_read = fread(src, 1, FILE_BUFFER_SIZE, f);
			total_bytes_read += bytes_read;
			if(bytes_read == 0) 
			{
				eof = true;
			}
			blakeTreeGPU_enqueue_src(bytes_read);
		}
	}

	fclose(f);
	blakeTreeGPU_close();

	blake256_final(&master_state, master_hash);
	hash2str(master_hash, master_hash_str);
	logger(INFO, master_hash_str);

	stopwatch_peek(&sw);
	loggerf(DEBUG, "%.1f MiB/s", (total_bytes_read >> 20) / sw.sec);
}


void test_gpu() 
{
	uint64_t total_bytes_read = 0;
	uint8_t *src, *dst;
	stopwatch_t sw;

	blakeTreeGPU_init();
	stopwatch_start(&sw);
	
	while(sw.msec < 5000)
	{
		dst = blakeTreeGPU_acquire_dst(NULL);
		if(dst) {
			blakeTreeGPU_release_dst();
		}

		while((src = blakeTreeGPU_acquire_src()))
		{
			total_bytes_read += FILE_BUFFER_SIZE;
			memset(src, 0, FILE_BUFFER_SIZE);
			blakeTreeGPU_enqueue_src(FILE_BUFFER_SIZE);
		}

		stopwatch_peek(&sw);
	}

	loggerf(DEBUG, "%.1f MiB/s", (total_bytes_read >> 20) / sw.sec);
	blakeTreeGPU_close();
}


void action_test() {
	const size_t test_size = 123;
	uint8_t in[test_size], out[HASH_LEN];
	const char result[HASH_LEN] = { // blake256( { 123 * 0 } 
		0xa4, 0xad, 0x43, 0xa0, 0xdc, 0xe6, 0xd2, 0x83,
		0x83, 0x16, 0x5b, 0x0d, 0xf7, 0x12, 0x61, 0x86, 
		0x72, 0x1e, 0x41, 0xfa, 0x58, 0x6c, 0x0a, 0xb6, 
		0x68, 0x2b, 0x2c, 0xe6, 0xf4, 0xc7, 0x30, 0xb9 
	};
	memset(in, 0, test_size);
	logger(INFO, "CPU hash test...");
	blake256_hash(out, in, test_size);
	if(memcmp(out, result, HASH_LEN) == 0) {
		logger(INFO, "CPU hash is valid");
	} else {
		logger(ERROR, "CPU hash invalid");
		exit(1);
	}

	logger(INFO, "GPU hash test...");
	test_gpu();
}
