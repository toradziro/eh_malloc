#include <border_tags_allocator.h>
#include <stdint.h>
#include <stdbool.h>

typedef unsigned char byte;

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

const size_t headerFooterSize = sizeof(BlockFooter) + sizeof(BlockHeader);
const size_t headerSize = sizeof(BlockHeader);
const size_t footerSize = sizeof(BlockFooter);
const int minBlockSize = sizeof(char) + headerFooterSize;

inline int64_t getAvailableSpaceWithoutMarkers(size_t size)
{
    return size - headerFooterSize;
}

typedef struct SHeap
{
    void*           m_buffer;
    BlockHeader*    m_firstBlock;
    BlockFooter*    m_lastFooter;
    int             m_bufferSize;
    int             m_freeSpace;
} Heap;

Heap globalHeap = { NULL, NULL, 0, 0 };

void initHeap(void *buf, size_t size)
{
    globalHeap.m_buffer = buf;
    globalHeap.m_bufferSize = size;
    globalHeap.m_freeSpace = getAvailableSpaceWithoutMarkers(size);

    // initialize first header and footer which we will use to cut blocks from
    // here header goes
    globalHeap.m_firstBlock = (BlockHeader*)globalHeap.m_buffer;
    globalHeap.m_firstBlock->m_blockSize = globalHeap.m_freeSpace;
    globalHeap.m_firstBlock->m_isFree = true;
        
    // here goes footer
    globalHeap.m_lastFooter = (BlockFooter*)((byte*)(globalHeap.m_buffer) + (size - sizeof(BlockFooter)));
    globalHeap.m_lastFooter->m_blockSize = globalHeap.m_freeSpace;
    globalHeap.m_lastFooter->m_isFree = true;
}

inline void* blockHeaderShift(void* start)
{
    return (byte*)(start) + sizeof(BlockHeader);
}

inline void* blockFooterShift(void* start)
{
    return (byte*)(start) + sizeof(BlockFooter);
}

inline size_t transformToSizeWithTags(size_t size)
{
    return size + sizeof(BlockFooter) + sizeof(BlockHeader);
}

inline BlockHeader* getHeader(BlockFooter* footer)
{
    return (BlockHeader*)((byte*)(footer) - (footer->m_blockSize + headerSize));
}

inline BlockFooter* getFooter(BlockHeader* header)
{
    return (BlockFooter*)((byte*)(header) + (header->m_blockSize + headerSize));
}

inline BlockHeader* getNextBlock(BlockHeader* header)
{
    if(getFooter(header) == globalHeap.m_lastFooter)
    {
        return NULL;
    }
    return (BlockHeader*)((byte*)(getFooter(header)) + footerSize);
}

void setupBTagsAllocator(void *buf, size_t size)
{
    initHeap(buf, size);
}

// Preparing block for return, if it's too big, we will cut part of it to return
// and leave in allocator another part
void cutTheBlockToFit(BlockHeader* iterator, size_t requestedSize)
{
    size_t sizeToCut = requestedSize + headerFooterSize;
    // in case of new block is gonna be smaller, than size of (metdata + one_void_pointer_size) size we won't cut
    if((int64_t)(iterator->m_blockSize - sizeToCut) <= minBlockSize)
    {
        iterator->m_isFree = false;
        getFooter(iterator)->m_isFree = false;
        return;
    }

    size_t newBlockSize = iterator->m_blockSize - sizeToCut;

    // iterator is a pointer to an old block
    iterator->m_blockSize = requestedSize;
    iterator->m_isFree = false;

    // Setting up an old block
    BlockFooter* newFooter = getFooter(iterator);
    newFooter->m_isFree = false;
    newFooter->m_blockSize = requestedSize;

    // setting up new block
    BlockHeader* newBlock = (BlockHeader*)blockFooterShift((void*)newFooter);
    newBlock->m_isFree = true;
    newBlock->m_blockSize = newBlockSize;

    BlockFooter* currentFooter = getFooter(newBlock);
    currentFooter->m_blockSize = newBlockSize;
}

void defragmentationAlgorithm(BlockHeader* iterator)
{
    // join all previous blocks
    BlockFooter* currFooter = getFooter(iterator);
    while(iterator != globalHeap.m_firstBlock)
    {
        BlockFooter* prevFooter = (BlockFooter*)((byte*)iterator - headerSize);
        if(!prevFooter->m_isFree)
        {
            break;
        }

        iterator = getHeader(prevFooter);
        iterator->m_blockSize = ((byte*)currFooter - (byte*)iterator) - headerSize;
        currFooter->m_blockSize = iterator->m_blockSize;
    }

    // join all ahead blocks
    while(currFooter != globalHeap.m_lastFooter)
    {
        BlockHeader* nextHeader = (BlockHeader*)((byte*)currFooter + footerSize);
        if(!nextHeader->m_isFree)
        {
            break;
        }

        currFooter = getFooter(nextHeader);
        currFooter->m_blockSize = ((byte*)currFooter - (byte*)iterator) - headerSize;
        iterator->m_blockSize = currFooter->m_blockSize;
    }
}

// Allocation function
void* myalloc(size_t size)
{
    if(globalHeap.m_freeSpace < size)
    {
        return NULL;
    }
    
    BlockHeader* blockItepator = globalHeap.m_firstBlock;

    while(blockItepator != NULL)
    {
        if(blockItepator->m_isFree && blockItepator->m_blockSize >= size)
        {
            // prepare block for allocation, at least we have to mark it as used
            cutTheBlockToFit(blockItepator, size);
            return (void*)((byte*)(blockItepator) + sizeof(BlockHeader));
        }
        blockItepator = getNextBlock(blockItepator);
    }
    return NULL;
}

// Free function
void myfree(void* p)
{
    BlockHeader* header = (BlockHeader*)((byte*)(p) - sizeof(BlockHeader));
    header->m_isFree = true;
    BlockFooter* footer = (BlockFooter*)((byte*)(p) + header->m_blockSize);
    footer->m_isFree = true;
    defragmentationAlgorithm(header);
}