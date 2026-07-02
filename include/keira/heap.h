#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

/**
 * Keira Kernel: Simple Bump Allocator (Heap)
 *
 * Provides basic dynamic memory allocation for the kernel.
 */

/**
 * Initialize the kernel heap.
 *
 * @param start Start address of the heap region
 * @param size  Size of the heap region in bytes
 */
void heap_init(void *start, size_t size);

/**
 * Allocate a block of memory from the kernel heap.
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL if out of memory
 */
void *kmalloc(size_t size);

/**
 * Free a previously allocated block (no-op in bump allocator).
 *
 * @param ptr Pointer to memory block to free
 */
void kfree(void *ptr);

/**
 * Get total heap size in bytes.
 */
size_t heap_get_total(void);

/**
 * Get number of bytes currently used.
 */
size_t heap_get_used(void);

/**
 * Get number of bytes free.
 */
size_t heap_get_free(void);

#endif /* HEAP_H */
