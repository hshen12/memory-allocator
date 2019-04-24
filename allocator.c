/**
 * allocator.c
 *
 * Explores memory management at the C runtime level.
 *
 * Author: <your team members go here>
 *
 * To use (one specific command):
 * LD_PRELOAD=$(pwd)/allocator.so command
 * ('command' will run with your allocator)
 *
 * To use (all following commands):
 * export LD_PRELOAD=$(pwd)/allocator.so
 * (Everything after this point will use your custom allocator -- be careful!)
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>


#ifndef DEBUG
#define DEBUG 1
#endif

#define LOG(fmt, ...) \
do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
	__LINE__, __func__, __VA_ARGS__); } while (0)

#define LOGP(str) \
do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): %s", __FILE__, \
	__LINE__, __func__, str); } while (0)

struct mem_block {
    /* Each allocation is given a unique ID number. If an allocation is split in
     * two, then the resulting new block will be given a new ID. */
	unsigned long alloc_id;

    /* Size of the memory region */
	size_t size;

    /* Space used; if usage == 0, then the block has been freed. */
	size_t usage;

    /* Pointer to the start of the mapped memory region. This simplifies the
     * process of finding where memory mappings begin. */
	struct mem_block *region_start;

    /* If this block is the beginning of a mapped memory region, the region_size
     * member indicates the size of the mapping. In subsequent (split) blocks,
     * this is undefined. */
	size_t region_size;

    /* Next block in the chain */
	struct mem_block *next;
};

/* Start (head) of our linked list: */
struct mem_block *g_head = NULL;

/* Allocation counter: */
unsigned long g_allocations = 0;

pthread_mutex_t g_alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * print_memory
 *
 * Prints out the current memory state, including both the regions and blocks.
 * Entries are printed in order, so there is an implied link from the topmost
 * entry to the next, and so on.
 */
void print_memory(void)
{
	puts("-- Current Memory State --");
	struct mem_block *current_block = g_head;
	struct mem_block *current_region = NULL;
	while (current_block != NULL) {
		if (current_block->region_start != current_region) {
			current_region = current_block->region_start;
			printf("[REGION] %p-%p %zu\n",
				current_region,
				(void *) current_region + current_region->region_size,
				current_region->region_size);
		}
		printf("[BLOCK]  %p-%p (%ld) %zu %zu %zu\n",
			current_block,
			(void *) current_block + current_block->size,
			current_block->alloc_id,
			current_block->size,
			current_block->usage,
			current_block->usage == 0
			? 0 : current_block->usage - sizeof(struct mem_block));
		current_block = current_block->next;
	}
}

void *reuse(size_t size) {
	assert(g_head != NULL);

	char *algo = getenv("ALLOCATOR_ALGORITHM");
	if (algo == NULL) {
		algo = "first_fit";
	}

	void *ptr = NULL;
	if (strcmp(algo, "first_fit") == 0) {
		first_fit_algo(size);
	} else if (strcmp(algo, "best_fit") == 0) {
		best_fit_algo(size, ptr);
	} else if (strcmp(algo, "worst_fit") == 0) {
		worst_fit_algo(size, ptr);
	}







    // TODO: using free space management algorithms, find a block of memory that
    // we can reuse. Return NULL if no suitable block is found.
	return NULL;
}

void *malloc(size_t size)
{
    // TODO: allocate memory. You'll first check if you can reuse an existing
    // block. If not, map a new memory region.

	size_t real_sz = size+sizeof(struct mem_block);
	// LOG("read size is: %zu\n", real_sz);

	int page_sz = getpagesize();
	size_t num_pages = real_sz / page_sz;
	if(real_sz % page_sz != 0) {
		num_pages++;
	}
	size_t region_sz = num_pages * page_sz;

	// LOG("Allocated memo: %zu\n", size);
	struct mem_block *block = mmap(NULL, region_sz, 
		PROT_READ | PROT_WRITE, 
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if(block == MAP_FAILED) {
		perror("mmap");
		return NULL;
	}
	
	block->alloc_id = g_allocations++;
	block->size = region_sz;
	block->usage = real_sz;
	block->region_start = block;
	block->region_size = region_sz;
	block->next = NULL;

	if(g_head == NULL) {
		g_head = block;
	} else {
		// LOG("g_head is: %p\n", g_head);
		struct mem_block *curr = g_head;
		while(curr->next != NULL) {
			// LOG("curr is %p\n", curr);
			curr = curr->next;
			// LOG("curr is %p\n", curr);
		}
		// LOG("LINKING %p to %p\n", curr, block);
		curr->next = block;
	}

	return block+1;
}

void free(void *ptr)
{	
	print_memory();
	// LOGP("Entering free\n");
	if (ptr == NULL) {
        /* Freeing a NULL pointer does nothing */
		return;
	}
	int page_sz = getpagesize();
	struct mem_block *block = (struct mem_block*) ptr;
	if(block->usage == sizeof(struct mem_block)) {

		LOGP("here");
		int result = munmap(block, page_sz);
		if(result == -1) {
			perror("munmap");
		}
	}
    // TODO: free memory. If the containing region is empty (i.e., there are no
    // more blocks in use), then it should be unmapped.
	print_memory();
}

void *calloc(size_t nmemb, size_t size)
{
    // TODO: hmm, what does calloc do?
	return NULL;
}

void *realloc(void *ptr, size_t size)
{
	if (ptr == NULL) {
        /* If the pointer is NULL, then we simply malloc a new block */
		return malloc(size);
	}

	if (size == 0) {
        /* Realloc to 0 is often the same as freeing the memory block... But the
         * C standard doesn't require this. We will free the block and return
         * NULL here. */
		free(ptr);
		return NULL;
	}

    // TODO: reallocation logic

	return NULL;
}

