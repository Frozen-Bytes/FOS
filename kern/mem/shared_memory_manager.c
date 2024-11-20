#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

struct FrameInfo* allocate_page(const struct Env* env, uint32 va , uint32 perm);

void free_share(struct Share* ptrShare);

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
struct Share* get_share(int32 ownerID, char* name);

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init()
{
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list) ;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	//[PROJECT'24.MS2] DONE
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = get_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}

//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] Create frames_storage:
//===========================
// Create the frames_storage and initialize it by 0
struct FrameInfo** create_frames_storage(int numOfFrames)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_frames_storage()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	// panic("create_frames_storage is not implemented yet");
	//Your Code is Here...

	assert(numOfFrames >= 1);

	uint32 storage_size = sizeof(void*) * numOfFrames;
	struct FrameInfo** storage = kmalloc(storage_size);

	if (!storage) {
		return NULL;
	}

	memset(storage, 0, storage_size);

	return storage;
}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
struct Share* create_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	// panic("create_share is not implemented yet");
	//Your Code is Here...

	struct Share* share_obj = kmalloc(sizeof(struct Share));
	if (!share_obj) {
		return NULL;
	}

	memset(share_obj, 0 , sizeof(struct Share));

	uint32 msb_mask = ~(1 << 31);
	share_obj->ID = ((uint32) share_obj & msb_mask);

	share_obj->ownerID = ownerID;
	share_obj->size = size;
	share_obj->references = 1;
	share_obj->isWritable = isWritable;

	uint32 num_of_frames = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	share_obj->framesStorage = create_frames_storage(num_of_frames);
	if (!share_obj->framesStorage) {
		kfree(share_obj);
		return NULL;
	}

	strncpy(share_obj->name, shareName, sizeof(share_obj->name));

	return share_obj;
}

//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* get_share(int32 ownerID, char* name)
{
	//TODO: [PROJECT'24.MS2 - #17] [4] SHARED MEMORY - get_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("get_share is not implemented yet");
	//Your Code is Here...

}

//=========================
// [4] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	// panic("createSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc();

	void *share_exist = get_share(ownerID, shareName);
	if (share_exist) {
		return E_SHARED_MEM_EXISTS;
	}

	struct Share* share_obj = create_share(ownerID, shareName, size, isWritable);
	if (!share_obj) {
		return E_NO_SHARE;
	}

	bool allocation_failed = 0;
	uint32 number_of_frames = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 va = (uint32) virtual_address;
	for (uint32 i = 0; i < number_of_frames; va += PAGE_SIZE, i++) {
		// Should shared allocations have a "locked" permission to avoid page replacement?
		uint32 pte_user =  PERM_PRESENT | PERM_WRITEABLE | PERM_USER;

		struct FrameInfo* frame_info = allocate_page(myenv, va,pte_user);
		if (!frame_info) {
			allocation_failed = 1;
			break;
		}

		share_obj->framesStorage[i] = frame_info;
	}

	if (allocation_failed) {
		free_share(share_obj);

		// Release allocated frames
		for (uint32 v = (uint32) virtual_address; v < va; v += PAGE_SIZE) {
			unmap_frame(myenv->env_page_directory, v);
		}

		return E_NO_SHARE;
	}

	acquire_spinlock(&AllShares.shareslock);
	LIST_INSERT_TAIL(&AllShares.shares_list, share_obj);
	release_spinlock(&AllShares.shareslock);

	return share_obj->ID;
}


//======================
// [5] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("getSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
void free_share(struct Share* ptrShare)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	// panic("free_share is not implemented yet");
	//Your Code is Here...

	assert(ptrShare);

	acquire_spinlock(&AllShares.shareslock);
	// Should do nothing, if `ptrShare` is not in the list
	LIST_REMOVE(&AllShares.shares_list, ptrShare);
	release_spinlock(&AllShares.shareslock);

	kfree(ptrShare->framesStorage);
	kfree(ptrShare);
}
//========================
// [B2] Free Share Object:
//========================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - freeSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("freeSharedObject is not implemented yet");
	//Your Code is Here...

}

//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
struct FrameInfo*
allocate_page(const struct Env* env, uint32 va , uint32 perm)
{
	uint32 *page_table = NULL;
	struct FrameInfo *frame_info = get_frame_info(env->env_page_directory, va, &page_table);

	// Reallocating a page is illegal (i.e. very bad)
	if (frame_info != NULL) {
		panic("allocate_page(): trying to allocate an already allocated page (va: %x)", va);
	}

	int status = allocate_frame(&frame_info);
	if (status == E_NO_MEM) {
		return NULL;
	}

	status = map_frame(ptr_page_directory, frame_info, va, perm);
	if (status == E_NO_MEM) {
		free_frame(frame_info);
		return NULL;
	}

	return frame_info;
}
