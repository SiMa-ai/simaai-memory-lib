//SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Sima ai
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/simaai/simaai_memory_ioctl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

#include "simaai_memory.h"

struct args {
	char chr;
	long int size;
	char target;
	unsigned int flags;
};

static int parse_args(const int argc, char *const argv[], struct args *args)
{
	const char *filename = argv[0];
	struct option long_options[] = {
		{ "help",   no_argument,       NULL, 'h' },
		{ "value",  required_argument, NULL, 'v' },
		{ "size",   required_argument, NULL, 's' },
		{ "target", required_argument, NULL, 't' },
		{ "cached", no_argument,       NULL, 'c' },
		{ "readonly", no_argument,     NULL, 'r' },
		{ 0,        0,                 0,     0  }
	};
	const char usage[] =
		"Usage: %s [OPTION]\n"
		"Verify SiMa.ai memory management device.\n"
		"\n"
		"  -h, --help          display this help and exit\n"
		"  -v, --value=SYMBOL  a value of the symbol to exchange via shared memory\n"
		"  -s, --size=SIZE     bytes to write to/read from mapped memory buffer\n"
		"  -t, --target=[0..6] target to allocate memory from, CMA (0) or OCM (1) DMS0-3 (2-5) EV (6)\n"
		"  -c, --cached        map memory as cached\n"
		"  -r, --readonly      map memory as readonly\n";
	int option_index;
	int c;

	while (1) {
		option_index = 0;
		c = getopt_long(argc, argv, "hv:s:t:cr", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 0:
			/* getopt_long already set a value of option */
			break;
		case 'h':
			fprintf(stderr, usage, basename(filename));
			return -1;
			break;
		case 'v':
			args->chr = *optarg;
			break;
		case 's':
			args->size = strtol(optarg, NULL, 10);
			if (args->size < 0) {
				fprintf(stderr, "Invalid size value\n");
				return -1;
			}
			break;
		case 't':
			args->target = strtol(optarg, NULL, 10);
			if ((args->target < 0) || (args->target > 6)) {
				fprintf(stderr, "Invalid target\n");
				return -1;
			}
			break;
		case 'c':
			args->flags |= SIMAAI_MEM_FLAG_CACHED;
			break;
		case 'r':
			args->flags |= SIMAAI_MEM_FLAG_RDONLY;
			break;
		default:
			fprintf(stderr, usage, basename(filename));
			return -1;
			break;
		}
	}

	return 0;
}

static void memory_info_wrapper(simaai_memory_t *buf)
{
	fprintf(stdout,
		"Buffer: size = %zu, phys address = 0x%0lx\n",
		simaai_memory_get_size(buf),
		simaai_memory_get_phys(buf));
}

static void test_memory_wrapper(const struct args *args)
{
	simaai_memory_t *mem_out, *mem_in;
	void *vaddr_out, *vaddr_in;
	char *data;
	clock_t start, end;

	fprintf(stdout, "Allocate output memory\n");
	if (args->flags == SIMAAI_MEM_FLAG_DEFAULT)
		mem_out = simaai_memory_alloc(args->size, args->target);
	else
		mem_out = simaai_memory_alloc_flags(args->size, args->target, args->flags);
	if (!mem_out) {
		fprintf(stderr, "Output memory allocation failed: %s\n",
			strerror(errno));
		return;
	} else {
		memory_info_wrapper(mem_out);
	}

	fprintf(stdout, "Attach to the input memory\n");
	mem_in = simaai_memory_attach(simaai_memory_get_phys(mem_out));
	if (!mem_in) {
		fprintf(stderr, "Attachment to the input memory failed: %s\n",
			strerror(errno));
		return;
	} else {
		memory_info_wrapper(mem_in);
	}

	fprintf(stdout, "Map output memory\n");
	vaddr_out = simaai_memory_map(mem_out);
	if (!vaddr_out) {
		fprintf(stderr, "Output memory mapping failed: %s\n",
			strerror(errno));
		return;
	}

	fprintf(stdout, "Map input memory\n");
	vaddr_in = simaai_memory_map(mem_in);
	if (!vaddr_in) {
		fprintf(stderr, "Input memory mapping failed: %s\n",
			strerror(errno));
		return;
	}

	/* Use kernel buffers as shared memory */
	data = calloc(1, args->size);
	if (!data) {
		fprintf(stderr, "Data memory allocation failed: %s\n",
			strerror(errno));
		return;
	}

	fprintf(stdout, "Write '%c' to the output memory %ld times\n",
		args->chr, args->size);

	start = clock();
	memset(vaddr_out, args->chr, args->size);
	if(args->flags & SIMAAI_MEM_FLAG_CACHED)
		simaai_memory_flush_cache(mem_out);
	end = clock();
	fprintf(stdout, "time taken to write %f\n",((double)(end - start))/CLOCKS_PER_SEC);

	fprintf(stdout, "Unmap output memory\n");

	simaai_memory_unmap(mem_out);
	fprintf(stdout, "Read from input memory %zu symbols\n", args->size);

	start = clock();
	if(args->flags & SIMAAI_MEM_FLAG_CACHED)
		simaai_memory_invalidate_cache(mem_out);
	memcpy(data, vaddr_in, args->size);
	end = clock();
	fprintf(stdout, "time taken to read %f\n",((double)(end - start))/CLOCKS_PER_SEC);

	fprintf(stdout, "Print first 10 symbols from shared memory\n");
	int i;
	for (i = 0; i < ((10 < args->size) ? 10 : args->size); i++)
		printf("%c(0x%02x) ", data[i], data[i]);
	printf("\n");

	fprintf(stdout, "Unmap input memory\n");
	simaai_memory_unmap(mem_in);

	fprintf(stdout, "Free input and output memory\n");
	simaai_memory_free(mem_in);
	simaai_memory_free(mem_out);
}

int main(int argc, char *argv[])
{
	struct args args = { 0 };

	if (parse_args(argc, argv, &args) != 0)
		return EXIT_FAILURE;

	test_memory_wrapper(&args);

	return EXIT_SUCCESS;
}
