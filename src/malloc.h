#pragma once
#include <cstddef> // for size_t

void custom_malloc_init();
void* custom_malloc(size_t size);
void custom_free(void* address);
