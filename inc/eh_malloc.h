#pragma once

#include <stddef.h>
#include <border_tags_allocator.h>
#include <slab_allocator.h>
#include <pthread.h>

#define trace cout << "Line: " << __LINE__ << endl;

typedef struct SBTagHeapsList
{
    BTagsHeap               m_heap;
    struct SBTagHeapsList*  m_next;
} BTagHeapsList;

typedef struct SGlobalHeap
{
    //-- Small cache objects - till 64 bytes
    Cache           m_cacheSmall;
    //-- Middle cache objects - from 64 till 512 bytes
    Cache           m_cacheMedium;
    //-- Big objects - 4096 - from 512 to 4096
    Cache           m_cacheBig;
    //-- Large objects - over 4096
    BTagHeapsList*  m_btHeaps;

    bool            m_onInit;
    pthread_mutex_t m_mutex;
} GlobalHeap;

void* eh_malloc(size_t size);
void eh_free(void* address);