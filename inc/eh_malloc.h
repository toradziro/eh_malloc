#pragma once

#include <stddef.h>
#include <border_tags_allocator.h>

#define trace cout << "Line: " << __LINE__ << endl;

typedef struct SGlobalHeap
{

} GlobalHeap;

void* eh_malloc(size_t size);