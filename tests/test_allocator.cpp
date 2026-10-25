
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdio>
#include "../src/slab_allocator.h"
#include "../src/malloc.h"

void test_basic_alloc_free() {
	printf("  [TEST] Basic alloc/free... ");

	custom_malloc_init();

	// Allocate 16 bytes
	void* ptr = custom_malloc(16);
	assert(ptr != nullptr);

	// Write to it (should not crash)
	memset(ptr, 0xAA, 16);

	// Free it
	custom_free(ptr);

	printf("PASSED\n");
}

void test_multiple_allocs_different_addresses() {
	printf("  [TEST] Multiple allocs return different addresses... ");

	custom_malloc_init();

	void* a = custom_malloc(16);
	void* b = custom_malloc(16);
	void* c = custom_malloc(16);

	assert(a != nullptr);
	assert(b != nullptr);
	assert(c != nullptr);
	assert(a != b);
	assert(b != c);
	assert(a != c);

	custom_free(a);
	custom_free(b);
	custom_free(c);

	printf("PASSED\n");
}

void test_zero_size_returns_null() {
	printf("  [TEST] Zero-size alloc returns nullptr... ");

	custom_malloc_init();

	void* ptr = custom_malloc(0);
	assert(ptr == nullptr);

	printf("PASSED\n");
}

void test_oversized_returns_null() {
	printf("  [TEST] Oversized alloc returns nullptr... ");

	custom_malloc_init();

	// Our allocator maxes out at 0x500 (1280 bytes)
	void* ptr = custom_malloc(0x600);
	assert(ptr == nullptr);

	printf("PASSED\n");
}

void test_many_allocations() {
	printf("  [TEST] Many allocations (slab chaining)... ");

	custom_malloc_init();

	const int N = 200;
	void* ptrs[N];

	// Allocate more blocks than a single slab can hold
	for (int i = 0; i < N; i++) {
		ptrs[i] = custom_malloc(64);
		assert(ptrs[i] != nullptr);
		memset(ptrs[i], 0xBB, 64);
	}

	// Free them all
	for (int i = 0; i < N; i++) {
		custom_free(ptrs[i]);
	}

	printf("PASSED\n");
}

void test_different_sizes() {
	printf("  [TEST] Different size classes... ");

	custom_malloc_init();

	void* p1 = custom_malloc(10);   // → slab_0x10
	void* p2 = custom_malloc(30);   // → slab_0x20
	void* p3 = custom_malloc(100);  // → slab_0x80
	void* p4 = custom_malloc(500);  // → slab_0x300

	assert(p1 != nullptr);
	assert(p2 != nullptr);
	assert(p3 != nullptr);
	assert(p4 != nullptr);

	// Write to verify usability
	memset(p1, 0x11, 10);
	memset(p2, 0x22, 30);
	memset(p3, 0x33, 100);
	memset(p4, 0x44, 500);

	custom_free(p1);
	custom_free(p2);
	custom_free(p3);
	custom_free(p4);

	printf("PASSED\n");
}

void test_alloc_free_realloc() {
	printf("  [TEST] Alloc-free-realloc cycle... ");

	custom_malloc_init();

	void* ptr1 = custom_malloc(32);
	assert(ptr1 != nullptr);
	memset(ptr1, 0xFF, 32);
	custom_free(ptr1);

	// After freeing, allocating the same size should succeed
	void* ptr2 = custom_malloc(32);
	assert(ptr2 != nullptr);
	memset(ptr2, 0xEE, 32);
	custom_free(ptr2);

	printf("PASSED\n");
}

int main() {
	printf("=== Slab Allocator Tests ===\n\n");

	test_basic_alloc_free();
	test_multiple_allocs_different_addresses();
	test_zero_size_returns_null();
	test_oversized_returns_null();
	test_many_allocations();
	test_different_sizes();
	test_alloc_free_realloc();

	printf("\n=== ALL TESTS PASSED ===\n");
	return 0;
}
