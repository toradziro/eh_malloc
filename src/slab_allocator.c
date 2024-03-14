#include <slab_allocator.h>
#define _GNU_SOURCE
#include <sys/mman.h>
#undef _GNU_SOURCE
#include <stdio.h>

#define trace printf("File: %s --- Function: %s --- Line: %d\n", __FILE__, __FUNCTION__, __LINE__);

typedef unsigned char byte;

const int sizeOfPage = 4096;
const int maxPossibleOrder = 10;
const int minObjectCount = 100;

//-- FD for funcs used by cache API
int countFullSlabMinimumSize(int sizeObject);
int countPossibleCountOfObjectsInSlab(int orderToPageSize, int objectSize);
static void* getFreeBlockFromFreeSlab(Cache *cache);
static void* getFreeBlockFromPartlyFullSlab(Cache *cache);
static void initNewFreeSlab(Cache* cache);
static void moveSlab(Cache* cache, CSlabData* pos, SlabState whereToMove, SlabState fromMoved);
static void letTheSlabGo(Cache* cache, SlabState stateToFree);
static int countSlabs(Cache* cache, SlabState stateToCount);

//-- Cache API goes here
//-- Set up cache for forward usages
void cacheSetup(Cache* cache, size_t object_size)
{
    cache->m_objectSize = object_size;
    cache->m_freeSlabs = NULL;
    cache->m_fullSlabs = NULL;
    cache->m_partlyFullSlabs = NULL;

    int minimumSlabSizeAcceptable = countFullSlabMinimumSize(object_size);
    for(int i = 0; i <= maxPossibleOrder; ++i)
    {
        int currentOrderToPageSize = (1UL << i) * sizeOfPage;
        if((minimumSlabSizeAcceptable <= currentOrderToPageSize) || i == maxPossibleOrder)
        {
            cache->m_slabOrder = i;
            cache->m_slabSize = currentOrderToPageSize;
            cache->m_slabObjects = countPossibleCountOfObjectsInSlab(currentOrderToPageSize, cache->m_objectSize);
            return;
        }
    }
}

//-- Allocates memory (return >= object_size) from cache
void* cacheAlloc(Cache* cache)
{
    if(cache->m_partlyFullSlabs != NULL)
    {
        return getFreeBlockFromPartlyFullSlab(cache);
    }
    else if(cache->m_freeSlabs != NULL)
    {
        //-- take first free slab, return first block, move slab to m_partlyFullSlabs
        return getFreeBlockFromFreeSlab(cache);
    }
    else
    {
        //-- allocate new free slab, take one pice and move new allocated slab to m_partlyFullSlabs
        initNewFreeSlab(cache);
        return getFreeBlockFromFreeSlab(cache);
    }
}

//-- Returns memory back in cache
void cacheFree(Cache* cache, void *ptr)
{
    trace
    CSlabData* currentSlab = (CSlabData*)((size_t)ptr & ~((1UL << cache->m_slabOrder) * sizeOfPage - 1));
    if(currentSlab == NULL)
    {
        printf("NULL\n");
    }
    else
    {
        printf("%p\n", ptr);
    }
    trace
    ++currentSlab->m_freeBlocksCount;
    trace
    if(currentSlab->m_freeBlocksCount == 1)
    {
        trace
        currentSlab->m_state = SS_PartlyFull;
        trace
        moveSlab(cache, currentSlab, SS_PartlyFull, SS_Full);
        trace
    }
    trace
    if((size_t)currentSlab->m_freeBlocksCount == cache->m_slabObjects)
    {
        trace
        currentSlab->m_state = SS_Free;
        trace
        moveSlab(cache, currentSlab, SS_Free, SS_PartlyFull);
        trace
    }
    trace
    //-- If we collected more than one free slab - automatically clean to avoid too much memory wasting
    if(countSlabs(cache, SS_Free) > 1)
    {
        trace
        cacheShrink(cache);
        trace
    }
}

//-- Return all memory from cache to system
void cacheRelease(Cache* cache)
{
    letTheSlabGo(cache, SS_Free);
    letTheSlabGo(cache, SS_Full);
    letTheSlabGo(cache, SS_PartlyFull);
}

//-- Function returns all free slabs to system
void cacheShrink(Cache* cache)
{
    letTheSlabGo(cache, SS_Free);
}

bool hasAddressInSlab(void* address, CSlabData* iterator, int slabSize)
{
    while(iterator->m_next != NULL)
    {
        if((void*)(iterator) <= address || (void*)((byte*)(iterator) + slabSize) >= address)
        {
            return true;
        }
        iterator = iterator->m_next;
    }
    return false;
}

bool hasAddressInCache(void* address, Cache* cache)
{
    CSlabData* iterator = cache->m_fullSlabs;
    if(hasAddressInSlab(address, iterator, cache->m_slabSize))
    {
        return true;
    }
    iterator = cache->m_partlyFullSlabs;
    if(hasAddressInSlab(address, iterator, cache->m_slabSize))
    {
        return true;
    }
    return false;
}

//-- Utilites and conf data
void safe_memset(void* data, int c, size_t size)
{
	volatile byte *p = (byte*)data;
	for (; size > 0; size--)
    {
		*p++ = (byte)c;
    }
}

void oneByteShift(size_t* order)
{
    *order = *order << 1;
}

int countFullSlabMinimumSize(int sizeObject)
{
    return (minObjectCount * (sizeObject)) + sizeof(CSlabData);
}

int countPossibleCountOfObjectsInSlab(int orderToPageSize, int objectSize)
{
    return (orderToPageSize - sizeof(CSlabData)) / (objectSize);
}

//-- Allocation and deallocation functions
static void* allocSlab(int order)
{
    size_t slabSize = (size_t)(1UL << order) * sizeOfPage;
    return mmap(NULL, slabSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

static void freeSlab(void* slab, int order)
{
    //-- TODO: Chack ret val
    munmap(slab, ((1UL << order) * sizeOfPage));
}

//-- Inside cache utilites
static void initNewFreeSlab(Cache* cache)
{
    //-- allocate slab
    void* buffer = allocSlab(cache->m_slabOrder);
    CSlabData* freeSlab = (CSlabData*)buffer;
    
    //-- initialize slab itself
    freeSlab->m_next = NULL;
    freeSlab->m_prev = NULL;
    freeSlab->m_freeBlocksCount = cache->m_slabObjects;
    freeSlab->m_state = SS_Free;

    cache->m_freeSlabs = freeSlab;
}

static CSlabData* getIteratorByState(Cache* cache, SlabState state)
{
    CSlabData* iterator = NULL;

    switch (state)
    {
    case SS_Free:
        iterator = cache->m_freeSlabs;
        break;
    case SS_PartlyFull:
        iterator = cache->m_partlyFullSlabs;
        break;
    case SS_Full:
        iterator = cache->m_fullSlabs;
        break;
    default:
        break;
    }

    return iterator;
}

static CSlabData** getListByState(Cache* cache, SlabState state)
{
    CSlabData** iterator = NULL;

    switch (state)
    {
    case SS_Free:
        iterator = &cache->m_freeSlabs;
        break;
    case SS_PartlyFull:
        iterator = &cache->m_partlyFullSlabs;
        break;
    case SS_Full:
        iterator = &cache->m_fullSlabs;
        break;
    default:
        break;
    }

    return iterator;
}

static bool isSlabAHead(Cache* cache, CSlabData* pos)
{
    return pos == cache->m_freeSlabs || pos == cache->m_partlyFullSlabs || pos == cache->m_fullSlabs;
}

static void moveSlab(Cache* cache, CSlabData* pos, SlabState whereToMove, SlabState fromMoved)
{
    if(pos && isSlabAHead(cache, pos))
    {
        CSlabData** slab = getListByState(cache, fromMoved);
        *slab = pos->m_next;
    }

    if(pos->m_prev)
    {
        pos->m_prev->m_next = pos->m_next;
    }

    if(pos->m_next)
    {
        pos->m_next->m_prev = pos->m_prev;
    }

    CSlabData** iterator = getListByState(cache, whereToMove);

    pos->m_prev = NULL;
    pos->m_next = *iterator;
    if(*iterator)
    {
        (*iterator)->m_prev = pos;
    }
    (*getListByState(cache, whereToMove)) = pos;
}

static void* getFreeBlockFromFreeSlab(Cache *cache)
{
    CSlabData* currentSlab = cache->m_freeSlabs;
    void* retPointer = (void*)((byte*)(currentSlab) + sizeof(CSlabData));
    
    --currentSlab->m_freeBlocksCount;
    currentSlab->m_state = SS_PartlyFull;

    moveSlab(cache, currentSlab, SS_PartlyFull, SS_Free);

    return retPointer;
}

static void* getFreeBlockFromPartlyFullSlab(Cache *cache)
{
    CSlabData* currentSlab = cache->m_partlyFullSlabs;

    void* retPointer = (void*)((byte*)(currentSlab) + sizeof(CSlabData) + ((cache->m_slabObjects - currentSlab->m_freeBlocksCount) * cache->m_objectSize));
    --currentSlab->m_freeBlocksCount;

    if(currentSlab->m_freeBlocksCount == 0)
    {
        currentSlab->m_state = SS_Full;
        moveSlab(cache, currentSlab, SS_Full, SS_PartlyFull);
    }

    return retPointer;
}

static void letTheSlabGo(Cache* cache, SlabState stateToFree)
{
    CSlabData* next = NULL;
    CSlabData* iterator = getIteratorByState(cache, stateToFree);

    while(iterator)
    {
        next = iterator->m_next;
        freeSlab((void*)(iterator), cache->m_slabOrder);
        iterator = next;
    }
    
    (*getListByState(cache, stateToFree)) = NULL;
}

static int countSlabs(Cache* cache, SlabState stateToCount)
{
    int count = 0;
    CSlabData* countSlab = NULL;
    switch (stateToCount)
    {
    case SS_Free:
        countSlab = cache->m_freeSlabs;
        break;
    case SS_PartlyFull:
        countSlab = cache->m_partlyFullSlabs;
        break;
    case SS_Full:
        countSlab = cache->m_fullSlabs;
        break;
    default:
        break;
    }
    while(countSlab != NULL)
    {
        ++count;
        countSlab = countSlab->m_next;
    }
    return count;
}
