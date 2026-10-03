#pragma once
#include <string.h>
#include "base.h"

///////////////////////// Arena Allocator {{{

/*
 * Arena allocator:
 *
 * Growing, chaining, commit-style arena allocator
 *
 * https://youtu.be/2wzcXBd8noQ?t=399
 * https://youtu.be/-XethY-QrR0?t=564
 *
 * functions:
 *  - {arena_alloc} initialize and set up the arena and the arena header.
 *  - {arena_push_no_zero} core allocation function to advance cursor in the active chunk, committing memory and chaining a new chunk as needed.
 *  - {arena_push} zero-initialized version for {arena_push_no_zero}.
 *  - {push_array} Allocates and returns zero-initialized array of len elements of type T from the arena.
 *
 * OVERVIEW:
 * - The Arena Header (Arena struct) is baked into the very start of the memory block it manages.
 *   Addressing the Arena struct itself serves as the base pointer to the entire memory block.
 *
 *   The Arena has:
 *     - Allocation offset `chunk_pos`, the cursor tracking the offset where the next piece of data will be allocated.
 *     - Commit limit `chunk_commit_pos`, the offset marking the upper boundary up to which memory has been committed.
 *     - Chunk capacity `chunk_cap`, the reverse limit for the total virtual memory size of this specfic chunk.
 *
 *   The chained growth structure allows us to implement a growing linear allocator without needing
 *   to resize the main memory block. The chained growth structure is functionally a (backward) singly
 *   linked list where each node in the list is a new separate chunk of memory. The linking is
 *   reversed, in that prev points to a previous chunk and current is the active chunk, where the
 *   Arena struct you use is the head of the chain and points to the previous chunk when it grows. New
 *   chunks are linked via `prev` pointer. Initially we start with one Arena, but if we need more
 *   reserve space, new chunk is allocated and old chunk is linked to the new chunk.
 *
 * - The chained growth structure implements a growing linear allocator by
 *   utilizing a singly linked list of separate memory chunks, thus avoiding
 *   resizing the main memory block.
 *
 * - When active chunk runs out of space, a new chunk is allocated, the old
 *   chunk is linked to the new new one using `prev` pointer, and head pointer
 *   is updated to new chunk `arena->current = new_chunk`, effectively creating
 *   a stack of chunks like so:
 *
 *   USAGE EXAMPLE: {{{
 *
 *   // Initial State: Chunk 1 is active head.
 *   arena->current -> [Chunk 1 | prev: NULL]
 *
 *   // Chunk 1 runs out of space, Chunk 2 is allocated.
 *   // The link is created: Chunk 2's prev points back to Chunk 1.
 *   // The head is updated: arena->current now points to Chunk 2.
 *   arena->current -> [Chunk 2 | prev: Chunk 1* ]
 *                     |
 *                     [Chunk 1 | prev: NULL]
 *
 *   // When Chunk 2 runs out of space, Chunk 3 is allocated.
 *   // The link is created: Chunk 3's prev points back to Chunk 2.
 *   // The head is updated: arena->current now points to Chunk 3.
 *   arena->current -> [ Chunk 3 | prev: Chunk 2* ]
 *                     |
 *                     [Chunk 2 | prev: Chunk 1* ]
 *                     |
 *                     [Chunk 1 | prev: NULL ]
 *
 *   // the direction of the allocation is forward: `Chunk 1 -> Chunk 2 -> -> Chunk 3`,
 *   // but the direction of the list traversal is backward `Chunk 3 -> Chunk 2 -> Chunk 1`.
 *
 *   }}}
 *
 * - Allocations can be zero-initialized by default:
 *   intetion behind this is to have less error-prone and simpler programs,
 *   but this can be opted-out of when it is too expensive.
 *
 * - Requires an offset to state the end of the last allocation.
 *   The end of the last allocation refers to the amount of memory
 *   that has been allocated in total (?)
 *
 * - `chunk_pos` tracks the offset where the next piece of data will be written. When memory is
 *   pushed, chunk_pos is aligned up, used, and then advanced by the allocation size.
 *
 * - You move the offset forward when you allocate memory by the size of the allocation.
 *
 * - All blocks have to be freed at once (you cannot free certain blocks of memory individually).
 */

/*
 * ALIGNMENT:
 * The pointer to the memory that is returned when you call VirtualAlloc or mmap should be aligned
 * correctly for the data you need, because unaligned memory access may be much slower than an
 * aligned memory access since computers read memory at its word size (4 bytes on 32 bit machines
 * and 8 bytes on 64 bit machines).
 *
 * To align a memory address to the specified alignment, you can use modulo arithmetic: look how
 * many bytes forwad you need to go in order for the memory address to be a multiple of the
 * specified alignment. We need to compute the number of bytes to advance the address (padding)
 * until the alignment evenly divides the address.
 *
 */

typedef struct Arena Arena;
struct Arena {
  /* --- Chained Arenas --- */
  Arena* current; // [Active Node] pointer to the active chunk, all allocations are tried here
                  // first. Used to quickly find where to allocate next.

  Arena* prev; // [Chain Link] pointer to the previously allocated Arena node, when a chunk is full,
               // a new chunk is allocated and linked via the prev pointer.

  u64 alignment; // memory alignment for allocations

  b8  growing;   // If arena is allowed to grow (chain new chunks).

  u8  filler[7]; // padding bytes to enforce structure alignment

  /* --- Chunk Offsets & Capacities --- */
  u64 base_pos; // [Global Offset] The total size of all *preceding* chunks in the chain (total size
                // of all previous chunks in the chain).

  u64 chunk_cap; // [Chunk Capacity] The hard limit of the reserved memory block for this specfic
                 // chunk.

  u64 chunk_pos; // [Allocation Offset] The current offset within this chunk where the next
                 // allocation will start (the "push" pointer).

  u64 chunk_commit_pos; // [Commit Limit] The current offset within this chunk up to which the
                        // memory is committed.
};

/**
 * push_array(arena, Type, len) (Type *) arena_push((arena), sizeof(Type) * (len))
 *   - arena, allocator to allocate the memory to
 *   - T, relevant type to calculate the bytes to allocate
 *   - len, amount of Type's added
 */
#  define push_array(arena, T, len) (T *) arena_push((arena), sizeof(T) * (len))

Arena* arena_alloc(void);
void* arena_push(Arena* arena, u64 size);
void* arena_push_no_zero(Arena* arena, u64 size);

/*
 * Scratch Arena
 *
 * - All allocations from a scratch arena are implicitly freed upon return (?)
 */
typedef struct Scratch Scratch;
struct Scratch {
  Arena* arena;
  u64    pos;
};

/////////////////////////
//// End of Arena   ////
/////////////////////// }}}
