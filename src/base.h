#include <stdint.h>
#include <stdlib.h>

#pragma once
#ifndef __BASELIB__
#define __BASELIB__
#endif

// searchable typecast
#define cast(type) (type)

#define internal static
#define local    static
#define global   static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef size_t usize;

// packed structs macro
// --------------------
// https://stackoverflow.com/a/3312896
// GCC  __attribute__((packed)) https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Packed-Structures.html
// MSVC pragma pack             https://learn.microsoft.com/en-us/cpp/preprocessor/pack?view=msvc-170
#ifdef __GNUC__ || __clang__
#  define PACK( __Declaration__ ) __Declaration__ __attribute__((packed))
#else //if _MSC_VER
#  define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif
