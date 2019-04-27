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
#include <stdlib.h>


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

void* first_fit_algo(size_t size) {

	struct mem_block *curr = g_head;
	while(curr != NULL) {

		if((curr->size-curr->usage) >= size) {
			return curr;
		}

		curr = curr->next;
	}
	return NULL;
}

void best_fit_algo(size_t size) {

	struct mem_block *re_block = NULL;
	size_t max = SIZE_MAX;
	
	struct mem_block *curr = g_head;
	while(curr != NULL) {
		size_t remain = curr->size-curr->usage;
		if(remain >= size) {
			if(remain == size) {
				return curr;
			}
			if(remain < max) {
				re_block = curr;
				max = remain;
			}
		}

		curr = curr->next;
	}
	return re_block;
	
}

void worst_fit_algo(size_t size, void *ptr) {

	struct mem_block *re_block = NULL;
	size_t min = 0;
	if(g_head == NULL) {
		return;
	} else {
		struct mem_block *curr = g_head;
		while(curr != NULL) {
			size_t remain = curr->size-curr->usage;
			if(remain >= size) {
				if(remain > min) {
					re_block = curr;
					min = remain;
				}
			}

			curr = curr->next;
		}
	}
	return re_block;
}

void *reuse(size_t size) {
	if(g_head == NULL) {
		return NULL;
	}

	char *algo = getenv("ALLOCATOR_ALGORITHM");
	if (algo == NULL) {
		algo = "first_fit";
	}

	void *ptr = NULL;
	if (strcmp(algo, "first_fit") == 0) {
		ptr = first_fit_algo(size);
	} else if (strcmp(algo, "best_fit") == 0) {
		ptr = best_fit_algo(size);
	} else if (strcmp(algo, "worst_fit") == 0) {
		ptr = worst_fit_algo(size);
	}

    // TODO: using free space management algorithms, find a block of memory that
    // we can reuse. Return NULL if no suitable block is found.
	return ptr;
}

void *malloc(size_t size)
{
    // TODO: allocate memory. You'll first check if you can reuse an existing
    // block. If not, map a new memory region.

	void *reuse_mem = reuse(size);

	if(reuse_mem == NULL) {

		size_t real_sz = size+sizeof(struct mem_block);

		int page_sz = getpagesize();
		size_t num_pages = real_sz / page_sz;
		if(real_sz % page_sz != 0) {
			num_pages++;
		}
		size_t region_sz = num_pages * page_sz;

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
			struct mem_block *curr = g_head;
			while(curr->next != NULL) {
				curr = curr->next;
			}
			curr->next = block;
			// curr->size = curr->usage;
		}

		return block+1;

	} else {
		(struct mem_block) reuse_mem;
		if (reuse_mem->usage == 0) {
			
			size_t real_sz = size+sizeof(struct mem_block);
			reuse_mem->usage = real_sz;






		} else {








		}
	}





}

void free(void *ptr)
{	
	if (ptr == NULL) {                                          
        /* Freeing a NULL pointer does nothing */               
		return;                                                 
	}                                                           

    /* Section 01: we didn't quite finish this below: (ptr -1) */                                                            
	struct mem_block *blk = (struct mem_block*) ptr - 1;        
	LOG("Free request; allocation = %lu\n", blk->alloc_id);     

    // TODO: algorithm for figuring out if we can free a region:
    // 1. go to region start                                    
    // 2. traverse through the linked list                      
    // 3. stop when you:                                        
    //     a. find something that's not free                    
    //     b. when you find the start of a different region     
    // 4. if (a) move on; if (b) then munmap                    

    /* Update the linked list */                                                            
	if (blk == g_head) {                                        
		g_head = blk->next;                                     
	} else {                                                    
		struct mem_block *prev = g_head;                        
		while (prev->next != blk) {                             
			prev = prev->next;                                  
		}                                                       
		prev->next = blk->next;                                 
	}                                                           

	int ret = munmap(blk->region_start, blk->region_size);      
	if (ret == -1) {                                            
		perror("munmap");                                       
	}
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

