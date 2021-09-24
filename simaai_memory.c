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

simaai_memory_t *simaai_memory_alloc(unsigned int size, int target)
{
	simaai_memory_t *memory;
	struct simaai_memory_info info = {0};
	int ret;

	memory = calloc(1, sizeof(*memory));
	if (!memory)
		return NULL;

	memory->fd = open(SIMAAI_ALLOCATOR, O_RDWR | O_SYNC);
	if (memory->fd < 0) {
		free(memory);
		return NULL;
	}

	if (target & (SIMAAI_MEM_TARGET_EV74 | SIMAAI_MEM_TARGET_M4))
		ret = ioctl(memory->fd, SIMAAI_IOC_MEM_ALLOC_DMA32, &size);
	else
		ret = ioctl(memory->fd, SIMAAI_IOC_MEM_ALLOC_GENERIC, &size);

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

simaai_memory_t *simaai_memory_attach(unsigned int id)
{
	simaai_memory_t *memory;
	struct simaai_memory_info info = {0};

	memory = calloc(1, sizeof(*memory));
	if (!memory)
		return NULL;

	memory->fd = open(SIMAAI_ALLOCATOR, O_RDWR | O_SYNC);
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
