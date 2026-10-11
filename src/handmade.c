/* vi: foldmethod=marker
 */

#include "handmade.h"

// Unity build includes
#include "os.c"
#include "arena.c"
#include "wav.c"

/* main loop */
global b32 running = true;
global u32 active_prng_seed = 123456789;
// max resolution: 3840 * 2160 * 4 bytes = ~33 MB
global read_only s32 MAX_WIDTH = 3840;
global read_only s32 MAX_HEIGHT = 2160;
// 4 bytes for 32-bit color depth (e.g. BGRA)
global read_only u32 BYTES_PER_PIXEL = 4;

/* Debug Print Macro for win32 and linux
 * Usage:
 *   DEBUG_LOG("Cursor positions - Play: %lu, Write: %lu", play_cursor, write_cursor);
 *   DEBUG_LOG("Sine wave phase: %f", sound_output.t_sine);
*/
#ifdef DEBUG
#  if defined(_WIN32)
   __declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);
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

// Xorshift RNGs
// https://excamera.com/sphinx/article-xorshift.html
u32 xorshift32(void)
{
  u32 x = active_prng_seed;

  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;

  active_prng_seed = x;

  return x;
}

#if defined(_WIN32)
#  include "win32_handmade.c"
#elif defined(__linux__)
#  include "linux_handmade.c"
#endif
