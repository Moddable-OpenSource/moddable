/*
 * SPDX-FileCopyrightText: 2006-2016 Matthew Conte
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <limits.h>
#include <stdio.h>
#include "tlsf.h"
#include "tlsf_common.h"
#include "tlsf_block_functions.h"
#include "tlsf_control_functions.h"

/*
** Static assertion mechanism.
*/

#define _tlsf_glue2(x, y) x ## y
#define _tlsf_glue(x, y) _tlsf_glue2(x, y)
#define tlsf_static_assert(exp) \
	typedef char _tlsf_glue(static_assert, __LINE__) [(exp) ? 1 : -1]

/* This code has been tested on 32- and 64-bit (LP/LLP) architectures. */
tlsf_static_assert(sizeof(int) * CHAR_BIT == 32);
tlsf_static_assert(sizeof(size_t) * CHAR_BIT >= 32);
tlsf_static_assert(sizeof(size_t) * CHAR_BIT <= 64);

/* Clear structure and point all empty lists at the null block. */
static control_t* control_construct(control_t* control, size_t bytes)
{
	// check that the requested size can at least hold the control_t. This will allow us 
	// to fill in the field of control_t necessary to determine the final size of 
	// the metadata overhead and check that the requested size can hold
	// this data and at least a block of minimum size
	if (bytes < sizeof(control_t))
	{
		return NULL;
	}

	/* Find the closest power of two for first layer */
	control->fl_index_max = 32 - __builtin_clz(bytes);

	/* Adapt second layer to the pool */
	if (bytes <= 16 * 1024) control->sl_index_count_log2 = 3;
	else if (bytes <= 256 * 1024) control->sl_index_count_log2 = 4;
	else control->sl_index_count_log2 = 5;

	control->fl_index_shift = (control->sl_index_count_log2 + ALIGN_SIZE_LOG2);
	control->sl_index_count = 1 << control->sl_index_count_log2;
	control->fl_index_count = control->fl_index_max - control->fl_index_shift + 1;
	control->small_block_size = 1 << control->fl_index_shift;
	
	// the total size fo the metadata overhead is the size of the control_t
	// added to the size of the sl_bitmaps and the size of blocks
	control->size = sizeof(control_t) + (sizeof(*control->sl_bitmap) * control->fl_index_count) +
										(sizeof(*control->blocks) * (control->fl_index_count * control->sl_index_count));

	// check that the requested size can hold the whole control structure and
	// a small block at least
	if (bytes < control->size + block_size_min)
	{
		return NULL;
	}

	control->block_null.next_free = &control->block_null;
	control->block_null.prev_free = &control->block_null;

	control->fl_bitmap = 0;
	control->sl_bitmap = align_ptr(control + 1, sizeof(*control->sl_bitmap));
	control->blocks = align_ptr(control->sl_bitmap + control->fl_index_count, sizeof(*control->blocks));


	/* SL_INDEX_COUNT must be <= number of bits in sl_bitmap's storage type. */
	tlsf_assert(sizeof(unsigned int) * CHAR_BIT >= control->sl_index_count
		&& "CHAR_BIT less than sl_index_count");

	/* Ensure we've properly tuned our sizes. */
	tlsf_assert(ALIGN_SIZE == control->small_block_size / control->sl_index_count); //ALIGN_SIZE does not match");

	for (int i = 0; i < control->fl_index_count; ++i)
	{
		control->sl_bitmap[i] = 0;
		for (int j = 0; j < control->sl_index_count; ++j)
		{
			control->blocks[i * control->sl_index_count + j] = &control->block_null;
		}
	}

	return control;
}

/*
** Debugging utilities.
*/

typedef struct integrity_t
{
	int prev_status;
	int status;
} integrity_t;

#define tlsf_insist(x) { if (!(x)) { status--; } }

static bool integrity_walker(void* ptr, size_t size, int used, void* user)
{
	block_header_t* block = block_from_ptr(ptr);
	integrity_t* integ = tlsf_cast(integrity_t*, user);
	const int this_prev_status = block_is_prev_free(block) ? 1 : 0;
	const int this_status = block_is_free(block) ? 1 : 0;
	const size_t this_block_size = block_size(block);

	int status = 0;
	tlsf_insist(integ->prev_status == this_prev_status && "prev status incorrect");
	tlsf_insist(size == this_block_size && "block size incorrect");

	if (tlsf_check_hook != NULL)
	{
		/* block_size(block) returns the size of the usable memory when the block is allocated.
		 * As the block under test is free, we need to subtract to the block size the next_free
		 * and prev_free fields of the block header as they are not a part of the usable memory
		 * when the block is free. In addition, we also need to subtract the size of prev_phys_block
		 * as this field is in fact part of the current free block and not part of the next (allocated)
		 * block. Check the comments in block_split function for more details.
		 */
		const size_t actual_free_block_size = used ? this_block_size : 
													 this_block_size - offsetof(block_header_t, next_free)- block_header_overhead;
		
		void* ptr_block = used ? (void*)block + block_start_offset :
								 (void*)block + sizeof(block_header_t);

		tlsf_insist(tlsf_check_hook(ptr_block, actual_free_block_size, !used));
	}

	integ->prev_status = this_status;
	integ->status += status;

	return true;
}


int tlsf_check(tlsf_t tlsf)
{
	int i, j;

	control_t* control = tlsf_cast(control_t*, tlsf);
	int status = 0;

	/* Check that the free lists and bitmaps are accurate. */
	for (i = 0; i < control->fl_index_count; ++i)
	{
		for (j = 0; j < control->sl_index_count; ++j)
		{
			const int fl_map = control->fl_bitmap & (1U << i);
			const int sl_list = control->sl_bitmap[i];
			const int sl_map = sl_list & (1U << j);
			const block_header_t* block = control->blocks[i * control->sl_index_count + j];

			/* Check that first- and second-level lists agree. */
			if (!fl_map)
			{
				tlsf_insist(!sl_map && "second-level map must be null");
			}

			if (!sl_map)
			{
				tlsf_insist(block == &control->block_null && "block list must be null");
				continue;
			}

			/* Check that there is at least one free block. */
			tlsf_insist(sl_list && "no free blocks in second-level map");
			tlsf_insist(block != &control->block_null && "block should not be null");

			while (block != &control->block_null)
			{
				int fli, sli;
				const bool is_block_free = block_is_free(block);
				tlsf_insist(is_block_free && "block should be free");
				tlsf_insist(!block_is_prev_free(block) && "blocks should have coalesced");
				tlsf_insist(!block_is_free(block_next(block)) && "blocks should have coalesced");
				tlsf_insist(block_is_prev_free(block_next(block)) && "block should be free");
				tlsf_insist(block_size(block) >= block_size_min && "block not minimum size");

				mapping_insert(control, block_size(block), &fli, &sli);
				tlsf_insist(fli == i && sli == j && "block size indexed in wrong list");

				block = block->next_free;
			}
		}
	}

	return status;
}

#undef tlsf_insist

static bool default_walker(void* ptr, size_t size, int used, void* user)
{
	(void)user;
	printf("\t%p %s size: %x (%p)\n", ptr, used ? "used" : "free", (unsigned int)size, block_from_ptr(ptr));
	return true;
}

void tlsf_walk_pool(pool_t pool, tlsf_walker walker, void* user)
{
	tlsf_walker pool_walker = walker ? walker : default_walker;
	block_header_t* block =
		offset_to_block(pool, -(int)block_header_overhead);

	bool ret_val = true;
	while (block && !block_is_last(block) && ret_val == true)
	{
		ret_val = pool_walker(
			block_to_ptr(block),
			block_size(block),
			!block_is_free(block),
			user);

		if (ret_val == true) {
			block = block_next(block);
		}
	}
}

size_t tlsf_block_size(void* ptr)
{
	size_t size = 0;
	if (ptr)
	{
		const block_header_t* block = block_from_ptr(ptr);
		size = block_size(block);
	}
	return size;
}

int tlsf_check_pool(pool_t pool)
{
	/* Check that the blocks are physically correct. */
	integrity_t integ = { 0, 0 };
	tlsf_walk_pool(pool, integrity_walker, &integ);

	return integ.status;
}

size_t tlsf_fit_size(tlsf_t tlsf, size_t size)
{
	if (size == 0 || tlsf == NULL) {
		return 0;
	}

	control_t* control = tlsf_cast(control_t*, tlsf);
	if (size < control->small_block_size) {
		return adjust_request_size(tlsf, size, ALIGN_SIZE);
	}

	/* because it's GoodFit, allocable size is one range lower */
	size_t sl_interval;
	sl_interval = (1 << (32 - __builtin_clz(size) - 1)) / control->sl_index_count;
	return size & ~(sl_interval - 1);
}

/*
** Size of the TLSF structures in a given memory block passed to
** tlsf_create, equal to the size of a control_t
*/
size_t tlsf_size(tlsf_t tlsf)
{
	if (tlsf == NULL)
	{
		return 0;
	}
	control_t* control = tlsf_cast(control_t*, tlsf);
	return control->size;
}

/*
** Overhead of the TLSF structures in a given memory block passed to
** tlsf_add_pool, equal to the overhead of a free block and the
** sentinel block.
*/
size_t tlsf_pool_overhead(void)
{
	return 2 * block_header_overhead;
}

size_t tlsf_alloc_overhead(void)
{
	return block_header_overhead;
}

pool_t tlsf_add_pool(tlsf_t tlsf, void* mem, size_t bytes)
{
	block_header_t* block;
	block_header_t* next;

	const size_t pool_overhead = tlsf_pool_overhead();
	const size_t pool_bytes = align_down(bytes - pool_overhead, ALIGN_SIZE);

	if (((ptrdiff_t)mem % ALIGN_SIZE) != 0)
	{
		printf("tlsf_add_pool: Memory must be aligned by %u bytes.\n",
			(unsigned int)ALIGN_SIZE);
		return 0;
	}

	if (pool_bytes < block_size_min || pool_bytes > tlsf_block_size_max(tlsf))
	{
#if defined (TLSF_64BIT)
		printf("tlsf_add_pool: Memory size must be between 0x%x and 0x%x00 bytes.\n", 
			(unsigned int)(pool_overhead + block_size_min),
			(unsigned int)((pool_overhead + tlsf_block_size_max(tlsf)) / 256));
#else
		printf("tlsf_add_pool: Memory size must be between %u and %u bytes.\n", 
			(unsigned int)(pool_overhead + block_size_min),
			(unsigned int)(pool_overhead + tlsf_block_size_max(tlsf)));
#endif
		return 0;
	}

	/*
	** Create the main free block. Offset the start of the block slightly
	** so that the prev_phys_block field falls outside of the pool -
	** it will never be used.
	*/
	block = offset_to_block(mem, -(tlsfptr_t)block_header_overhead);
	block_set_size(block, pool_bytes);
	block_set_free(block);
	block_set_prev_used(block);
	block_insert(tlsf_cast(control_t*, tlsf), block);

	/* Split the block to create a zero-size sentinel block. */
	next = block_link_next(block);
	block_set_size(next, 0);
	block_set_used(next);
	block_set_prev_free(next);

	return mem;
}

void tlsf_remove_pool(tlsf_t tlsf, pool_t pool)
{
	control_t* control = tlsf_cast(control_t*, tlsf);
	block_header_t* block = offset_to_block(pool, -(int)block_header_overhead);

	int fl = 0, sl = 0;

	tlsf_assert(block_is_free(block) && "block should be free");
	tlsf_assert(!block_is_free(block_next(block)) && "next block should not be free");
	tlsf_assert(block_size(block_next(block)) == 0 && "next block size should be zero");

	mapping_insert(control, block_size(block), &fl, &sl);
	remove_free_block(control, block, fl, sl);
}

/*
** TLSF main interface.
*/

#if _DEBUG
int test_ffs_fls()
{
	/* Verify ffs/fls work properly. */
	int rv = 0;
	rv += (tlsf_ffs(0) == -1) ? 0 : 0x1;
	rv += (tlsf_fls(0) == -1) ? 0 : 0x2;
	rv += (tlsf_ffs(1) == 0) ? 0 : 0x4;
	rv += (tlsf_fls(1) == 0) ? 0 : 0x8;
	rv += (tlsf_ffs(0x80000000) == 31) ? 0 : 0x10;
	rv += (tlsf_ffs(0x80008000) == 15) ? 0 : 0x20;
	rv += (tlsf_fls(0x80000008) == 31) ? 0 : 0x40;
	rv += (tlsf_fls(0x7FFFFFFF) == 30) ? 0 : 0x80;

#if defined (TLSF_64BIT)
	rv += (tlsf_fls_sizet(0x80000000) == 31) ? 0 : 0x100;
	rv += (tlsf_fls_sizet(0x100000000) == 32) ? 0 : 0x200;
	rv += (tlsf_fls_sizet(0xffffffffffffffff) == 63) ? 0 : 0x400;
#endif

	if (rv)
	{
		printf("test_ffs_fls: %x ffs/fls tests failed.\n", rv);
	}
	return rv;
}
#endif

tlsf_t tlsf_create(void* mem, size_t max_bytes)
{
#if _DEBUG
	if (test_ffs_fls())
	{
		return NULL;
	}
#endif

	if (mem == NULL)
	{
		return NULL;
	}

	if (((tlsfptr_t)mem % ALIGN_SIZE) != 0)
	{
		printf("tlsf_create: Memory must be aligned to %u bytes.\n",
			(unsigned int)ALIGN_SIZE);
		return NULL;
	}

	control_t* control_ptr = control_construct(tlsf_cast(control_t*, mem), max_bytes);
	return tlsf_cast(tlsf_t, control_ptr);
}

tlsf_t tlsf_create_with_pool(void* mem, size_t pool_bytes, size_t max_bytes)
{
	tlsf_t tlsf = tlsf_create(mem, max_bytes ? max_bytes : pool_bytes);
	if (tlsf != NULL)
	{
		tlsf_add_pool(tlsf, (char*)mem + tlsf_size(tlsf), pool_bytes - tlsf_size(tlsf));
	}
	return tlsf;
}

void tlsf_destroy(tlsf_t tlsf)
{
	/* Nothing to do. */
	(void)tlsf;
}

pool_t tlsf_get_pool(tlsf_t tlsf)
{
	return tlsf_cast(pool_t, (char*)tlsf + tlsf_size(tlsf));
}

void* tlsf_malloc(tlsf_t tlsf, size_t size)
{
	control_t* control = tlsf_cast(control_t*, tlsf);
	size_t adjust = adjust_request_size(tlsf, size, ALIGN_SIZE);
	// Returned size is 0 when the requested size is larger than the max block
	// size.
	if (adjust == 0) {
		return NULL;
	}
	// block_locate_free() may adjust our allocated size further.
	block_header_t* block = block_locate_free(control, &adjust);
	return block_prepare_used(control, block, adjust);
}

/**
 * @brief Allocate memory of at least `size` bytes at a given address in the pool.
 *
 * @param tlsf TLSF structure to allocate memory from.
 * @param size Minimum size, in bytes, of the memory to allocate
 * @param address address at which the allocation must be done
 *
 * @return pointer to free memory or NULL in case of incapacity to perform the malloc
 */
void* tlsf_malloc_addr(tlsf_t tlsf, size_t size, void *address)
{
	control_t* control = tlsf_cast(control_t*, tlsf);

	/* adjust the address to be ALIGN_SIZE bytes aligned. */
	const unsigned int addr_adjusted = align_down(tlsf_cast(unsigned int, address), ALIGN_SIZE);

	/* adjust the size to be ALIGN_SIZE bytes aligned. Add to the size the difference
	 * between the requested address and the address_adjusted. */
	size_t size_adjusted = align_up(size + (tlsf_cast(unsigned int, address) - addr_adjusted), ALIGN_SIZE);

	/* find the free block that starts before the address in the pool and is big enough
	 * to support the size of allocation at the given address */
	block_header_t* block = offset_to_block(tlsf_get_pool(tlsf), -(int)block_header_overhead);
	
	const char *alloc_start = tlsf_cast(char*, addr_adjusted);
	const char *alloc_end = alloc_start + size_adjusted;
	bool block_found = false;
	do {
		const char *block_start = tlsf_cast(char*, block_to_ptr(block));
		const char *block_end = tlsf_cast(char*, block_to_ptr(block)) + block_size(block);
		if (block_start <= alloc_start && block_end > alloc_start) {
			/* A: block_end >= alloc_end. B: block is free */
			if (block_end < alloc_end || !block_is_free(block)) {
				/* not(A) || not(B)
				 * We won't find another suitable block from this point on
				 * so we can break and return NULL */
				break;
			} 
			/* A && B
			 * The block can fit the alloc and is located at a position allowing for the alloc
			 * to be placed at the given address. We can return from the while */
			block_found = true;
		} else if (!block_is_last(block)) {
			/* the block doesn't match the expected criteria, continue with the next block */
			block = block_next(block);
		}

	} while (!block_is_last(block) && block_found == false);

	if (!block_found) {
		return NULL;
	}
	
	/* remove block from the free list since a part of it will be used */
	block_remove(control, block);

	/* trim any leading space or add the leading space to the overall requested size
	 * if the leading space is not big enough to store a block of minimum size */
	const size_t space_before_addr_adjusted = addr_adjusted - tlsf_cast(unsigned int, block_to_ptr(block));
	block_header_t *return_block = block;
	if (space_before_addr_adjusted >= block_size_min) {
		return_block = block_trim_free_leading(control, block, space_before_addr_adjusted);
	}
	else {
		size_adjusted += space_before_addr_adjusted;
	}

	/* trim trailing space if any and return a pointer to the first usable byte allocated */
	return  block_prepare_used(control, return_block, size_adjusted);
}

/**
 * @brief Allocate memory of at least `size` bytes where byte at `data_offset` will be aligned to `alignment`.
 *
 * This function will allocate memory pointed by `ptr`. However, the byte at `data_offset` of
 * this piece of memory (i.e., byte at `ptr` + `data_offset`) will be aligned to `alignment`.
 * This function is useful for allocating memory that will internally have a header, and the
 * usable memory following the header (i.e. `ptr` + `data_offset`) must be aligned.
 *
 * For example, a call to `multi_heap_aligned_alloc_impl_offs(heap, 64, 256, 20)` will return a
 * pointer `ptr` to free memory of minimum 64 bytes, where `ptr + 20` is aligned on `256`.
 * So `(ptr + 20) % 256` equals 0.
 *
 * @param tlsf TLSF structure to allocate memory from.
 * @param align Alignment for the returned pointer's offset.
 * @param size Minimum size, in bytes, of the memory to allocate INCLUDING
 *             `data_offset` bytes.
 * @param data_offset Offset to be aligned on `alignment`. This can be 0, in
 *                    this case, the returned pointer will be aligned on
 *                    `alignment`. If it is not a multiple of CPU word size,
 *                    it will be aligned up to the closest multiple of it.
 *
 * @return pointer to free memory.
 */
void* tlsf_memalign_offs(tlsf_t tlsf, size_t align, size_t size, size_t data_offset)
{
	control_t* control = tlsf_cast(control_t*, tlsf);
	const size_t adjust = adjust_request_size(tlsf, size, ALIGN_SIZE);
	const size_t off_adjust = align_up(data_offset, ALIGN_SIZE);

	/*
	** We must allocate an additional minimum block size bytes so that if
	** our free block will leave an alignment gap which is smaller, we can
	** trim a leading free block and release it back to the pool. We must
	** do this because the previous physical block is in use, therefore
	** the prev_phys_block field is not valid, and we can't simply adjust
	** the size of that block.
	*/
	const size_t gap_minimum = sizeof(block_header_t) + off_adjust;
	/* The offset is included in both `adjust` and `gap_minimum`, so we
	** need to subtract it once.
	*/
	const size_t size_with_gap = adjust_request_size(tlsf, adjust + align + gap_minimum - off_adjust, align);

	/*
	** If alignment is less than or equal to base alignment, we're done, because
	** we are guaranteed that the size is at least sizeof(block_header_t), enough
	** to store next blocks' metadata. Plus, all pointers allocated will all be
	** aligned on a 4-byte bound, so ptr + data_offset will also have this
	** alignment constraint. Thus, the gap is not required.
	** If we requested 0 bytes, return null, as tlsf_malloc(0) does.
	*/
	size_t aligned_size = (adjust && align > ALIGN_SIZE) ? size_with_gap : adjust;

	block_header_t* block = block_locate_free(control, &aligned_size);

	/* This can't be a static assert. */
	tlsf_assert(sizeof(block_header_t) == block_size_min + block_header_overhead);

	if (block)
	{
		void* ptr = block_to_ptr(block);
		void* aligned = align_ptr(ptr, align);
		size_t gap = tlsf_cast(size_t,
			tlsf_cast(tlsfptr_t, aligned) - tlsf_cast(tlsfptr_t, ptr));

	   /*
		** If gap size is too small or if there is no gap but we need one,
		** offset to next aligned boundary.
		** NOTE: No need for a gap if the alignment required is less than or is
		** equal to ALIGN_SIZE.
		*/
		if ((gap && gap < gap_minimum) || (!gap && off_adjust && align > ALIGN_SIZE))
		{
			const size_t gap_remain = gap_minimum - gap;
			const size_t offset = tlsf_max(gap_remain, align);
			const void* next_aligned = tlsf_cast(void*,
				tlsf_cast(tlsfptr_t, aligned) + offset);

			aligned = align_ptr(next_aligned, align);
			gap = tlsf_cast(size_t,
				tlsf_cast(tlsfptr_t, aligned) - tlsf_cast(tlsfptr_t, ptr));
		}

		if (gap)
		{
			tlsf_assert(gap >= gap_minimum && "gap size too small");
			block = block_trim_free_leading(control, block, gap - off_adjust);
		}
	}

	/* Preparing the block will also the trailing free memory. */
	return block_prepare_used(control, block, adjust);
}

/**
 * @brief Same as `tlsf_memalign_offs` function but with a 0 offset.
 * The pointer returned is aligned on `align`.
 */
void* tlsf_memalign(tlsf_t tlsf, size_t align, size_t size)
{
	return tlsf_memalign_offs(tlsf, align, size, 0);
}


void tlsf_free(tlsf_t tlsf, void* ptr)
{
	/* Don't attempt to free a NULL pointer. */
	if (ptr)
	{
		control_t* control = tlsf_cast(control_t*, tlsf);
		block_header_t* block = block_from_ptr(ptr);
		tlsf_assert(!block_is_free(block) && "block already marked as free");
		block_mark_as_free(block);
		block = block_merge_prev(control, block);
		block = block_merge_next(control, block);
		block_insert(control, block);
	}
}

/*
** The TLSF block information provides us with enough information to
** provide a reasonably intelligent implementation of realloc, growing or
** shrinking the currently allocated block as required.
**
** This routine handles the somewhat esoteric edge cases of realloc:
** - a non-zero size with a null pointer will behave like malloc
** - a zero size with a non-null pointer will behave like free
** - a request that cannot be satisfied will leave the original buffer
**   untouched
** - an extended buffer size will leave the newly-allocated area with
**   contents undefined
*/
void* tlsf_realloc(tlsf_t tlsf, void* ptr, size_t size)
{
	control_t* control = tlsf_cast(control_t*, tlsf);
	void* p = 0;

	/* Zero-size requests are treated as free. */
	if (ptr && size == 0)
	{
		tlsf_free(tlsf, ptr);
	}
	/* Requests with NULL pointers are treated as malloc. */
	else if (!ptr)
	{
		p = tlsf_malloc(tlsf, size);
	}
	else
	{
		block_header_t* block = block_from_ptr(ptr);
		block_header_t* next = block_next(block);

		const size_t cursize = block_size(block);
		const size_t combined = cursize + block_size(next) + block_header_overhead;
		const size_t adjust = adjust_request_size(tlsf, size, ALIGN_SIZE);

		// if adjust if equal to 0, the size is too big
		if (adjust == 0)
		{
			return p;
		}

		tlsf_assert(!block_is_free(block) && "block already marked as free");

		/*
		** If the next block is used, or when combined with the current
		** block, does not offer enough space, we must reallocate and copy.
		*/
		if (adjust > cursize && (!block_is_free(next) || adjust > combined))
		{
			p = tlsf_malloc(tlsf, size);
			if (p)
			{
				const size_t minsize = tlsf_min(cursize, size);
				memcpy(p, ptr, minsize);
				tlsf_free(tlsf, ptr);
			}
		}
		else
		{
			/* Do we need to expand to the next block? */
			if (adjust > cursize)
			{
				block_merge_next(control, block);
				block_mark_as_used(block);
			}

			/* Trim the resulting block and return the original pointer. */
			block_trim_used(control, block, adjust);
			p = ptr;
		}
	}

	return p;
}
