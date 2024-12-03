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
#include "inc/types.h"
#include "kern/conc/spinlock.h"
#include "kheap.h"
#include "memory_manager.h"

static struct FrameInfo* allocate_page(const struct Env* env, uint32 va , uint32 perm);
static bool pt_is_page_empty(uint32* page_dir, uint32 va);

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

	// uint32 msb_mask = ~(1 << 31);
	share_obj->ID = ((uint32) share_obj & (~(1 << 31))); // msb masking

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

	// NOTE: ALWAYS USE `share_obj->name` WITH `sizeof` OPERATOR!!!
	// BE WARNED VERY VERY BAD THING COULD AND WILL HAPPEN!!!
	uint32 name_buffer_len = sizeof(share_obj->name) / sizeof(*share_obj->name);
	strncpy(share_obj->name, shareName, name_buffer_len);

	// Make sure the string is always null terminated, OR BAD THINGS WILL HAPPEN!!!
	share_obj->name[name_buffer_len - 1] = '\0';

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
	acquire_spinlock(&AllShares.shareslock);

    struct Share* object = NULL;
	bool found = 0;
	LIST_FOREACH(object , &AllShares.shares_list) {
		if (object->ownerID == ownerID && strcmp(object->name, name) == 0) {
			found = 1;
			break; 
		}
	}

	release_spinlock(&AllShares.shareslock);

	if (found == 1) {
		return object;
	}
	else {
		return NULL;
	}
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

	uint8 allocation_failed = 0;
	uint32 pte_user = PERM_PRESENT | PERM_WRITEABLE | PERM_USER;
	uint32 number_of_frames = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 va = (uint32) virtual_address;
	for (uint32 i = 0; i < number_of_frames; va += PAGE_SIZE, i++) {
		// Should shared allocations have a "locked" permission to avoid page replacement?

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
/*
Get the shared object from the "shares_list" done
Get its physical frames from the “frames_storage” done
Share these frames with the current process starting from the given "virtual_address" done
Use the flag isWritable to make the sharing either read-only OR writable done
Update references done gamed
RETURN:
	ID of the shared object (its VA after masking out its msb) if success
	E_SHARED_MEM_NOT_EXISTS if the shared object is NOT exists


*/
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	// panic("getSharedObject is not implemented yet");
	struct Env* myenv = get_cpu_proc(); //The calling environment
	struct Share *shared_obj = get_share(ownerID, shareName);
	if(!shared_obj){
		return E_SHARED_MEM_NOT_EXISTS;
	}
	uint32 number_of_frames = ROUNDUP(shared_obj->size, PAGE_SIZE) / PAGE_SIZE;

	for (uint32 i = 0; i < number_of_frames; virtual_address += PAGE_SIZE, i++){
		struct FrameInfo* frame = shared_obj->framesStorage[i];

		uint32 perm = PERM_USER | (shared_obj->isWritable ? PERM_WRITEABLE : 0);
		map_frame(myenv->env_page_directory, frame, (uint32)virtual_address, perm);
	}

	shared_obj->references++;

	return shared_obj->ID;
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
	// panic("freeSharedObject is not implemented yet");
	//Your Code is Here...
	struct Share* share_obj = NULL;

	acquire_spinlock(&AllShares.shareslock);
	bool found = 0;
	LIST_FOREACH(share_obj, &AllShares.shares_list) {
		if (share_obj->ID == sharedObjectID) { 
			found = 1;
			break;
		}
	}
	release_spinlock(&AllShares.shareslock);
	

	if (!found) {
		return -1;
	}

	struct Env* myenv = get_cpu_proc();
	uint32 start_va = ROUNDDOWN((uint32) startVA, PAGE_SIZE);
	uint32 end_va = start_va + ROUNDUP(share_obj->size, PAGE_SIZE);
	for (uint32 va = start_va; va < end_va; va += PAGE_SIZE) {
		uint32* page_table = NULL;
		get_page_table(myenv->env_page_directory, va, &page_table);

		unmap_frame(myenv->env_page_directory, va);
		if (pt_is_page_empty(myenv->env_page_directory, va)) {
			pd_clear_page_dir_entry(myenv->env_page_directory, va);
			kfree(page_table);
		}
	}
	share_obj->references--;
	if (share_obj->references == 0) {
		free_share(share_obj);
	}

	tlbflush();

	return 0;
}

//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
static struct FrameInfo*
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

	status = map_frame(env->env_page_directory, frame_info, va, perm);
	if (status == E_NO_MEM) {
		free_frame(frame_info);
		return NULL;
	}

	return frame_info;
}

static bool
pt_is_page_empty(uint32* page_dir, uint32 va) {
	uint32* page_table = NULL;
	get_page_table(page_dir, va, &page_table);
	if (!page_table) {
		return 1;
	}

	uint32* end_va = (void*) page_table + PAGE_SIZE;
	for (uint32* va = page_table; va < end_va; va++) {
		int32 perm = *va;
		if ((perm & PERM_PRESENT) || (perm & PERM_USER_MARKED)) {
			return 0;
		}
	}

	return 1;
}
