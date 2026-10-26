#include "arena.h"

void* arena_push(Arena* arena, u64 size)
{
  if (size == 0)
  {
    return NULL;
  }

  u64 aligned_size    = ALIGN_POW2(size, ARENA_ALIGNMENT);
  u64 page_size       = os_page_size();
  Arena* active_block = arena->next;

  /* ---- 1. Slow Path: Reserve and chain a new block if the active one cannot fit the push ---- */
  if (active_block == NULL || (ARENA_HEADER_SIZE + active_block->cursor + aligned_size > active_block->reserved))
  {
    u64 base_block_size     = arena->block_size ? arena->block_size : ARENA_DEFAULT_BLOCK_SIZE;
    u64 required_block_size = ARENA_HEADER_SIZE + aligned_size;
    u64 max_block_size      = arena->max_block ? (arena->max_block + ARENA_HEADER_SIZE) : UINT64_MAX;

    /* Clamp default target to max cap, then ensure it is at least large enough for this allocation */
    u64 reserve_size = Max(required_block_size, ClampTop(base_block_size, max_block_size));
    reserve_size     = ALIGN_POW2(reserve_size, page_size);

    u64 commit_size = ALIGN_POW2(required_block_size, page_size);

    Arena* new_block = (Arena*)os_vm_reserve(reserve_size);
    if (new_block == NULL)
    {
      return NULL;
    }

    if (!os_vm_commit(new_block, commit_size))
    {
      os_vm_release(new_block, reserve_size);
      return NULL;
    }

    new_block->next       = arena->next;
    new_block->reserved   = reserve_size;
    new_block->committed  = commit_size;
    new_block->cursor     = 0;
    new_block->block_size = 0;
    new_block->max_block  = 0;

    arena->next  = new_block;
    active_block = new_block;

    /* Double target size for subsequent blocks, guarding against 64-bit overflow */
    u64 next_block_size = (base_block_size <= (UINT64_MAX / 2)) ? (base_block_size * 2) : UINT64_MAX;
    arena->block_size   = ClampTop(next_block_size, max_block_size);
  }
  /* ---- 2. Fast Path: Commit additional pages if the active block needs more physical RAM ---- */
  else
  {
    u64 required_bytes = ARENA_HEADER_SIZE + active_block->cursor + aligned_size;
    if (required_bytes > active_block->committed)
    {
      u64 commit_target = ALIGN_POW2(required_bytes, page_size);
      u8* commit_start  = (u8*)active_block + active_block->committed;
      u64 commit_size   = commit_target - active_block->committed;

      if (!os_vm_commit(commit_start, commit_size))
      {
        return NULL;
      }

      active_block->committed = commit_target;
    }
  }

  /* ---- 3. Sub-allocate from the active block and advance the cursor ---- */
  void* payload = (u8*)active_block + ARENA_HEADER_SIZE + active_block->cursor;
  active_block->cursor += aligned_size;

  return payload;
}

void* arena_push_zero(Arena* arena, u64 size)
{
  void* payload = arena_push(arena, size);
  if (payload != NULL)
  {
    MemoryZero(payload, ALIGN_POW2(size, ARENA_ALIGNMENT));
  }

  return payload;
}

void* arena_mark(Arena* arena)
{
  Arena* active_block = arena->next;
  if (active_block == NULL)
  {
    return NULL;
  }

  /* Mark is the exact memory address the next push will land on */
  return (u8*)active_block + ARENA_HEADER_SIZE + active_block->cursor;
}

void arena_pop(Arena* arena, void* mark)
{
  /* State before any pushes; release the entire chain */
  if (mark == NULL)
  {
    Arena* block = arena->next;
    while (block != NULL)
    {
      Arena* older_block = block->next;
      os_vm_release(block, block->reserved);
      block = older_block;
    }

    arena->next = NULL;
    return;
  }

  Arena* block = arena->next;
  u8* mark_ptr = (u8*)mark;

  while (block != NULL)
  {
    u8* payload_start = (u8*)block + ARENA_HEADER_SIZE;
    u8* block_end     = (u8*)block + block->reserved;

    if (mark_ptr >= payload_start && mark_ptr <= block_end)
    {
      /* Release all blocks allocated after the target block */
      while (arena->next != block)
      {
        Arena* newer_block = arena->next;
        arena->next        = newer_block->next;
        os_vm_release(newer_block, newer_block->reserved);
      }

      /* Rewind the cursor to the mark's offset within this block */
      block->cursor = (u64)(mark_ptr - payload_start);
      return;
    }

    block = block->next;
  }

  assert(0 && "Arena pop failed: mark was not found in any active block.");
}

void arena_reset(Arena* arena)
{
  Arena* newest_block = arena->next;
  if (newest_block == NULL)
  {
    return;
  }

  /* Release every block older than the newest block */
  Arena* block = newest_block->next;
  while (block != NULL)
  {
    Arena* older_block = block->next;
    os_vm_release(block, block->reserved);
    block = older_block;
  }

  /* The newest block becomes the sole block in the chain with a rewound cursor */
  newest_block->next   = NULL;
  newest_block->cursor = 0;

  /* Decommit excess physical RAM down to a single page to prevent memory hoarding */
  u64 page_size = os_page_size();
  if (newest_block->committed > page_size)
  {
    u8* decommit_start = (u8*)newest_block + page_size;
    u64 decommit_size  = newest_block->committed - page_size;

    os_vm_decommit(decommit_start, decommit_size);
    newest_block->committed = page_size;
  }
}

void arena_destroy(Arena* arena)
{
  arena_pop(arena, NULL);
}
