/* vi: foldmethod=marker
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#pragma once

/* Debug Print Macro for win32 and linux
 * Usage:
 *   DEBUG_LOG("Cursor positions - Play: %lu, Write: %lu", play_cursor, write_cursor);
 *   DEBUG_LOG("Sine wave phase: %f", sound_output.t_sine);
*/
#ifdef DEBUG
#  if defined(_WIN32)
    // Windows Debug Logger
#    define DEBUG_LOG(format, ...)                                        \
      do                                                                  \
      {                                                                   \
        char dbg_msg[512] = {0};                                          \
        snprintf(dbg_msg, sizeof(dbg_msg), format "\n", ##__VA_ARGS__);   \
        OutputDebugStringA(dbg_msg);                                      \
      } while (0)
#  elif defined(__linux__)
    // Linux Debug Logger
#    define DEBUG_LOG(format, ...)                                        \
      do                                                                  \
      {                                                                   \
        fprintf(stderr, "[DEBUG] " format "\n", ##__VA_ARGS__);           \
      } while (0)
#  else
    // Unknown OS Fallback
#    define DEBUG_LOG(format, ...)
#  endif
#else
  // If we are in Release mode, DEBUG_LOG does nothing.
  // The compiler will should erase these lines from the final build.
#  define DEBUG_LOG(format, ...)
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

typedef float f32;
typedef double f64;

typedef s8 b8;
typedef s16 b16;
typedef s32 b32;
typedef s64 b64;

typedef size_t usize;
typedef intptr_t ssize;
typedef uintptr_t uptr;

global const f32 PI_F32 = 3.14159265359f;

#define enum8(name)  u8
#define enum16(name) u16
#define enum32(name) u32
#define enum64(name) u64

// Minimum and Maximum values
#  define Min(a, b) ( ((a) < (b)) ? (a) : (b) )
#  define Max(a, b) ( ((a) > (b)) ? (a) : (b) )

/**
 * Clamp(value, min, max)
 * Limit the value to given minimal and maximal range
 */
#  define Clamp(value, min, max) ( ((value) < (min)) ? (min) : ((max) < (value)) ? (max) : (value) )

/**
 * ClampTop(value, limit)
 * Caps a value at a specified maximum ceiling.
 * Use when you do not care about a minimum floor limit.
 */
#  define ClampTop(value, limit) Min(value, limit)

// packed structs macro
// --------------------
// https://stackoverflow.com/a/3312896
// GCC  __attribute__((packed))
// https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Packed-Structures.html
// MSVC pragma pack
// https://learn.microsoft.com/en-us/cpp/preprocessor/pack?view=msvc-170
#if defined(__GNUC__) || defined(__clang__)
#  define PACK(__Declaration__) __Declaration__ __attribute__((packed))
#else // if _MSC_VER
#  define PACK(__Declaration__) \
    __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#endif
