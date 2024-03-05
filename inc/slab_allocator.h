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
    CSlabData*       m_next;
    CSlabData*       m_prev;
    SlabState        m_state;
    int              m_freeBlocksCount;
} CSlabData;

// Contains all data about current cache
typedef struct SCache
{
    CSlabData*  m_freeSlabs;
    CSlabData*  m_fullSlabs;
    CSlabData*  m_partlyFullSlabs;

    size_t      object_size;  /* allocating object size */
    size_t      slab_objects; /* count of objects in one SLAB */ 
    int         slab_order;   /* slab order size (i.e. (2^order * 4096)) SLAB */
    int         slabSize;     /* slab size after applying the formula above */
} Cache;

// Function returns all free slabs to system
void cache_shrink(Cache *cache);
// Return all memory from cache to system
void cache_release(Cache *cache);
// Returns memory back in cache
void cache_free(Cache *cache, void *ptr);
// Allocates memory (return >= object_size) from cache
void* cache_alloc(Cache *cache);
// Set up cache for forward usages
void cache_setup(Cache* cache, size_t object_size);