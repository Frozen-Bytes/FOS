#ifndef FOS_INC_DYNBLK_MANAGE_H
#define FOS_INC_DYNBLK_MANAGE_H
#include <inc/queue.h>
#include <inc/types.h>
#include <inc/environment_definitions.h>

/*Data*/
/*Max Size for the Dynamic Allocator*/
#define DYN_ALLOC_MAX_SIZE (32<<20) 		//32 MB
#define DYN_ALLOC_MAX_BLOCK_SIZE (1<<11) 	//2 KB
#define DYN_ALLOC_MIN_BLOCK_SIZE (1<<3) 	//8 BYTE

/*Implementation Type of List*/
#define IMPLICIT_LIST 1
#define EXPLICIT_LIST_ALL 2
#define EXPLICIT_LIST_FREE_ONLY 3
#define LIST_IMPLEMENTATION EXPLICIT_LIST_FREE_ONLY

/*Allocation Type*/
enum
{
	DA_FF = 1,
	DA_NF,
	DA_BF,
	DA_WF
};

//=============================================================================
//TODO: [PROJECT'24.MS1 - #00 GIVENS] [3] DYNAMIC ALLOCATOR - data structures
struct BlockElement
{
	LIST_ENTRY(BlockElement) prev_next_info;	/* linked list links */
};// __attribute__((packed))

LIST_HEAD(MemBlock_LIST, BlockElement);
struct MemBlock_LIST freeBlocksList ;
//=============================================================================

/*Functions*/

/*2024*/
//should be implemented inside kern/mem/kheap.c
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit);
//should be implemented inside kern/proc/user_environment.c
void initialize_uheap_dynamic_allocator(struct Env* env, uint32 daStart, uint32 daLimit);

//=============================================================================
//TODO: [PROJECT'24.MS1 - #00 GIVENS] [3] DYNAMIC ALLOCATOR - helper functions
__inline__ uint32 get_block_size(void* va);
__inline__ int8 is_free_block(void* va);
void print_blocks_list(struct MemBlock_LIST list);
//=============================================================================

//Required Functions
//In KernelHeap: should be implemented inside kern/mem/kheap.c
//In UserHeap: should be implemented inside lib/uheap.c
void* sbrk(int numOfPages);

void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace);
void set_block_data(void* va, uint32 totalSize, bool isAllocated);
void *alloc_block(uint32 size, int ALLOC_STRATEGY);
void *alloc_block_FF(uint32 size);
void *alloc_block_BF(uint32 size);
void *alloc_block_WF(uint32 size);
void *alloc_block_NF(uint32 size);
void free_block(void* va);
void *realloc_block_FF(void* va, uint32 new_size);

#endif
