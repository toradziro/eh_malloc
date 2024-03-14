#include <eh_malloc.h>
#define _GNU_SOURCE
#include <sys/mman.h>
#undef _GNU_SOURCE
#include <stdio.h>
#include <stdint.h> 

typedef unsigned char byte;

const size_t smallSlabSize = 64;
const size_t mediumSlabSize = 512;
const size_t bigSlabSize = 4096;

static void initHeap(GlobalHeap* heap);
static void* allocInBT(size_t size, GlobalHeap* heap);
static void freeInBT(void* address, GlobalHeap* heap);

static GlobalHeap* heapSingleton()
{
    static GlobalHeap heap = { .m_cacheSmall = {}, .m_cacheMedium = {}, .m_cacheBig = {}, .m_btHeaps = NULL, .m_onInit = true };
    return &heap;
}

size_t align(size_t n, size_t round_up_to)
{
	return (n + round_up_to - 1) & ~(round_up_to - 1);
}

void* eh_malloc(size_t size)
{
    GlobalHeap* heap = heapSingleton();
    pthread_mutex_lock(&heap->m_mutex);
    void* result = NULL;

    size = align(size, sizeof(intptr_t));

    if(heap->m_onInit)
    {
        initHeap(heap);
    }
    if(size <= smallSlabSize)
    {
        result = cacheAlloc(&heap->m_cacheSmall);
    }
    else if(size <= mediumSlabSize)
    {
        result = cacheAlloc(&heap->m_cacheMedium);
    }
    else if(size <= bigSlabSize)
    {
        result = cacheAlloc(&heap->m_cacheBig);
    }
    else
    {
        result = allocInBT(size, heap);
    }
    pthread_mutex_unlock(&heap->m_mutex);
    return result;
}

void eh_free(void* address)
{
    if(address == NULL)
    {
        return;
    }
    trace
    GlobalHeap* heap = heapSingleton();
    trace
    pthread_mutex_lock(&heap->m_mutex);
    trace

    if(hasAddressInCache(address, &heap->m_cacheSmall))
    {
        trace
        cacheFree(&heap->m_cacheSmall, address);
        trace
    }
    else if(hasAddressInCache(address, &heap->m_cacheMedium))
    {
        trace
        cacheFree(&heap->m_cacheMedium, address);
        trace
    }
    else if(hasAddressInCache(address, &heap->m_cacheBig))
    {
        trace
        cacheFree(&heap->m_cacheBig, address);
        trace
    }
    else
    {
        trace
        freeInBT(address, heap);
        trace
    }
    pthread_mutex_unlock(&heap->m_mutex);
}

static void initHeap(GlobalHeap* heap)
{
    cacheSetup(&heap->m_cacheSmall, smallSlabSize);
    cacheSetup(&heap->m_cacheMedium, mediumSlabSize);
    cacheSetup(&heap->m_cacheBig, bigSlabSize);
    heap->m_btHeaps = NULL;
    heap->m_onInit = false;
    pthread_mutex_init(&heap->m_mutex, NULL);
}

static void* allocInBT(size_t size, GlobalHeap* heap)
{
    size_t sizeForBT = size + sizeof(BTagHeapsList) + sizeof(BTagHeapsList*) + sizeof(BTagsHeap) + sizeof(BlockFooter) + sizeof(BlockHeader);
    BTagHeapsList* newBTNode = (BTagHeapsList*)mmap(NULL,
            sizeForBT,
            PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_PRIVATE,
            -1, 0);

    if(newBTNode == NULL)
    {
        return NULL;
    }

    newBTNode->m_next = NULL;
    BTagHeapsList* iterator = heap->m_btHeaps;
    if(iterator == NULL)
    {
        iterator = newBTNode;
    }
    else
    {
        while(iterator->m_next != NULL)
        {
            iterator = iterator->m_next;
        }
        iterator->m_next = newBTNode;
    }

    byte* addressOfBuffer = ((byte*)newBTNode) + sizeof(BTagsHeap) + sizeof(BTagHeapsList*); 
    size_t bufferSize = size + sizeof(BlockFooter) + sizeof(BlockHeader);
    setupBTagsAllocator((void*)addressOfBuffer, bufferSize, &newBTNode->m_heap);
    return BTAlloc(size, &newBTNode->m_heap);
}

static void freeInBT(void* address, GlobalHeap* heap)
{
    BTagHeapsList* iterator = heap->m_btHeaps;
    if(iterator == NULL)
    {
        return;
    }
    size_t sizeForFree = iterator->m_heap.m_bufferSize + sizeof(BTagHeapsList) + sizeof(BTagHeapsList*) + sizeof(BTagsHeap);
    if(address >= (void*)(iterator) && address <= (void*)((byte*)(iterator) + iterator->m_heap.m_bufferSize))
    {
        heap->m_btHeaps = heap->m_btHeaps->m_next;
        munmap((void*)(iterator), sizeForFree);
    }

    BTagHeapsList* prevElem = NULL;
    while(iterator != NULL)
    {
        if(address >= (void*)(iterator) && address <= (void*)((byte*)(iterator) + iterator->m_heap.m_bufferSize))
        {
            prevElem->m_next = iterator->m_next;
            munmap((void*)(iterator), sizeForFree);
            break;
        }
        prevElem = iterator;
        iterator = iterator->m_next;
    }
}