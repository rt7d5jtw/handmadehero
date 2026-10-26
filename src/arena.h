
#ifndef ARENA_H
#define ARENA_H

#include "base.h"

/*
 * Commit-style growing and chaining arena allocator.
 * Inspired by Allen Webster and Ryan Fleury.
 */

#define ARENA_ALIGNMENT          ((u64)16) /* bytes, not KiB-scale */
#define ARENA_DEFAULT_BLOCK_SIZE KiB(64)

typedef struct Arena Arena;
struct Arena
{
  Arena* next;       /* older block; NULL on the oldest block                       */
  u64    reserved;   /* per block: total address space held (page-aligned)          */
  u64    committed;  /* per block: bytes currently backed by RAM (page-aligned)     */
  u64    cursor;     /* per block: byte offset from payload start to next free byte */

  u64    block_size; /* root only: reservation size for the next new block          */
  u64    max_block;  /* root only: growth cap, 0 = unbounded                        */
};

/* Core Allocation API */
void* arena_push     (Arena* arena, u64 size);
void* arena_push_zero(Arena* arena, u64 size);

/* Lifecycle & Scoped Memory API */
void* arena_mark     (Arena* arena);
void  arena_pop      (Arena* arena, void* mark);
void  arena_reset    (Arena* arena);
void  arena_destroy  (Arena* arena);

/* Uninitialized Helper Macros (fast path for buffers overwritten immediately) */
#define PushArray(arena, type, count)     (type*)arena_push((arena), sizeof(type) * (count))
#define PushStruct(arena, type)           PushArray((arena), type, 1)

/* Zero-Initialized Helper Macros (safe default for general structs and arrays) */
#define PushArrayZero(arena, type, count) (type*)arena_push_zero((arena), sizeof(type) * (count))
#define PushStructZero(arena, type)       PushArrayZero((arena), type, 1)

/* Rounds an address or size up to the next power-of-2 boundary */
#define ALIGN_POW2(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

#define ARENA_HEADER_SIZE ALIGN_POW2(sizeof(Arena), ARENA_ALIGNMENT)

#endif /* ARENA_H */
