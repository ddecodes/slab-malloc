
#pragma once
#include "bitmap.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
  #include <Windows.h>
#else
  #include <sys/mman.h>    // mmap, munmap
#endif

const static uint32_t MAGIC = 0x8CBEEFC8;

// Forward declaration: Slab is a template class defined below.
template<size_t slab_size, size_t memory_size> class Slab;


template<size_t slab_size, size_t memory_size, size_t max_blocks = memory_size / slab_size>
struct SlabHeader {
	uint32_t magic;                             // Integrity check value
	uint32_t size;                              // Block size this slab serves
	size_t free_blocks;                         // Number of free blocks remaining
	size_t next_fit_block;                      // Next-fit search starting position
	Slab<slab_size, memory_size>* prev, * next; // Linked list of slabs
	Bitmap<max_blocks> mem_map;                 // Bitmap tracking free/used blocks
};


template<size_t size>
struct MemoryBlock {
	uintptr_t slab_ptr;  // Points back to the owning Slab
	char data[size];      // The actual memory the user gets
};


template<size_t slab_size, size_t memory_size> class Slab {
private:
	// Calculate sizes at COMPILE TIME using constexpr/const:
	const static size_t MAX_HEADER_SIZE = sizeof(SlabHeader<slab_size, memory_size>);
	const static size_t MAX_BLOCKS = (memory_size - MAX_HEADER_SIZE) / sizeof(MemoryBlock<slab_size>);

	// Compile-time sanity checks:
	static_assert(memory_size > MAX_HEADER_SIZE);                                    // Slab must be bigger than its header
	static_assert((sizeof(MemoryBlock<slab_size>) + MAX_HEADER_SIZE) <= memory_size); // Must fit at least 1 block

	SlabHeader<slab_size, memory_size, MAX_BLOCKS> header;  // Metadata
	MemoryBlock<slab_size> blocks[MAX_BLOCKS];              // The actual memory blocks

	// --- Private helpers ---
	bool is_address_in_slab(void* address);
	void* alloc_in_current_slab(size_t block_index);
	void* alloc_in_new_slab();
	void free_from_current_slab(size_t block_index);
	void* request_memory_from_os(size_t size);
	void free_memory_to_os(void* addrss, size_t size);

public:
	void init(Slab* prev = nullptr);
	void* alloc();
	void free(void* address);
};

template<size_t slab_size, size_t memory_size>
void Slab<slab_size, memory_size>::init(Slab* prev) {
	header.magic = MAGIC;
	header.size = slab_size;
	header.prev = prev;
	header.next = nullptr;
	header.free_blocks = MAX_BLOCKS;
	header.next_fit_block = 0;
	header.mem_map.init();
}

template<size_t slab_size, size_t memory_size>
void* Slab<slab_size, memory_size>::alloc() {
	assert(header.magic == MAGIC);
	assert(header.size == slab_size);

	size_t block_index = -1;
	if (header.free_blocks &&
		((block_index = header.mem_map.find_unused(header.next_fit_block)) != BITMAP_NO_BITS_LEFT)) {
		return alloc_in_current_slab(block_index);
	} else if (header.next) {
		return header.next->alloc();
	} else {
		return alloc_in_new_slab();
	}

}

template<size_t slab_size, size_t memory_size>
void Slab<slab_size, memory_size>::free(void* address) {
	assert(header.magic == MAGIC);
	assert(header.size == slab_size);
	assert(is_address_in_slab(address));

	size_t block_index = (uintptr_t(address) - uintptr_t(blocks)) / sizeof(MemoryBlock<slab_size>);
	assert(header.mem_map.check_used(block_index));
	free_from_current_slab(block_index);
}

template<size_t slab_size, size_t memory_size>
bool Slab<slab_size, memory_size>::is_address_in_slab(void* address) {
	if ((address >= blocks) && (address <= &blocks[MAX_BLOCKS - 1].data[slab_size - 1])) {
		return true;
	} else {
		return false;
	}
	return true;
}

template<size_t slab_size, size_t memory_size>
void* Slab<slab_size, memory_size>::alloc_in_new_slab() {
	Slab* new_slab = static_cast<Slab*>(request_memory_from_os(sizeof(Slab)));
	if (!new_slab) {
		return nullptr;
	}
	new_slab->init(this);
	header.next = new_slab;
	return new_slab->alloc();
}

template<size_t slab_size, size_t memory_size>
void* Slab<slab_size, memory_size>::alloc_in_current_slab(size_t block_index) {
	header.mem_map.set_used(block_index);
	header.next_fit_block = (block_index + 1) % MAX_BLOCKS;
	header.free_blocks--;
	blocks[block_index].slab_ptr = uintptr_t(this);
	return static_cast<void*>(blocks[block_index].data);
}

template<size_t slab_size, size_t memory_size>
void Slab<slab_size, memory_size>::free_from_current_slab(size_t block_index) {
	header.mem_map.set_unused(block_index);
	header.next_fit_block = block_index;
	header.free_blocks++;

	if ((header.free_blocks == 0) && (header.prev)) {
		//slab is empty, and it's not the first;
		header.prev->header.next = nullptr;
		free_memory_to_os(this, sizeof(Slab));
		//The slab committed suicide, don't ever use it again!
	}
}

template<size_t slab_size, size_t memory_size>
void* Slab<slab_size, memory_size>::request_memory_from_os(size_t size) {
#ifdef _WIN32
	// Windows: Use VirtualAlloc
	void* address = VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
	if (!address) {
		printf("error: 0x%lX\n", GetLastError());
		assert(false);
	}
#else
	// macOS / Linux: Use mmap
	void* address = mmap(nullptr, size,
	                     PROT_READ | PROT_WRITE,
	                     MAP_PRIVATE | MAP_ANONYMOUS,
	                     -1, 0);
	if (address == MAP_FAILED) {
		perror("mmap failed");
		assert(false);
		return nullptr;
	}
#endif
	return address;
}


template<size_t slab_size, size_t memory_size>
void Slab<slab_size, memory_size>::free_memory_to_os(void* addrss, size_t size) {
#ifdef _WIN32
	VirtualFree(addrss, size, MEM_FREE);
#else
	munmap(addrss, size);
#endif
}
