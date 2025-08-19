#define MEM_TRACK_INTERNAL
#include "mem_track.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Global memory statistics
static mem_stats_t g_mem_stats = {0};

// Header structure for tracking allocations
typedef struct mem_header {
    size_t size;
    const char* file;
    int line;
    uint32_t magic;  // Magic number for corruption detection
} mem_header_t;

#define MEM_MAGIC 0xDEADBEEF
#define HEADER_SIZE sizeof(mem_header_t)

void* tracked_malloc(size_t size, const char* file, int line) {
    if (size == 0) {
        return NULL;
    }

    // Allocate extra space for header
    void* raw_ptr = malloc(size + HEADER_SIZE);
    if (!raw_ptr) {
        printf("Memory allocation failed: %zu bytes at %s:%d\r\n", size, file, line);
        return NULL;
    }

    // Set up header
    mem_header_t* header = (mem_header_t*)raw_ptr;
    header->size = size;
    header->file = file;
    header->line = line;
    header->magic = MEM_MAGIC;

    // Update statistics
    g_mem_stats.total_allocated += size;
    g_mem_stats.current_allocated += size;
    g_mem_stats.allocation_count++;

    if (g_mem_stats.current_allocated > g_mem_stats.peak_allocated) {
        g_mem_stats.peak_allocated = g_mem_stats.current_allocated;
    }

    printf("MALLOC: %zu bytes at %s:%d (total: %zu)\r\n", size, file, line, g_mem_stats.current_allocated);

    // Return pointer after header
    return (char*)raw_ptr + HEADER_SIZE;
}

void* tracked_calloc(size_t nmemb, size_t size, const char* file, int line) {
    size_t total_size = nmemb * size;
    void* ptr = tracked_malloc(total_size, file, line);
    
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

void* tracked_realloc(void* ptr, size_t size, const char* file, int line) {
    if (!ptr) {
        return tracked_malloc(size, file, line);
    }

    if (size == 0) {
        tracked_free(ptr, file, line);
        return NULL;
    }

    // Get original header
    mem_header_t* old_header = (mem_header_t*)((char*)ptr - HEADER_SIZE);
    
    // Verify magic number
    if (old_header->magic != MEM_MAGIC) {
        printf("Memory corruption detected in realloc at %s:%d\r\n", file, line);
        return NULL;
    }

    size_t old_size = old_header->size;

    // Allocate new memory
    void* new_ptr = tracked_malloc(size, file, line);
    if (!new_ptr) {
        return NULL;
    }

    // Copy old data
    size_t copy_size = (old_size < size) ? old_size : size;
    memcpy(new_ptr, ptr, copy_size);

    // Free old memory
    tracked_free(ptr, file, line);

    return new_ptr;
}

void tracked_free(void* ptr, const char* file, int line) {
    if (!ptr) {
        return;
    }

    // Get header
    mem_header_t* header = (mem_header_t*)((char*)ptr - HEADER_SIZE);
    
    // Verify magic number
    if (header->magic != MEM_MAGIC) {
        printf("Memory corruption detected in free at %s:%d\r\n", file, line);
        return;
    }

    // Update statistics
    g_mem_stats.current_allocated -= header->size;
    g_mem_stats.free_count++;

    printf("FREE: %zu bytes at %s:%d (allocated at %s:%d, remaining: %zu)\r\n", 
              header->size, file, line, header->file, header->line, g_mem_stats.current_allocated);

    // Clear magic number to detect double-free
    header->magic = 0;

    // Free the original allocation (use system free)
    free((char*)ptr - HEADER_SIZE);
}

mem_stats_t* get_mem_stats(void) {
    return &g_mem_stats;
}

void print_mem_stats(void) {
    printf("\n=== Memory Statistics ===\n");
    printf("Dynamic allocations:\n");
    printf("  Total allocated:     %zu bytes (%.2f KB)\n", 
           g_mem_stats.total_allocated, g_mem_stats.total_allocated / 1024.0);
    printf("  Currently allocated: %zu bytes (%.2f KB)\n", 
           g_mem_stats.current_allocated, g_mem_stats.current_allocated / 1024.0);
    printf("  Peak allocated:      %zu bytes (%.2f KB)\n", 
           g_mem_stats.peak_allocated, g_mem_stats.peak_allocated / 1024.0);
    printf("  Allocation count:    %zu\n", g_mem_stats.allocation_count);
    printf("  Free count:          %zu\n", g_mem_stats.free_count);
    printf("  Leaked allocations:  %zu\n", g_mem_stats.allocation_count - g_mem_stats.free_count);
    
    printf("Static buffers:\n");
    printf("  Total static size:   %zu bytes (%.2f KB)\n", 
           g_mem_stats.static_buffer_size, g_mem_stats.static_buffer_size / 1024.0);
    
    printf("Total memory usage:    %zu bytes (%.2f KB)\n", 
           g_mem_stats.current_allocated + g_mem_stats.static_buffer_size,
           (g_mem_stats.current_allocated + g_mem_stats.static_buffer_size) / 1024.0);
    printf("========================\n\n");
}

void add_static_buffer_size(size_t size, const char* name) {
    g_mem_stats.static_buffer_size += size;
    log_debug("STATIC BUFFER: %s = %zu bytes (total static: %zu)", 
              name, size, g_mem_stats.static_buffer_size);
}

void reset_mem_stats(void) {
    memset(&g_mem_stats, 0, sizeof(g_mem_stats));
}

// Function pointer compatible wrappers (without file/line info)
void* tracked_malloc_simple(size_t size) {
    return tracked_malloc(size, "unknown", 0);
}

void tracked_free_simple(void* ptr) {
    tracked_free(ptr, "unknown", 0);
}
