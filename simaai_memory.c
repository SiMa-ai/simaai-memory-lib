//SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Sima ai
 */

#include "simaai_memory.h"

#include <assert.h>
#include <fcntl.h>
#include <linux/simaai/simaai_memory_ioctl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define SIMAAI_ALLOCATOR	"/dev/simaai-mem"
#define SIMAAI_CACHE_LINE_SIZE	(64)

struct simaai_memory_t {
	/* Virtual address of memory chunk */
	void *vaddr;
	/* Size of memory chunk */
	unsigned int size;
	/* Physical address of the memory chunk */
	uint64_t phys_addr;
	/* Bus address of the memory chunk */
	uint64_t bus_addr;
	/* Target allocation hardware */
	uint64_t target;
	/* indicates offset from parent segment */
	uint64_t offset;
};

static int fd = -1;

simaai_memory_t *simaai_memory_alloc_flags(unsigned int size, int target, int flags)
{
	simaai_memory_t *memory;
	struct simaai_alloc_args alloc_args = {0};
	int ret;

	memory = calloc(1, sizeof(*memory));
	if (!memory)
		return NULL;

	if(fd < 0)
		fd = open(SIMAAI_ALLOCATOR, O_RDWR | O_SYNC);

	if (fd < 0) {
		free(memory);
		return NULL;
	}

	alloc_args.num_of_segments = 1;
	alloc_args.size[0] = size;
	alloc_args.flags = flags;
	alloc_args.target = target;
	ret = ioctl(fd, SIMAAI_IOC_MEM_ALLOC_COHERENT, &alloc_args);

	if (ret < 0) {
		free(memory);
		return NULL;
	}

	memory->target = target;
	memory->offset = alloc_args.offset[0];
	memory->size = alloc_args.size[0];
	memory->phys_addr = alloc_args.phys_addr[0];
	memory->bus_addr = alloc_args.bus_addr[0];

	return memory;
}

simaai_memory_t *simaai_memory_alloc(unsigned int size, int target)
{
	return simaai_memory_alloc_flags(size, target, SIMAAI_MEM_FLAG_DEFAULT);
}

static void free_segment_memory(struct simaai_memory_t **segments_memory, unsigned int last_index)
{
	unsigned int iter = 0;
	for(iter = 0; iter < last_index; iter++)
		free(segments_memory[iter]);
}

simaai_memory_t **simaai_memory_alloc_segments_flags(uint32_t *segments, uint32_t num_of_segments,
		int target, int flags)
{

	simaai_memory_t **segments_memory;
	simaai_memory_t *memory;
	struct simaai_alloc_args alloc_args = {0};
	int ret;
	unsigned int iter = 0;

	if (num_of_segments > MAX_SEGMENTS)
		return NULL;

	segments_memory = (simaai_memory_t **)calloc(num_of_segments, sizeof(simaai_memory_t *));
	if (!segments_memory)
		return NULL;

	if(fd < 0)
		fd = open(SIMAAI_ALLOCATOR, O_RDWR | O_SYNC);

	if (fd < 0) {
		free(segments_memory);
		return NULL;
	}

	alloc_args.num_of_segments = num_of_segments;
	alloc_args.flags = flags;
	alloc_args.target = target;
	for (iter  = 0; iter < num_of_segments; iter++)
		alloc_args.size[iter] = segments[iter];

	ret = ioctl(fd, SIMAAI_IOC_MEM_ALLOC_COHERENT, &alloc_args);
	if (ret < 0) {
		free(segments_memory);
		return NULL;
	}

	for (iter = 0; iter < num_of_segments; iter++) {

		memory = calloc(1, sizeof(*memory));
		if (!memory) {
			free_segment_memory(segments_memory, iter);
			free(segments_memory);
			return NULL;
		}

		segments_memory[iter] = memory;
		memory->target = target;
		memory->offset = alloc_args.offset[iter];
		memory->size = alloc_args.size[iter];
		memory->phys_addr = alloc_args.phys_addr[iter];
		memory->bus_addr = alloc_args.bus_addr[iter];
	}

	return segments_memory;
}

simaai_memory_t **simaai_memory_alloc_segments(uint32_t *segments, uint32_t num_of_segments, int target)
{
	return simaai_memory_alloc_segments_flags(segments, num_of_segments, target, SIMAAI_MEM_FLAG_DEFAULT);
}

simaai_memory_t *simaai_memory_attach(uint64_t phys_addr)
{
	simaai_memory_t *memory;
	struct simaai_memory_info info = {0};
	memory = calloc(1, sizeof(*memory));
	if (!memory)
		return NULL;

	if(fd < 0)
		fd = open(SIMAAI_ALLOCATOR, O_RDWR | O_SYNC);

	if (fd < 0) {
		free(memory);
		return NULL;
	}

	info.phys_addr = phys_addr;
	if (ioctl(fd, SIMAAI_IOC_MEM_INFO, &info) < 0) {
		free(memory);
		return NULL;
	}

	memory->offset = info.offset;
	memory->target = info.target;
	memory->size = info.size;
	memory->phys_addr = info.phys_addr;
	memory->bus_addr = info.bus_addr;

	return memory;
}

void simaai_memory_free(simaai_memory_t *memory)
{
	struct simaai_free_args free_args = {0};

	assert(memory);

	if(fd < 0)
		fd = open(SIMAAI_ALLOCATOR, O_RDWR | O_SYNC);

	if (fd < 0) {
		return;
	}

	if (memory->vaddr)
		munmap(memory->vaddr - memory->offset, memory->size);
	free_args.num_of_segments = 1;
	free_args.phys_addr[0] = memory->phys_addr;
	ioctl(fd, SIMAAI_IOC_MEM_FREE, &free_args);
	free(memory);
}

void simaai_memory_free_segments(simaai_memory_t **segments, unsigned int num_of_segments)
{
	struct simaai_free_args free_args = {0};
	unsigned int iter = 0;

	assert(segments);

	if(fd < 0)
		fd = open(SIMAAI_ALLOCATOR, O_RDWR | O_SYNC);

	if (fd < 0) {
		return;
	}

	free_args.num_of_segments = num_of_segments;
	for (iter = 0; iter < num_of_segments; iter++) {

		assert(segments[iter]);
		if (segments[iter]->vaddr)
			munmap(segments[iter]->vaddr - segments[iter]->offset, segments[iter]->size);
		free_args.phys_addr[iter] = segments[iter]->phys_addr;

		free(segments[iter]);
	}

	ioctl(fd, SIMAAI_IOC_MEM_FREE, &free_args);
	free(segments);
}

void *simaai_memory_map(simaai_memory_t *memory)
{
	assert(memory);

	if(fd < 0)
		fd = open(SIMAAI_ALLOCATOR, O_RDWR | O_SYNC);

	if (fd < 0) {
		return NULL;
	}

	void *vaddr = mmap(NULL, (memory->size + memory->offset), PROT_READ | PROT_WRITE,
			   MAP_SHARED, fd, (memory->phys_addr - memory->offset));

	if (vaddr == MAP_FAILED)
		vaddr = NULL;
	else
		memory->vaddr = (uint64_t)vaddr + memory->offset;

	return memory->vaddr;
}

void simaai_memory_unmap(simaai_memory_t *memory)
{
	assert(memory);

	if (memory->vaddr) {
		munmap(memory->vaddr - memory->offset, memory->size + memory->offset);
		memory->vaddr = NULL;
	}
}

void *simaai_memory_get_virt(simaai_memory_t *memory)
{
	assert(memory);

	if (memory->vaddr)
		return memory->vaddr;
	else
		return NULL;
}

uint64_t simaai_memory_get_phys(simaai_memory_t *memory)
{
	assert(memory);

	return memory->phys_addr;
}

uint64_t simaai_memory_get_bus(simaai_memory_t *memory)
{
	assert(memory);

	return memory->bus_addr;
}

size_t simaai_memory_get_size(simaai_memory_t *memory)
{
	assert(memory);

	return memory->size;
}

uint32_t simaai_memory_get_target(simaai_memory_t *memory)
{
	assert(memory);

	return memory->target;
}

static void simaai_memory_exec_op_cvac(uint64_t start, uint64_t size)
{
	uint64_t i;

	for (i = start; i < (start + size); i += SIMAAI_CACHE_LINE_SIZE) {
		__asm__ __volatile__("dc cvac, %0\n\t" : : "r" (i) :"memory");
	}
	__asm__ __volatile__("dsb st\n\t" : : :"memory");
}

static void simaai_memory_exec_op_civac(uint64_t start, uint64_t size)
{
	uint64_t i;

	for (i = start; i < (start + size); i += SIMAAI_CACHE_LINE_SIZE) {
		__asm__ __volatile__("dc civac, %0\n\t" : : "r" (i) :"memory");
	}
	__asm__ __volatile__("dsb sy\n\t" : : :"memory");
}

static void simaai_memory_op_cache(simaai_memory_t *memory,
		unsigned int offset, unsigned int size, const char op)
{
	assert(memory);
	if (!memory->vaddr)
		return;

	if (size == 0)
		size = memory->size;

	if ((offset + size) > memory->size)
		size = memory->size - offset;

	if (op == 'c')
		simaai_memory_exec_op_cvac((uint64_t)(memory->vaddr) + offset, size);
	else if (op == 'i')
		simaai_memory_exec_op_civac((uint64_t)(memory->vaddr) + offset, size);
}

void simaai_memory_flush_cache(simaai_memory_t *memory)
{
	simaai_memory_op_cache(memory, 0, 0, 'c');
}

void simaai_memory_flush_cache_part(simaai_memory_t *memory,
		unsigned int offset, unsigned int size)
{
	simaai_memory_op_cache(memory, offset, size, 'c');
}

void simaai_memory_invalidate_cache(simaai_memory_t *memory)
{
	simaai_memory_op_cache(memory, 0, 0, 'i');
}

void simaai_memory_invalidate_cache_part(simaai_memory_t *memory,
		unsigned int offset, unsigned int size)
{
	simaai_memory_op_cache(memory, offset, size, 'i');
}
