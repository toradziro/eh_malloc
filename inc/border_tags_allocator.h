#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct SBlockFooter
{
    int     m_blockSize;
    bool    m_isFree;
} BlockFooter;

typedef struct SBlockHeader
{
    int     m_blockSize;
    bool    m_isFree;
} BlockHeader;

typedef struct SHeap
{
    void*           m_buffer;
    BlockHeader*    m_firstBlock;
    BlockFooter*    m_lastFooter;
    int             m_bufferSize;
    int             m_freeSpace;
} BTagsHeap;

void setupBTagsAllocator(void *buf, size_t size, BTagsHeap* heap);
void* BTAlloc(size_t size, BTagsHeap* heap);
void BTFree(void* p, BTagsHeap* heap);