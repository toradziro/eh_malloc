#include <eh_malloc.h>
#include <sys/mman.h>

const size_t smallSlabSize = 64;
const size_t mediumSlabSize = 512;
const size_t bigSlabSize = 4096;

static void initHeap(GlobalHeap* heap);

static GlobalHeap* heapSingleton()
{
    static GlobalHeap heap = { 0, 0, 0, NULL, .m_onInit = true };
    return &heap;
}

void* eh_malloc(size_t size)
{
    GlobalHeap* heap = heapSingleton();
    if(heap->m_onInit)
    {
        initHeap(heap);
    }
}

static void initHeap(GlobalHeap* heap)
{
    cache_setup(&heap->m_cacheSmall, smallSlabSize);
    cache_setup(&heap->m_cacheMedium, mediumSlabSize);
    cache_setup(&heap->m_cacheBig, bigSlabSize);
    heap->m_btHeaps = NULL;
    heap->m_onInit = false;
}