#include <slab_allocator.h>
#include <stdbool.h>
#define _GNU_SOURCE
#include <sys/mman.h>

typedef unsigned char byte;

void safe_memset(void *data, int c, size_t size)
{
	volatile byte *p = (byte*)data;
	for (; size > 0; size--)
    {
		*p++ = (byte)c;
    }
}

const int sizeOfPage = 4096;
const int maxPossibleOrder = 10;
const int minObjectCount = 100;

inline void oneByteShift(size_t* order)
{
    *order = *order << 1;
}

inline int countFullSlabMinimumSize(int sizeObject)
{
    return (minObjectCount * (sizeObject)) + sizeof(CSlabData);
}

inline int countPossibleCountOfObjectsInSlab(int orderToPageSize, int objectSize)
{
    return (orderToPageSize - sizeof(CSlabData)) / (objectSize);
}

void* allocSlab(int order)
{
    return mmap(NULL, ((2 << order) * sizeOfPage), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

void freeSlab(void* slab, int order)
{
    //-- TODO: Chack ret val
    munmap(slab, ((2 << order) * sizeOfPage));
}

// Set up cache for forward usages
void cache_setup(Cache* cache, size_t object_size)
{
    cache->object_size = object_size;
    cache->m_freeSlabs = NULL;
    cache->m_fullSlabs = NULL;
    cache->m_partlyFullSlabs = NULL;

    int minimumSlabSizeAcceptable = countFullSlabMinimumSize(object_size);
    for(int i = 0; i <= maxPossibleOrder; ++i)
    {
        int currentOrderToPageSize = (1UL << i) * sizeOfPage;
        if((minimumSlabSizeAcceptable <= currentOrderToPageSize) || i == maxPossibleOrder)
        {
            cache->slab_order = i;
            cache->slabSize = currentOrderToPageSize;
            cache->slab_objects = countPossibleCountOfObjectsInSlab(currentOrderToPageSize, cache->object_size);
            return;
        }
    }
}

void initNewFreeSlab(Cache* cache)
{
    // allocate slab
    void* buffer = allocSlab(cache->slab_order);
    CSlabData* freeSlab = (CSlabData*)buffer;
    
    // initialize slab itself
    freeSlab->m_next = NULL;
    freeSlab->m_prev = NULL;
    freeSlab->m_freeBlocksCount = cache->slab_objects;
    freeSlab->m_state = SS_Free;

    cache->m_freeSlabs = freeSlab;
}

CSlabData* getIteratorByState(Cache* cache, SlabState state)
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

CSlabData** getListByState(Cache* cache, SlabState state)
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

bool isSlabAHead(Cache* cache, CSlabData* pos)
{
    return pos == cache->m_freeSlabs || pos == cache->m_partlyFullSlabs || pos == cache->m_fullSlabs;
}

void moveSlab(Cache* cache, CSlabData* pos, SlabState whereToMove, SlabState fromMoved)
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

void* getFreeBlockFromFreeSlab(Cache *cache)
{
    CSlabData* currentSlab = cache->m_freeSlabs;
    void* retPointer = (void*)((byte*)(currentSlab) + sizeof(CSlabData));
    
    --currentSlab->m_freeBlocksCount;
    currentSlab->m_state = SS_PartlyFull;

    moveSlab(cache, currentSlab, SS_PartlyFull, SS_Free);

    return retPointer;
}

void* getFreeBlockFromPartlyFullSlab(Cache *cache)
{
    CSlabData* currentSlab = cache->m_partlyFullSlabs;

    void* retPointer = (void*)((byte*)(currentSlab) + sizeof(CSlabData) + ((cache->slab_objects - currentSlab->m_freeBlocksCount) * cache->object_size));
    --currentSlab->m_freeBlocksCount;

    if(currentSlab->m_freeBlocksCount == 0)
    {
        currentSlab->m_state = SS_Full;
        moveSlab(cache, currentSlab, SS_Full, SS_PartlyFull);
    }

    return retPointer;
}

// Allocates memory (return >= object_size) from cache
void *cache_alloc(Cache* cache)
{
    if(cache->m_partlyFullSlabs != NULL)
    {
        return getFreeBlockFromPartlyFullSlab(cache);
    }
    else if(cache->m_freeSlabs != NULL)
    {
        // take first free slab, return first block, move slab to m_partlyFullSlabs
        return getFreeBlockFromFreeSlab(cache);
    }
    else
    {
        // allocate new free slab, take one pice and move new allocated slab to m_partlyFullSlabs
        initNewFreeSlab(cache);
        return getFreeBlockFromFreeSlab(cache);
    }
}

// Returns memory back in cache
void cache_free(Cache* cache, void *ptr)
{
    CSlabData* currentSlab = (CSlabData*)((size_t)ptr & ~((1UL << cache->slab_order) * 4096 - 1));
    ++currentSlab->m_freeBlocksCount;
    if(currentSlab->m_freeBlocksCount == 1)
    {
        currentSlab->m_state = SS_PartlyFull;
        moveSlab(cache, currentSlab, SS_PartlyFull, SS_Full);
    }
    if(currentSlab->m_freeBlocksCount == cache->slab_objects)
    {
        currentSlab->m_state = SS_Free;
        moveSlab(cache, currentSlab, SS_Free, SS_PartlyFull);
    }
}

void letTheSlabGo(Cache* cache, SlabState stateToFree)
{
    CSlabData* next = NULL;
    CSlabData* iterator = getIteratorByState(cache, stateToFree);

    while(iterator)
    {
        next = iterator->m_next;
        freeSlab((void*)(iterator), cache->slab_order);
        iterator = next;
    }
    
    (*getListByState(cache, stateToFree)) = NULL;
}

// Return all memory from cache to system
void cache_release(Cache* cache)
{
    letTheSlabGo(cache, SS_Free);
    letTheSlabGo(cache, SS_Full);
    letTheSlabGo(cache, SS_PartlyFull);
}

// Function returns all free slabs to system
void cache_shrink(Cache* cache)
{
    letTheSlabGo(cache, SS_Free);
}