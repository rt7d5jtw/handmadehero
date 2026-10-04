/* vi: foldmethod=marker
 */

#pragma once
#pragma once
#ifndef BASE_H
#  define BASE_H

///////////////////////// Foreign Includes {{{

#if defined(__linux__) || defined(OS_LINUX)
#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#  endif
#endif

#include <string.h>
#include <assert.h>
#include <stddef.h>

// Integer Types (handle missing <stdint.h> for Visual C++ 2008 and older)
#if defined(_MSC_VER) && (_MSC_VER <= 1500)
    typedef signed __int8      int8_t;
    typedef signed __int16     int16_t;
    typedef signed __int32     int32_t;
    typedef signed __int64     int64_t;
    typedef unsigned __int8    uint8_t;
    typedef unsigned __int16   uint16_t;
    typedef unsigned __int32   uint32_t;
    typedef unsigned __int64   uint64_t;
#  if defined(_WIN64)
    typedef signed __int64     intptr_t;
    typedef unsigned __int64   uintptr_t;
#  else
    typedef signed __int32     intptr_t;
    typedef unsigned __int32   uintptr_t;
#  endif
#else
#  include <stdint.h>
#endif

#if !defined(__cplusplus)
#  if defined(_MSC_VER) && (_MSC_VER >= 1800)
#    include <stdbool.h>
#  elif defined(_MSC_VER)
#    ifndef __bool_true_false_are_defined
       typedef unsigned char bool;
#      define true  1
#      define false 0
#      define __bool_true_false_are_defined 1
#    endif
#  elif (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__GNUC__) || defined(__clang__)
#    include <stdbool.h>
#  else
#    ifndef __bool_true_false_are_defined
       typedef unsigned char bool;
#      define true  1
#      define false 0
#      define __bool_true_false_are_defined 1
#    endif
#  endif
#endif

// }}} Foreign Includes

///////////////////////// Context Macros {{{

/*
 * Compiler macros:    https://sourceforge.net/p/predef/wiki/Compilers/
 * Language standards: https://sourceforge.net/p/predef/wiki/Standards/
 * Architectures:      https://sourceforge.net/p/predef/wiki/Architectures/
 * Operating systems:  https://sourceforge.net/p/predef/wiki/OperatingSystems/
 * Libraries:          https://sourceforge.net/p/predef/wiki/Libraries/
 *
 */

/* define compiler */
#  if defined(_MSC_VER)
#    define COMPILER_MSVC 1
#  elif defined(__clang__)
#    define COMPILER_CLANG 1
#  elif defined(__GNUC__)
#    define COMPILER_GCC 1
#  else
#    error no context for this compiler
#  endif

/* define operating system */
#  if defined(_WIN32)
#    define OS_WINDOWS 1
#  elif defined(__gnu_linux__) || defined(__linux__)
#    define OS_LINUX 1
#  elif defined(__APPLE__) && defined(__MACH__)
#    define OS_MAC 1
#  elif defined(BSD) && defined(__FreeBSD__)
#    define OS_FREEBSD 1
#  else
#    error missing os detection
#  endif

/* Architecture macros: https://wolfcon.github.io/Life/PreDefinedCC++CompilerMarcros.html */

/* define architecture */
#  if defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
#    define ARCH_X64 1
#  elif defined(__i386__) || defined(_M_IX86) || defined(_X86_)
#    define ARCH_X86 1
#  elif defined(__arm__) || defined(_M_ARM)
#    define ARCH_ARM 1
#  elif defined(__aarch64__) || defined(_M_ARM64)
#    define ARCH_ARM64 1
#  elif defined(__mips__)
#    define ARCH_MIPS 1
#  elif defined(__powerpc) || defined(_M_PPC)
#    define ARCH_PPC 1
#  else
#    error missing ARCH detection
#  endif

// Zero fill missing context macros
#  if !defined(COMPILER_MSVC)
#    define COMPILER_MSVC 0
#  endif
#  if !defined(COMPILER_CLANG)
#    define COMPILER_CLANG 0
#  endif
#  if !defined(COMPILER_GCC)
#    define COMPILER_GCC 0
#  endif
#  if !defined(COMPILER_TINYC)
#    define COMPILER_TINYC 0
#  endif
#  if !defined(OS_WINDOWS)
#    define OS_WINDOWS 0
#  endif
#  if !defined(OS_LINUX)
#    define OS_LINUX 0
#  endif
#  if !defined(OS_MAC)
#    define OS_MAC 0
#  endif
#  if !defined(OS_FREEBSD)
#    define OS_FREEBSD 0
#  endif
#  if !defined(ARCH_X64)
#    define ARCH_X64 0
#  endif
#  if !defined(ARCH_X86)
#    define ARCH_X86 0
#  endif
#  if !defined(ARCH_ARM)
#    define ARCH_ARM 0
#  endif
#  if !defined(ARCH_ARM64)
#    define ARCH_ARM64 0
#  endif
#  if !defined(ARCH_MIPS)
#    define ARCH_MIPS 0
#  endif
#  if !defined(ARCH_PPC)
#    define ARCH_PPC 0
#  endif
#  if !defined(ENABLE_ASSERT)
#    define ENABLE_ASSERT 0
#  endif
// End of if defined(_MSC_VER)

////////////////////////////
// End of Context Macros //
///////////////////////////  }}}

///////////////////////// Helper Macros {{{

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
#define read_only const

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

///////////////////////////
// End of Helper Macros //
///////////////////////// }}}

///////////////////////// Basic Types {{{

// Short names for primitive types

// https://cplusplus.com/reference/cstdint/
// https://en.cppreference.com/w/c/types/integer

// Signed with bitcount
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// Unsigned with bitcount
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Booleans with bitcount
typedef s8  b8;
typedef s16 b16;
typedef s32 b32;
typedef s64 b64;

// Floats with bitcount
typedef float  f32;
typedef double f64;

typedef size_t usize;
typedef intptr_t ssize;
typedef uintptr_t uptr;

typedef char         byte;
typedef unsigned int uint;

// define UTF-16
// https://en.cppreference.com/w/c/string/multibyte/char16_t
// NOTE: uchar.h requires C11 and is not supported for Mac
typedef uint16_t c16;

//////////////////////////
// End of Basic Types ///
//////////////////////// }}}

///////////////////////// Basic Constants {{{

global const s8  MIN_S8  = (s8) -128;
global const s16 MIN_S16 = (s16) -32768;
global const s32 MIN_S32 = (s32) -2147483647 - 1;
global const s64 MIN_S64 = -9223372036854775807LL - 1;

global const s8  MAX_S8  = (s8) 0x7f;
global const s16 MAX_S16 = (s16) 0x7fff;
global const s32 MAX_S32 = (s32) 0x7fffffff;
global const s64 MAX_S64 = 0x7fffffffffffffffLL;

global const u8  MAX_U8  = (u8) 0xff;
global const u16 MAX_U16 = (u16) 0xffff;
global const u32 MAX_U32 = (u32) 0xffffffff;
global const u64 MAX_U64 = 0xffffffffffffffffULL;

global const f32 MACHINE_EPSILON_F32    = 1.1920929e-7f;
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

#endif // BASE_H
