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
#define SIMAAI_OCM_ALLOCATOR	"/dev/simaai-ocm"
#define SIMAAI_DMS0_ALLOCATOR	"/dev/simaai-dms0"
#define SIMAAI_DMS1_ALLOCATOR	"/dev/simaai-dms1"
#define SIMAAI_DMS2_ALLOCATOR	"/dev/simaai-dms2"
#define SIMAAI_DMS3_ALLOCATOR	"/dev/simaai-dms3"
#define SIMAAI_EV74_ALLOCATOR	"/dev/simaai-evmem"
#define SIMAAI_CACHE_LINE_SIZE	(64)

struct simaai_memory_t {
	/* Allocator file descriptor */
	int fd;
	/* Virtual address of memory chunk */
	void *vaddr;
	/* Size of memory chunk */
	unsigned int size;
	/* ID of memory chunk assigned by the allocator */
	unsigned int id;
	/* Physical address of the memory chunk */
	uint64_t phys_addr;
};

simaai_memory_t *simaai_memory_alloc_flags(unsigned int size, int target, int flags)
{
	simaai_memory_t *memory;
	struct simaai_memory_info info = {0};
	struct simaai_alloc_args alloc_args = {0};
	int ret;

	memory = calloc(1, sizeof(*memory));
	if (!memory)
		return NULL;

	switch(target) {
	case SIMAAI_MEM_TARGET_OCM:
		memory->fd = open(SIMAAI_OCM_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	case SIMAAI_MEM_TARGET_DMS0:
		memory->fd = open(SIMAAI_DMS0_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	case SIMAAI_MEM_TARGET_DMS1:
		memory->fd = open(SIMAAI_DMS1_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	case SIMAAI_MEM_TARGET_DMS2:
		memory->fd = open(SIMAAI_DMS2_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	case SIMAAI_MEM_TARGET_DMS3:
		memory->fd = open(SIMAAI_DMS3_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	case SIMAAI_MEM_TARGET_EV74:
		memory->fd = open(SIMAAI_EV74_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	default:
		memory->fd = open(SIMAAI_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	}

	if (memory->fd < 0) {
		free(memory);
		return NULL;
	}

	alloc_args.size = size;
	alloc_args.flags = flags;
	if (target & (SIMAAI_MEM_TARGET_EV74 | SIMAAI_MEM_TARGET_M4 | SIMAAI_MEM_TARGET_OCM
			| SIMAAI_MEM_TARGET_DMS0 | SIMAAI_MEM_TARGET_DMS1
			| SIMAAI_MEM_TARGET_DMS2 | SIMAAI_MEM_TARGET_DMS3))
		ret = ioctl(memory->fd, SIMAAI_IOC_MEM_ALLOC_COHERENT, &alloc_args);
	else
		ret = ioctl(memory->fd, SIMAAI_IOC_MEM_ALLOC_GENERIC, &alloc_args);

	if (ret < 0) {
		close(memory->fd);
		free(memory);
		return NULL;
	}

	if (ioctl(memory->fd, SIMAAI_IOC_MEM_INFO, &info) < 0) {
		close(memory->fd);
		free(memory);
		return NULL;
	}

	memory->size = info.size;
	memory->id = info.id;
	memory->phys_addr = info.phys_addr;

	return memory;
}

simaai_memory_t *simaai_memory_alloc(unsigned int size, int target)
{
	return simaai_memory_alloc_flags(size, target, SIMAAI_MEM_FLAG_DEFAULT);
}

simaai_memory_t *simaai_memory_attach(unsigned int id)
{
	simaai_memory_t *memory;
	struct simaai_memory_info info = {0};
	memory = calloc(1, sizeof(*memory));
	if (!memory)
		return NULL;

	switch(SIMAAI_GET_TARGET_ALLOCATOR(id)) {
	case SIMAAI_TARGET_ALLOCATOR_OCM:
		memory->fd = open(SIMAAI_OCM_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	case SIMAAI_TARGET_ALLOCATOR_DMS0:
		memory->fd = open(SIMAAI_DMS0_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	case SIMAAI_TARGET_ALLOCATOR_DMS1:
		memory->fd = open(SIMAAI_DMS1_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	case SIMAAI_TARGET_ALLOCATOR_DMS2:
		memory->fd = open(SIMAAI_DMS2_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	case SIMAAI_TARGET_ALLOCATOR_DMS3:
		memory->fd = open(SIMAAI_DMS3_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	case SIMAAI_TARGET_ALLOCATOR_EV74:
		memory->fd = open(SIMAAI_EV74_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	default:
		memory->fd = open(SIMAAI_ALLOCATOR, O_RDWR | O_SYNC);
		break;
	}

	if (memory->fd < 0) {
		free(memory);
		return NULL;
	}

	if (ioctl(memory->fd, SIMAAI_IOC_MEM_GET, &id) < 0) {
		close(memory->fd);
		free(memory);
		return NULL;
	}

	if (ioctl(memory->fd, SIMAAI_IOC_MEM_INFO, &info) < 0) {
		close(memory->fd);
		free(memory);
		return NULL;
	}

	memory->size = info.size;
	memory->id = info.id;
	memory->phys_addr = info.phys_addr;

	return memory;
}

void simaai_memory_free(simaai_memory_t *memory)
{
	assert(memory);

	if (memory->vaddr)
		munmap(memory->vaddr, memory->size);

	ioctl(memory->fd, SIMAAI_IOC_MEM_FREE);
	close(memory->fd);
	free(memory);
}

void *simaai_memory_map(simaai_memory_t *memory)
{
	assert(memory);

	void *vaddr = mmap(NULL, memory->size, PROT_READ | PROT_WRITE,
			   MAP_SHARED, memory->fd, 0);

	if (vaddr == MAP_FAILED)
		vaddr = NULL;
	else
		memory->vaddr = vaddr;

	return vaddr;
}

void simaai_memory_unmap(simaai_memory_t *memory)
{
	assert(memory);

	if (memory->vaddr) {
		munmap(memory->vaddr, memory->size);
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

size_t simaai_memory_get_size(simaai_memory_t *memory)
{
	assert(memory);

	return memory->size;
}

unsigned int simaai_memory_get_id(simaai_memory_t *memory)
{
	assert(memory);

	return memory->id;
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
		simaai_memory_exec_op_cvac(memory->vaddr + offset, size);
	else if (op == 'i')
		simaai_memory_exec_op_civac(memory->vaddr + offset, size);
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
