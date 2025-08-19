#ifndef MEM_TRACK_H
#define MEM_TRACK_H

#include <stddef.h>
#include <stdint.h>

// Memory tracking statistics
typedef struct {
    size_t total_allocated;      // Total bytes allocated dynamically
    size_t current_allocated;    // Currently allocated bytes dynamically
    size_t peak_allocated;       // Peak dynamic allocation
    size_t allocation_count;     // Number of dynamic allocations
    size_t free_count;           // Number of frees
    size_t static_buffer_size;   // Total size of static buffers
} mem_stats_t;

// Memory tracking functions
void* tracked_malloc(size_t size, const char* file, int line);
void* tracked_calloc(size_t nmemb, size_t size, const char* file, int line);
void* tracked_realloc(void* ptr, size_t size, const char* file, int line);
void tracked_free(void* ptr, const char* file, int line);

// Function pointer compatible wrappers (without file/line info)
void* tracked_malloc_simple(size_t size);
void tracked_free_simple(void* ptr);

// Statistics functions
mem_stats_t* get_mem_stats(void);
void print_mem_stats(void);
void reset_mem_stats(void);
void add_static_buffer_size(size_t size, const char* name);

// Convenience macros for when memory tracking is explicitly wanted
#ifdef ENABLE_MEM_TRACKING
#define MALLOC_TRACKED(size) tracked_malloc(size, __FILE__, __LINE__)
#define CALLOC_TRACKED(nmemb, size) tracked_calloc(nmemb, size, __FILE__, __LINE__)
#define REALLOC_TRACKED(ptr, size) tracked_realloc(ptr, size, __FILE__, __LINE__)
#define FREE_TRACKED(ptr) tracked_free(ptr, __FILE__, __LINE__)
#define REGISTER_STATIC_BUFFER(buffer, name) add_static_buffer_size(sizeof(buffer), name)
#else
#define MALLOC_TRACKED(size) malloc(size)
#define CALLOC_TRACKED(nmemb, size) calloc(nmemb, size)
#define REALLOC_TRACKED(ptr, size) realloc(ptr, size)
#define FREE_TRACKED(ptr) free(ptr)
#define REGISTER_STATIC_BUFFER(buffer, name) ((void)0)
#endif

#endif // MEM_TRACK_H
