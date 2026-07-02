#include "../../include/keira/heap.h"

/**
 * Keira Kernel: Simple Bump Allocator
 *
 * This is a minimal bump allocator. It allocates memory by simply
 * incrementing a pointer. kfree is a no-op (memory is never reclaimed).
 * This is sufficient for a hobby OS kernel's early needs.
 *
 * All allocations are 16-byte aligned for safety.
 */

static uint8_t *heap_start = 0;
static uint8_t *heap_end = 0;
static uint8_t *heap_next = 0;

void heap_init(void *start, size_t size) {
    heap_start = (uint8_t *)start;
    heap_end = heap_start + size;
    heap_next = heap_start;
}

void *kmalloc(size_t size) {
    if (size == 0)
        return (void *)0;

    /* Align to 16 bytes */
    size = (size + 15) & ~((size_t)15);

    if (heap_next + size > heap_end) {
        /* Out of memory */
        return (void *)0;
    }

    void *ptr = heap_next;
    heap_next += size;
    return ptr;
}

void kfree(void *ptr) {
    /* No-op for bump allocator */
    (void)ptr;
}

size_t heap_get_total(void) {
    return (size_t)(heap_end - heap_start);
}

size_t heap_get_used(void) {
    return (size_t)(heap_next - heap_start);
}

size_t heap_get_free(void) {
    return (size_t)(heap_end - heap_next);
}
