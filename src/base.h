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

// Byte constants
#  define KiB(x) ((x) << 10)
#  define MiB(x) ((x) << 20)
#  define GiB(x) ((x) << 30)
#  define TiB(x) ((u64) (x) << 40llu)

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

#define enum8(name)  u8
#define enum16(name) u16
#define enum32(name) u32
#define enum64(name) u64

/* https://en.cppreference.com/w/c/string/byte/memset : memset(ptr, value, size) */
#  define MemoryZero(ptr, size) memset((ptr), 0, (size))
#  define MemoryZeroStruct(ptr) MemoryZero((ptr), sizeof(*(ptr)))
#  define MemoryZeroArray(ptr)  MemoryZero((ptr), sizeof(ptr))
#  define MemoryZeroTyped(ptr, c) MemoryZero((ptr), sizeof(*(ptr)) * (c))

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

/**
 * This function takes an existing pointer (ptr) and moves it (forward) to a
 * higher address value untl it sits exactly on an address that is a multiple
 * of the specified align value.
 *
 * align an address forward
 * AlignUpPow2(u64 x, u64 A) = (x + A - 1) AND (-A) -> u64
 * ----------------------------
 * 1. create a mask: `~((ptr) - 1)`, ...10000 -> ...01111 (create a mask that clears the lower bits of the ptr, effectively rounding the number down to the nearest multiple of alignment)
 * 2. set all lower bits to 0: `(alignment - 1)`, this only works when the alignemnt is power of 2, subtracting one flips the bits from position of the single 1 downwards.
 * 3. flip the mask: `~((alignment) - 1)` all the higher order bits are set to 1 and the lower bits are set to 0.
 * 4. rounding up: `((x) + (p) - 1)`, ceiling division
 *
 * ((alignment) - 1) creates trailing ones, subtracting 1 from a power of two sets all the lower
 * bits to 1. This is done to generate the mask that identifies all the bits below the alignment
 * boundary.
 *
 * ~((alignment) - 1) creates trailing zeros (the mask), flips all of the bits to clear the
 * misalignment bits of the pointer address.
 *
 * 10000000 - 1 [8 bits]
 * ------------
 * 01111111     [8 bits]
 *
 * Decimal       =                                  128
 * 32-bit Binary =  00000000 00000000 00000000 10000000
 * Decimal       =                                  127
 * 32-bit Binary =  00000000 00000000 00000000 01111111
 * Decimal       =                                 ~127 (bitwise NOT)
 * 32-bit Binary =  11111111 11111111 11111111 10000000
 *
 * We force the lower bits to 0 by the subsequent bitwise NOT (~) and then the bitwise AND (&) of
 * the result with the address. & Mask forces the lower bits to 0 since the zeros in the mask ensure
 * that the corresponding bits in the address are cleared to 0, thus forcing the address to be
 * perfect multiple of the alignment.
 *
 * EXAMPLE:
 *
 * EXAMPLE:
 *   ptr       = 1128
 *   alignment = 4096
 *
 *   roundup = ((ptr) + (alignment) - 1) -> 5223                 [ 0000000000000000000000000000000000000000000000000001010001100111 ]
 *   mask    = ~((alignment) - 1)        -> 18446744073709547520 [ 1111111111111111111111111111111111111111111111111111000000000000 ]
 *   result  = roundup & mask            -> 4096                 [ 0000000000000000000000000000000000000000000000000001000000000000 ]
 */
#  define AlignUpPow2(ptr, alignment)   ( ((ptr) + (alignment) - 1) & ~((alignment) - 1) )
#  define AlignDownPow2(ptr, alignment) ( (ptr) & ~((alignment) - 1) )

/* Arena macros */
#  define MEM_MAX_ALIGN            64
#  define MEM_INITIAL_COMMIT       KiB(4)
#  define MEM_DEFAULT_RESERVE_SIZE GiB(1)
#  define MEM_COMMIT_BLOCK_SIZE    MiB(64)
#  define M_COMMIT_BLOCK_SIZE      MiB(64)
#  define MEM_INTERNAL_MIN_SIZE    AlignUpPow2(sizeof(Arena), MEM_MAX_ALIGN)

/**
 * Characer macro from Mr4th.
 * Packs four sequential ASCII characters into a single 32-bit unsigned integer.
 *
 * This allows us to read 4-byte chunk identifiers from a file (like "RIFF" or "data").
 *
 * ENDIANNESS (Little-Endian Architecture):
 * Modern CPUs (x86/x64) are little-endian, meaning the least significant byte (LSB)
 * of an integer is stored at the lowest memory address.
 *
 * To ensure the 32-bit integer matches the physical left-to-right byte order
 * on disk `[a, b, c, d]`, we must construct the integer backwards:
 *   - 'a' (byte 0) is the LSB (shifted by 0).
 *   - 'b' (byte 1) is shifted left into the 2nd byte slot (<< 8).
 *   - 'c' (byte 2) is shifted left into the 3rd byte slot (<< 16).
 *   - 'd' (byte 3) is the MSB, shifted into the highest byte slot (<< 24).
 *
 * EXAMPLE: AsciiID4('R', 'I', 'F', 'F')
 *   'F' (0x46) << 24 = 0x46000000
 *   'F' (0x46) << 16 = 0x00460000
 *   'I' (0x49) << 8  = 0x00004900
 *   'R' (0x52)       = 0x00000052
 *   -----------------------------
 *   Bitwise OR (|)   = 0x46464952 (Memory layout: 52 49 46 46 -> 'R' 'I' 'F' 'F')
 *
 * The explicit (u32) casts prevent accidental sign-extension and undefined
 * compiler behavior when shifting standard signed character literals.
 */
#define AsciiID4(a, b, c, d) ((cast(u32)(d) << 24) | (cast(u32)(c) << 16) | (cast(u32)(b) << 8) | (cast(u32)(a)))

///////////////////////// Basic Constants {{{

global const s8  MIN_S8  = (s8) 0x80;
global const s16 MIN_S16 = (s16) 0x8000;
global const s32 MIN_S32 = (s32) 0x80000000;
global const s64 MIN_S64 = (s64) 0x8000000000000000llu;

global const s8  MAX_S8  = (s8) 0x7f;
global const s16 MAX_S16 = (s16) 0x7fff;
global const s32 MAX_S32 = (s32) 0x7fffffff;
global const s64 MAX_S64 = (s64) 0x7fffffffffffffffllu;

global const u8  MAX_U8  = (u8) 0xff;
global const u16 MAX_U16 = (u16) 0xffff;
global const u32 MAX_U32 = (u32) 0xffffffff;
global const u64 MAX_U64 = (u64) 0xffffffffffffffffllu;

global const f32 MACHINE_EPSILON_F32    = 1.1920929e-7;
global const f32 PI_F32                 = 3.14159265359f;
global const f32 TAU_F32                = 6.28318530718f;
global const f32 E_F32                  = 2.71828182846f;
global const f32 GOLDEN_RATIO_BIG_F32   = 1.61803398875f;
global const f32 GOLDEN_RATIO_SMALL_F32 = 0.61803398875f;

global const f64 MACHINE_EPSILON_F64    = 2.220446e-16;
global const f64 PI_F64                 = 3.14159265359;
global const f64 TAU_F64                = 6.28318530718;
global const f64 E_F64                  = 2.71828182846;
global const f64 GOLDEN_RATIO_BIG_F64   = 1.61803398875;
global const f64 GOLDEN_RATIO_SMALL_F64 = 0.61803398875;

/////////////////////////////////
//// End of Basic Constants ////
/////////////////////////////// }}}
