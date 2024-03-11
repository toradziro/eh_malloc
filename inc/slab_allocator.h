#pragma once

#include <stddef.h>

typedef enum ESlabState
{
    SS_Free,
    SS_PartlyFull,
    SS_Full
} SlabState;

typedef struct SCSlabData
{
    struct SCSlabData*  m_next;
    struct SCSlabData*  m_prev;
    SlabState           m_state;
    int                 m_freeBlocksCount;
} CSlabData;

// Contains all data about current cache
typedef struct SCache
{
    CSlabData*  m_freeSlabs;
    CSlabData*  m_fullSlabs;
    CSlabData*  m_partlyFullSlabs;

    size_t      m_objectSize;  /* allocating object size */
    size_t      m_slabObjects; /* count of objects in one SLAB */ 
    int         m_slabOrder;   /* slab order size (i.e. (2^order * 4096)) SLAB */
    int         m_slabSize;     /* slab size after applying the formula above */
} Cache;

// Set up cache for forward usages
void cacheSetup(Cache* cache, size_t object_size);
// Allocates memory (return >= object_size) from cache
void* cacheAlloc(Cache* cache);
// Function returns all free slabs to system
void cacheShrink(Cache* cache);
// Return all memory from cache to system
void cacheRelease(Cache* cache);
// Returns memory back in cache
void cacheFree(Cache* cache, void* ptr);
// Check if a pointer in the cache
bool hasAddressInCache(void* address, Cache* cache);