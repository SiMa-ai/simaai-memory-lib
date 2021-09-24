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

#define SIMAAI_MEM_TARGET_ALL		(SIMAAI_MEM_TARGET_GENERIC | \
					 SIMAAI_MEM_TARGET_MOSAIC | \
					 SIMAAI_MEM_TARGET_M4 | \
					 SIMAAI_MEM_TARGET_EV74 | \
					 SIMAAI_MEM_TARGET_EM4)

#define SIMAAI_MEM_TARGET_UNKNOWN	SIMAAI_MEM_TARGET_ALL

/**
 * @brief Allocate contiguous memory chunk of given size.
 *
 * @param size Memory chunk size.
 * @param target Target memory type to allocate.
 * @return Allocated memory chunk context that can later be successfully
 *         passed to simaai_memory_* functions or NULL in case of failure.
 */
simaai_memory_t *simaai_memory_alloc(unsigned int size, int target);

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

#ifdef __cplusplus
}
#endif /* extern "C" { */
#endif /* _SIMAAI_MEMORY_H_ */
