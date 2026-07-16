#include "include/malloc.h"
#include "include/syscall.h"
#include "include/string.h"

struct block_header {
    size_t size; // Size of payload (excluding header), low bit set to 1 if allocated
};

#define BLOCK_HEADER_SIZE sizeof(struct block_header)
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

static void *heap_start = NULL;

void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t aligned_size = ALIGN(size);

    if (heap_start == NULL) {
        heap_start = sys_sbrk(0);
        if (heap_start == (void *)-1) {
            return NULL;
        }
    }

    void *current_break = sys_sbrk(0);
    char *curr = (char *)heap_start;

    // First-fit block search
    while (curr < (char *)current_break) {
        struct block_header *header = (struct block_header *)curr;
        size_t block_size = header->size & ~1;
        int is_allocated = header->size & 1;

        if (!is_allocated && block_size >= aligned_size) {
            // Found a free block! Can we split it?
            if (block_size >= aligned_size + BLOCK_HEADER_SIZE + ALIGNMENT) {
                // Split the block
                struct block_header *next_header = (struct block_header *)(curr + BLOCK_HEADER_SIZE + aligned_size);
                next_header->size = (block_size - aligned_size - BLOCK_HEADER_SIZE) & ~1;

                header->size = (aligned_size | 1);
            } else {
                // Use the whole block
                header->size |= 1;
            }
            return (void *)(curr + BLOCK_HEADER_SIZE);
        }
        curr += BLOCK_HEADER_SIZE + block_size;
    }

    // No free block found, request more space
    size_t required = BLOCK_HEADER_SIZE + aligned_size;
    void *prev_break = sys_sbrk((long)required);
    if (prev_break == (void *)-1) {
        return NULL;
    }

    struct block_header *header = (struct block_header *)prev_break;
    header->size = (aligned_size | 1);

    return (void *)((char *)prev_break + BLOCK_HEADER_SIZE);
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    struct block_header *header = (struct block_header *)((char *)ptr - BLOCK_HEADER_SIZE);
    header->size &= ~1; // Mark as free

    // Coalesce free blocks
    if (heap_start == NULL) return;
    void *current_break = sys_sbrk(0);
    char *curr = (char *)heap_start;

    while (curr < (char *)current_break) {
        struct block_header *current_header = (struct block_header *)curr;
        size_t current_size = current_header->size & ~1;
        int current_allocated = current_header->size & 1;

        if (!current_allocated) {
            char *next = curr + BLOCK_HEADER_SIZE + current_size;
            if (next < (char *)current_break) {
                struct block_header *next_header = (struct block_header *)next;
                size_t next_size = next_header->size & ~1;
                int next_allocated = next_header->size & 1;

                if (!next_allocated) {
                    // Merge next block into current
                    current_header->size = (current_size + BLOCK_HEADER_SIZE + next_size) & ~1;
                    continue; // Re-evaluate current block
                }
            }
        }
        curr += BLOCK_HEADER_SIZE + current_size;
    }
}

void *calloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = malloc(total);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    struct block_header *header = (struct block_header *)((char *)ptr - BLOCK_HEADER_SIZE);
    size_t current_size = header->size & ~1;
    if (current_size >= size) {
        return ptr; // Existing block is large enough
    }

    void *new_ptr = malloc(size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, current_size);
        free(ptr);
    }
    return new_ptr;
}
