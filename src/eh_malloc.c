#include <eh_malloc.h>
#define _GNU_SOURCE
#include <sys/mman.h>
#undef _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>

typedef unsigned char byte;

const size_t smallSlabSize = 64;
const size_t mediumSlabSize = 512;
const size_t bigSlabSize = 4096;
const int    sizeOfPage = 4096;
const int    initialOrderForBT = 5;

static void  initHeap(GlobalHeap* heap);
static void* allocInBT(size_t size, GlobalHeap* heap);
static void  freeInBT(void* address, GlobalHeap* heap);

static GlobalHeap* heapSingleton()
{
    static GlobalHeap heap = {
        .m_cacheSmall = {}, .m_cacheMedium = {}, .m_cacheBig = {}, .m_btHeaps = NULL, .m_onInit = true};
    return &heap;
}

//-- API for malloc and free
void* eh_malloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }
    GlobalHeap* heap = heapSingleton();
    pthread_mutex_lock(&heap->m_mutex);
    void* result = NULL;

    if (heap->m_onInit)
    {
        initHeap(heap);
    }
    if (size <= smallSlabSize)
    {
        result = cacheAlloc(&heap->m_cacheSmall);
    }
    else if (size <= mediumSlabSize)
    {
        result = cacheAlloc(&heap->m_cacheMedium);
    }
    else if (size <= bigSlabSize)
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
    if (address == NULL)
    {
        return;
    }
    GlobalHeap* heap = heapSingleton();
    pthread_mutex_lock(&heap->m_mutex);
    if (hasAddressInCache(address, &heap->m_cacheSmall))
    {
        cacheFree(&heap->m_cacheSmall, address);
    }
    else if (hasAddressInCache(address, &heap->m_cacheMedium))
    {
        cacheFree(&heap->m_cacheMedium, address);
    }
    else if (hasAddressInCache(address, &heap->m_cacheBig))
    {
        cacheFree(&heap->m_cacheBig, address);
    }
    else
    {
        freeInBT(address, heap);
    }
    pthread_mutex_unlock(&heap->m_mutex);
}

//-- Initialization of global heap
static void initHeap(GlobalHeap* heap)
{
    pthread_mutex_init(&heap->m_mutex, NULL);
    pthread_mutex_lock(&heap->m_mutex);

    cacheSetup(&heap->m_cacheSmall, smallSlabSize);
    cacheSetup(&heap->m_cacheMedium, mediumSlabSize);
    cacheSetup(&heap->m_cacheBig, bigSlabSize);

    size_t sizeForBt = sizeOfPage * (1UL << initialOrderForBT);
    heap->m_btHeaps = mmapWrapperForBT(sizeForBt);

    size_t bufferSize = sizeForBt - sizeof(BTagHeapsList) - sizeof(BTagHeapsList*) - sizeof(BTagsHeap);
    byte*  addressOfBuffer = ((byte*)heap->m_btHeaps) + sizeof(BTagsHeap) + sizeof(BTagHeapsList*);
    setupBTagsAllocator(addressOfBuffer, bufferSize, &heap->m_btHeaps->m_heap);

    heap->m_onInit = false;

    pthread_mutex_unlock(&heap->m_mutex);
}

//-- Operations with BTAllocator
static void* mmapWrapperForBT(size_t size)
{
    size_t sizeForBT = size + sizeof(BTagHeapsList) + sizeof(BTagHeapsList*) + sizeof(BTagsHeap) + sizeof(BlockFooter) +
                       sizeof(BlockHeader);
    return mmap(NULL, sizeForBT, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

inline static void* calculateAddresOfBuffer(BTagHeapsList* newBTNode)
{
    return ((byte*)newBTNode) + sizeof(BTagsHeap) + sizeof(BTagHeapsList*);
}

inline static size_t getSizeWithBTMarkers(size_t size)
{
    return size + sizeof(BlockFooter) + sizeof(BlockHeader);
}

inline static size_t getSizeWithoutBTMarkers(size_t size)
{
    return size - sizeof(BlockFooter) - sizeof(BlockHeader);
}

static void* allocInBT(size_t size, GlobalHeap* heap)
{
    BTagHeapsList* iterator = heap->m_btHeaps;
    size_t         initialBTSize = sizeOfPage * (1UL << initialOrderForBT);
    size_t         bufferSize = getSizeWithBTMarkers(size >= initialBTSize ? size : initialBTSize);

    while (iterator != NULL && iterator->m_next != NULL)
    {
        if ((size_t)(iterator->m_heap.m_freeSpace) >= getSizeWithBTMarkers(size))
        {
            return BTAlloc(size, &iterator->m_heap);
        }
        iterator = iterator->m_next;
    }
    if (iterator == NULL)
    {
        heap->m_btHeaps = mmapWrapperForBT(bufferSize);
        iterator = heap->m_btHeaps;
    }
    else
    {
        iterator->m_next = mmapWrapperForBT(bufferSize);
        iterator = iterator->m_next;
    }

    iterator->m_next = NULL;

    setupBTagsAllocator(calculateAddresOfBuffer(iterator), bufferSize, &iterator->m_heap);

    return BTAlloc(size, &iterator->m_heap);
}

static void freeInBT(void* address, GlobalHeap* heap)
{
    BTagHeapsList* iterator = heap->m_btHeaps;
    if (iterator == NULL)
    {
        return;
    }
    size_t sizeForFree =
        iterator->m_heap.m_bufferSize + sizeof(BTagHeapsList) + sizeof(BTagHeapsList*) + sizeof(BTagsHeap);

    BTagHeapsList* prevElem = iterator;
    while (iterator != NULL)
    {
        if (address >= (void*)(iterator) && address <= ((void*)((byte*)(iterator) + iterator->m_heap.m_bufferSize)))
        {
            BTFree(address, &iterator->m_heap);

            if (iterator != heap->m_btHeaps &&
                getSizeWithoutBTMarkers(iterator->m_heap.m_bufferSize) == iterator->m_heap.m_freeSpace)
            {
                prevElem->m_next = iterator->m_next;
                munmap((void*)(iterator), sizeForFree);
            }
            break;
        }
        prevElem = iterator;
        iterator = iterator->m_next;
    }
}

//-- Dump Allocator Data
static void dumpCache(Cache* cache)
{
    CSlabData* iterator = cache->m_freeSlabs;
    while (iterator != NULL)
    {
        printf("Free slab: %p\n", iterator);
        printf("Free Blocks: %d\n", iterator->m_freeBlocksCount);
        iterator = iterator->m_next;
    }
    iterator = cache->m_partlyFullSlabs;
    while (iterator != NULL)
    {
        printf("Partly full slab: %p\n", iterator);
        printf("Free Blocks: %d\n", iterator->m_freeBlocksCount);
        iterator = iterator->m_next;
    }
    iterator = cache->m_fullSlabs;
    while (iterator != NULL)
    {
        printf("Full slab: %p\n", iterator);
        printf("Free Blocks: %d\n", iterator->m_freeBlocksCount);
        iterator = iterator->m_next;
    }
}

static void dumpBTagsAllocator(BTagHeapsList* heap)
{
    printf("Free space: %ld\n", heap->m_heap.m_freeSpace);
    printf("Buffer size: %ld\n", heap->m_heap.m_bufferSize);
}

void dumpHeap()
{
    GlobalHeap* heap = heapSingleton();
    pthread_mutex_lock(&heap->m_mutex);
    printf("-----Small cache------\n");
    dumpCache(&heap->m_cacheSmall);
    printf("------Medium cache------\n");
    dumpCache(&heap->m_cacheMedium);
    printf("------Big cache------\n");
    dumpCache(&heap->m_cacheBig);
    BTagHeapsList* iterator = heap->m_btHeaps;
    while (iterator != NULL)
    {
        printf("------BT Heap------\n");
        dumpBTagsAllocator(iterator);
        iterator = iterator->m_next;
    }
    pthread_mutex_unlock(&heap->m_mutex);
}