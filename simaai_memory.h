//SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Sima ai
 */

#ifndef _SIMAAI_MEMORY_H_
#define _SIMAAI_MEMORY_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>

/*
 * Memory chunk context.
 */
typedef struct simaai_memory_t simaai_memory_t;

/*
 * Memory type to allocate.
 * The memory type depends on the target processor because
 * some targets operate only in 32 bit address space,
 * others could use arbitrary allocated memory regions.
 */
#define SIMAAI_MEM_TARGET_GENERIC	(1 << 0)
#define SIMAAI_MEM_TARGET_MOSAIC	(1 << 1)
#define SIMAAI_MEM_TARGET_M4		(1 << 2)
#define SIMAAI_MEM_TARGET_EV74		(1 << 3)
#define SIMAAI_MEM_TARGET_EM4		(1 << 4)
#define SIMAAI_MEM_TARGET_OCM		(1 << 5)
#define SIMAAI_MEM_TARGET_DMS0	(1 << 6)
#define SIMAAI_MEM_TARGET_DMS1	(1 << 7)
#define SIMAAI_MEM_TARGET_DMS2	(1 << 8)
#define SIMAAI_MEM_TARGET_DMS3	(1 << 9)

#define SIMAAI_MEM_TARGET_ALL		(SIMAAI_MEM_TARGET_GENERIC | \
					 SIMAAI_MEM_TARGET_MOSAIC | \
					 SIMAAI_MEM_TARGET_M4 | \
					 SIMAAI_MEM_TARGET_EV74 | \
					 SIMAAI_MEM_TARGET_EM4)

#define SIMAAI_MEM_TARGET_UNKNOWN	SIMAAI_MEM_TARGET_ALL

#define SIMAAI_MEM_FLAG_CACHED	(1 << 0)
#define SIMAAI_MEM_FLAG_RDONLY	(1 << 1)
#define SIMAAI_MEM_FLAG_DEFAULT	(0x0)

/**
 * @brief Allocate contiguous memory chunk of given size with default flags:
 *        non-cachable, writable
 *
 * @param size Memory chunk size.
 * @param target Target memory type to allocate.
 * @return Allocated memory chunk context that can later be successfully
 *         passed to simaai_memory_* functions or NULL in case of failure.
 */
simaai_memory_t *simaai_memory_alloc(unsigned int size, int target);

/**
 * @brief Allocate contiguous memory chunk of given size with specific flags.
 *
 * @param size Memory chunk size.
 * @param target Target memory type to allocate.
 * @param flags Memory flags (cahceble, writable, etc).
 * @return Allocated memory chunk context that can later be successfully
 *         passed to simaai_memory_* functions or NULL in case of failure.
 */
simaai_memory_t *simaai_memory_alloc_flags(unsigned int size, int target, int flags);

/**
 * @brief Attach to the previously allocated memory chunk by ID.
 *
 * @param id Memory chunk ID.
 * @return Allocated memory chunk context that can later be successfully
 *         passed to simaai_memory_* functions or NULL in case of failure.
 */
simaai_memory_t *simaai_memory_attach(unsigned int id);

/**
 * @brief Free the previously allocated memory chunk.
 *
 * @param memory The context of the memory chunk to be freed.
 * @return None.
 */
void simaai_memory_free(simaai_memory_t *memory);

/**
 * @brief Map the previously allocated memory chunk.
 *
 * @param memory The memory chunk context.
 * @return A starting virtual address of memory chunk,
 *         or NULL in case of failure.
 */
void *simaai_memory_map(simaai_memory_t *memory);

/**
 * @brief Unmap the previously allocated and mapped memory chunk.
 *
 * @param memory The memory chunk context.
 * @return None.
 */
void simaai_memory_unmap(simaai_memory_t *memory);

/**
 * @brief Get the virtual address of the previously allocated
 *        and mapped memory chunk.
 *
 * @param memory The memory chunk context.
 * @return A memory chunk starting virtual address.
 */
void *simaai_memory_get_virt(simaai_memory_t *memory);

/**
 * @brief Get the physical address of the previously allocated memory chunk.
 *
 * @param memory The memory chunk context.
 * @return A memory chunk starting physical address.
 */
uint64_t simaai_memory_get_phys(simaai_memory_t *memory);

/**
 * @brief Get the size of the allocated memory chunk.
 *
 * @param memory The memory chunk context.
 * @return Memory chunk size.
 */
size_t simaai_memory_get_size(simaai_memory_t *memory);

/**
 * @brief Get ID of the allocated memory chunk.
 *
 * @param memory The memory chunk context.
 * @return An ID of the previously allocated memory chunk.
 */
unsigned int simaai_memory_get_id(simaai_memory_t *memory);

/**
 * @brief Flush cache of the allocated memory chunk.
 *        Should be called after the last write from the application cores.
 *
 * @param memory The memory chunk context.
 * @return None.
 */
void simaai_memory_flush_cache(simaai_memory_t *memory);

/**
 * @brief Flush cache of the allocated memory chunk.
 *        Should be called after the last write from the application cores.
 *
 * @param memory The memory chunk context.
 * @param size Memory chunk offset inside the buffer to flush.
 * @param size Memory chunk size to flush.
 * @return None.
 */
void simaai_memory_flush_cache_part(simaai_memory_t *memory, unsigned int offset, unsigned int size);

/**
 * @brief Invalidate cache of the allocated memory chunk.
 *        Should be called before the first read from the application cores.
 *
 * @param memory The memory chunk context.
 * @return None.
 */
void simaai_memory_invalidate_cache(simaai_memory_t *memory);

/**
 * @brief Invalidate cache of the allocated memory chunk.
 *        Should be called before the first read from the application cores.
 *
 * @param memory The memory chunk context.
 * @param size Memory chunk offset inside the buffer to invalidate.
 * @param size Memory chunk size to invalidate.
 * @return None.
 */
void simaai_memory_invalidate_cache_part(simaai_memory_t *memory, unsigned int offset, unsigned int size);

#ifdef __cplusplus
}
#endif /* extern "C" { */
#endif /* _SIMAAI_MEMORY_H_ */
