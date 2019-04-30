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

void scribbling (void *ptr, size_t length, char fill) {
	LOG("length is %zu fill is %c\n", length, fill);
	char *temp = (char *) ptr;
	size_t i;
	for(i = 0; i < length; i++) {
		*temp = fill;
		temp++;
	}
	LOG("length is %zu fill is %c\n", length, fill);
}

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
//        LOGP("1\n");
		if (current_block->region_start != current_region) {
//            LOGP("3\n");
			current_region = current_block->region_start;
			printf("[REGION] %p-%p %zu\n",
				current_region,
				(void *) current_region + current_region->region_size,
				current_region->region_size);
		}
//        LOGP("2\n");
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
void write_memory(FILE * fp)
{
	fprintf(fp, "-- Current Memory State --\n");
	struct mem_block *current_block = g_head;
	struct mem_block *current_region = NULL;
	while (current_block != NULL) {
//        LOGP("1\n");
		if (current_block->region_start != current_region) {
//            LOGP("3\n");
			current_region = current_block->region_start;
			fprintf(fp, "[REGION] %p-%p %zu\n",
				current_region,
				(void *) current_region + current_region->region_size,
				current_region->region_size);
		}
//        LOGP("2\n");
		fprintf(fp, "[BLOCK]  %p-%p (%ld) %zu %zu %zu\n",
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

void *first_fit_algo(size_t size) {
	LOG("first-fit - size = %zu\n", size);
	struct mem_block *curr = g_head;
	while(curr != NULL) {
		LOG("curr id is %lu\n", curr->alloc_id);
		size_t left = curr->size-curr->usage;
		LOG("left is %zu and size is %zu id is %lu\n", left, size, curr->alloc_id);
		if((left) >= size) {
			LOG("first-fit end id = %lu\n", curr->alloc_id);
			return curr;
		}
		LOG("after curr id is %lu\n", curr->alloc_id);
		curr = curr->next;
	}
	LOGP("first-fit end NULL\n");
	return NULL;
}

void *best_fit_algo(size_t size) {
	LOG("best-fit - size = %zu\n", size);
	struct mem_block *re_block = NULL;
	size_t min = SIZE_MAX;
	
	struct mem_block *curr = g_head;
	while(curr != NULL) {
		size_t remain = curr->size-curr->usage;
		if(remain >= size) {
			if(remain == size) {
				return curr;
			}
			if(remain < min) {
				re_block = curr;
				min = remain;
			}
		}

		curr = curr->next;
	}
	return re_block;
	
}

void *worst_fit_algo(size_t size) {
	LOG("worst-fit - size = %zu\n", size);
	struct mem_block *re_block = NULL;
	size_t max = 0;
	struct mem_block *curr = g_head;
	while(curr != NULL) {
		size_t remain = curr->size-curr->usage;
		if(remain >= size) {
			if(remain > max) {
				re_block = curr;
				max = remain;
			}
		}

		curr = curr->next;
	}
	
	return re_block;
}

void *reuse(size_t size) {
//    LOGP("call reuse\n");
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
	if(ptr == NULL) {
		LOGP("yes\n");
	}
	// struct mem_block *blk = (struct mem_block *)ptr;
	// LOG("returned id is %lu\n", blk->alloc_id);
    // TODO: using free space management algorithms, find a block of memory that
    // we can reuse. Return NULL if no suitable block is found.
	return ptr;
}

void *malloc_size(size_t size)
{
    // TODO: allocate memory. You'll first check if you can reuse an existing
    // block. If not, map a new memory region.
	LOG("call malloc -- size: %zu\n", size);
	size_t real_sz = size+sizeof(struct mem_block);
	struct mem_block *reuse_struct = reuse(real_sz);
	reuse_struct = NULL;

	if(reuse_struct == NULL) {
//        LOGP("mmap a new region\n");
		int page_sz = getpagesize();
		size_t num_pages = real_sz / page_sz;
		if(real_sz % page_sz != 0) {
			num_pages++;
		}
//        LOGP("blah\n");
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
//         LOG("111malloc end|id = %lu, size = %zu, usage = %zu\n", block->alloc_id, block->size, block->usage);
		LOG("XXXnew: %p\n", block);
		return block;

	} else {
		LOG("reuse a block id = %lu usage is %zu\n", reuse_struct->alloc_id, reuse_struct->usage);
		if (reuse_struct->usage == 0) {
			reuse_struct->usage = real_sz;
			reuse_struct->alloc_id = g_allocations++;
//            LOG("222- malloc end|id = %lu, size = %zu, usage = %zu\n", reuse_struct->alloc_id, reuse_struct->size, reuse_struct->usage);
			LOG("XXXreuse: %p\n", reuse_struct);
			return reuse_struct;
		} else {
			// LOGP("REUSE NOT 0\n");
			LOG("reuse size: %zu    %p\n", reuse_struct->size - reuse_struct->usage, reuse_struct->region_start);
			void * ptr = (void *)reuse_struct + reuse_struct->usage;
			// LOGP("REUSE NOT 0\n");
			struct mem_block * blkptr = (struct mem_block *) ptr;
			blkptr->alloc_id = g_allocations++;
			blkptr->usage = real_sz;
			blkptr->size = reuse_struct->size-reuse_struct->usage;
			blkptr->region_start = reuse_struct->region_start;
			blkptr->next = reuse_struct->next;
			reuse_struct->next = blkptr;
			reuse_struct->size = reuse_struct->usage;
			// LOGP("REUSE NOT 0\n");
           LOG("333malloc end|reuse_id = %lu, id = %lu, size = %zu, usage = %zu | check - %lu\n", reuse_struct->alloc_id, blkptr->alloc_id, blkptr->size, blkptr->usage, reuse_struct->next->alloc_id);
			LOG("XXXpointer is %p\n", blkptr);


			return blkptr;
		}
	}
}

void *malloc(size_t size) {
	LOG("malloc size is %zu\n", size);
	pthread_mutex_lock(&g_alloc_mutex);
	struct mem_block *blk = malloc_size(size);
	scribbling(blk + 1, blk->usage - sizeof(struct mem_block), 0xAA);
	pthread_mutex_unlock(&g_alloc_mutex);
	LOGP("return\n");
	return blk + 1;
}

void free(void *ptr)
{
//    return;
	LOGP("call free\n");
	// return;
	if (ptr == NULL) {
        /* Freeing a NULL pointer does nothing */               
		return;                                                 
	}
	pthread_mutex_lock(&g_alloc_mutex);

    /* Section 01: we didn't quite finish this below: (ptr -1) */                                                            
	struct mem_block *blk = (struct mem_block*) ptr - 1;        
	LOG("Free request; id = %lu\n", blk->alloc_id);

	blk->usage = 0;
	
	struct mem_block *curr = blk->region_start;
	struct mem_block *temp = blk->region_start;
	while(curr != NULL) {
		if(curr->region_start != temp) {
			break;
			// munmap(temp, temp->region_size);
		} else if (curr->usage != 0) {
//            LOGP("free end - move on\n");
			pthread_mutex_unlock(&g_alloc_mutex);
			return;
		}

		curr = curr->next;
	}
	if (temp == g_head) {
		g_head = curr;
	} else {
		struct mem_block *prev = g_head;
		while (prev->next != temp) {
			prev = prev->next;
		}
		prev->next = curr;
	}

	int ret = munmap(temp, temp->region_size);
	if (ret == -1) {
		perror("munmap");
	}
//    LOGP("Free end - munmap\n");
	pthread_mutex_unlock(&g_alloc_mutex);
	return;
}

void *calloc(size_t nmemb, size_t size)
{


	pthread_mutex_lock(&g_alloc_mutex);
//    void *malloc(size_t size) {
	struct mem_block *blk = malloc_size(size);
	LOGP("2222222222222222222\n");
	scribbling(blk + 1, blk->usage - sizeof(struct mem_block), 0);
	pthread_mutex_unlock(&g_alloc_mutex);
	return blk + 1;
//    }
    // TODO: hmm, what does calloc do?
}


void *realloc(void *ptr, size_t size)
{
	if (ptr == NULL) {
        /* If the pointer is NULL, then we simply malloc a new block */

		void *temp = malloc(size);
		LOGP("444444444444444444\n");



		return temp;
	}

	if (size == 0) {
        /* Realloc to 0 is often the same as freeing the memory block... But the
         * C standard doesn't require this. We will free the block and return
         * NULL here. */
		free(ptr);
		return NULL;
	}

    // TODO: reallocation logic
	pthread_mutex_lock(&g_alloc_mutex);

	struct mem_block *blk = (struct mem_block *) ptr - 1;
	size_t real_sz = size + sizeof(struct mem_block);
	if (blk->size >= real_sz) {
        // update usage
		blk->usage = real_sz;
		pthread_mutex_unlock(&g_alloc_mutex);

		return ptr;
	} else {
        // 1. malloc new block
		pthread_mutex_unlock(&g_alloc_mutex);

		void *new_blk = malloc(size);
		LOGP("333333\n");
		pthread_mutex_lock(&g_alloc_mutex);

        // 2. copy the data there
		char * temp = (char *)new_blk;
		char * temp_ptr =(char *) ptr;
		int i;
		for(i = 0; i < blk->usage; i++) {
			*temp = *temp_ptr;
			temp_ptr++;
			temp++;
		}
        // 3. free the old block
		pthread_mutex_unlock(&g_alloc_mutex);

		free(ptr);
		return new_blk;

	}
//    void *ptr = malloc_size()
}
