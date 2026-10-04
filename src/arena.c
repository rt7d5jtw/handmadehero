/**
 * Reserve and commit memory from the currenk chunk to satisfy {size}.
 *
 *   1. Check current chunk capacity: calculate the required position `next_chunk_pos` after
 *       alignment and adding {size}
 *
 *   2. Allocate new chunk (if necessary): if we exceed chunk capacity (reserved memory limit for
 *      the current chunk), reverse and commit more memory.
 *
 *   3. Commit more memory (if necessary): make more reversed memory usable (lazy committing)
 *
 *   4. Allocate and Advance: If the memory fits within committed range, return a pointer to the
 *      aligned start of the requested memory block and advance `current->chunk_pos`
 */
void* arena_push_no_zero(Arena* arena, u64 size)
{
  void*  result  = 0;
  Arena* current = arena->current;

  // allocate new chunk if necessary
  if (arena->growing) {
    // Align forward
    u64 next_chunk_pos = AlignUpPow2(current->chunk_pos, arena->alignment);
    // add size to next_chunk_pos
    next_chunk_pos += size;

    // allocate new chunk if necessary
    if (next_chunk_pos > current->chunk_cap) {
      u64 new_reserve_size = MEM_DEFAULT_RESERVE_SIZE;
      u64 enough_to_fit    = size + MEM_INTERNAL_MIN_SIZE;

      if (new_reserve_size < enough_to_fit) {
        new_reserve_size = AlignUpPow2(enough_to_fit, KiB(4));
      }

      void* memory = os_memory_reserve(new_reserve_size);
      if (os_memory_commit(memory, MEM_INITIAL_COMMIT)) {
        // AsanPoison(memory, new_reserve_size);
        // AsanUnpoison(memory, MEM_INTERNAL_MIN_SIZE);

        // The Arena Header is placed at the start of the reserved memory block, this memory block
        // is then passed on to the caller.
        Arena* new_chunk            = cast(Arena*) memory;
        new_chunk->prev             = current; // link new chunk back to the old chunk
        new_chunk->base_pos         = current->base_pos + current->chunk_cap;
        new_chunk->chunk_cap        = new_reserve_size;
        new_chunk->chunk_pos        = MEM_INTERNAL_MIN_SIZE;
        new_chunk->chunk_commit_pos = MEM_INITIAL_COMMIT;
        // Update global allocator head pointer to the newly allocated chunk
        // and make sure the new chunk is used for the allocation
        current = arena->current = new_chunk;
      }
    }
  }

  {
    // if there is room in this chunk's reserve ...
    u64 result_pos     = AlignUpPow2(current->chunk_pos, arena->alignment);
    u64 next_chunk_pos = result_pos + size;
    if (next_chunk_pos <= current->chunk_cap) {

      // commit more memory if necessary
      if (next_chunk_pos > current->chunk_commit_pos) {
        // forward align
        u64 next_commit_pos_aligned = AlignUpPow2(next_chunk_pos, MEM_COMMIT_BLOCK_SIZE);
        // restrict to next commit to chunk capacity (if aligned position overshoots the chunk
        // capacity, to not commit past the reserved limit)
        u64 next_commit_pos = Min(next_commit_pos_aligned, current->chunk_cap);
        // take the difference of commit positions
        u64 commit_size = next_commit_pos - current->chunk_commit_pos;

        if (os_memory_commit((u8*) current + current->chunk_commit_pos, commit_size)) {
          arena->chunk_commit_pos = next_commit_pos;
        }
      }

      // if there is room in the commit range, return memory & advance pos
      if (next_chunk_pos <= current->chunk_commit_pos) {
        // AsanUnpoison((U8*) current + current->chunk_pos, next_chunk_pos - current->chunk_pos);
        result             = (u8*) current + result_pos;
        current->chunk_pos = next_chunk_pos;
      }
    }
  }

  return result;
}

// element is popped from the top of the stack and removed (?)
// void arena_pop_to(Arena* arena, u64 pos) {
//  if (pos < arena->base_pos) {
//    arena->base_pos = pos;
//
//    u64 p = arena->base_pos;
//    u64 p_aligned = AlignUpPow2(p, M_COMMIT_BLOCK_SIZE - 1);
//  }
//}

/**
 * wrapper for arena_push_no_zero to zero initialize the memory
 * this function memsets the allocated memory to 0.
 *
 * Set every byte in teh newly allocated block to 0.
 */
void* arena_push(Arena* arena, u64 size)
{
  void* result = arena_push_no_zero(arena, size);
  MemoryZero(result, size);
  return result;
}

internal Arena* arena_alloc_(u64 reserve_size, b32 growing)
{
  Arena* arena = 0;
  if (reserve_size >= MEM_INITIAL_COMMIT) {
    void* memory = os_memory_reserve(reserve_size);
    if (os_memory_commit(memory, MEM_INITIAL_COMMIT)) {
      arena                   = (Arena*) memory;
      arena->current          = arena;
      arena->prev             = 0;
      arena->alignment        = sizeof(void*);
      arena->growing          = cast(b8)growing;
      arena->base_pos         = 0;
      arena->chunk_cap        = reserve_size;
      arena->chunk_pos        = MEM_INTERNAL_MIN_SIZE;
      arena->chunk_commit_pos = MEM_INITIAL_COMMIT;
    }
  }

  assert(arena != 0);
  return arena;
}

/**
 * Setup function to initialize the arena:
 * reserves memory, commits initial head and initializes the header.
 * NOTE: It is important that this is called BEFORE `arena_push` or `arena_push_no_zero` !
 */

// #define arena_alloc() arena_alloc_(MEM_DEFAULT_RESERVE_SIZE, 1);
Arena* arena_alloc(void)
{
  Arena* result = arena_alloc_(MEM_DEFAULT_RESERVE_SIZE, 1);
  return result;
}
