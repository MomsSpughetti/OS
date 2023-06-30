
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

MallocMetadata* malloc_meta_head = NULL;
memoryInfo mm_info{0,0,0,0};

typedef struct MallocMetadata
{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
} MallocMetadata;

typedef struct memoryInfo
{
    int freeBlocksCount; //done
    size_t totalFreeBytes;//
    int totalBlocksCount;
    size_t totaBytes;
} memoryInfo;

void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _num_meta_data_bytes();
size_t _size_meta_data();