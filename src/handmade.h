#ifndef HANDMADE_H
#define HANDMADE_H

#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "base.h"
#include "os.h"
#include "arena.h"
#include "wav.h"

#if defined(_WIN32)
#  include <windows.h>
#  include <dsound.h>
#  include <xinput.h>
#elif defined(__linux__)
#  include <sys/mman.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#  include <time.h>
#  include <x86intrin.h>

#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/keysym.h>

#  include <alsa/asoundlib.h>
#else
#  warning "Unknown platform!"
#endif

typedef struct AudioBuffer AudioBuffer;
struct AudioBuffer
{
  s32 samples_per_second;
  s32 sample_count;
  s32 tone_hz;
  s32 tone_volume;
  s16* samples;
};

typedef struct OffscreenBuffer OffscreenBuffer;
struct OffscreenBuffer
{
  void* memory;
  u32 width;
  u32 height;
  u32 bytes_per_pixel;
};

/* Cursed Debug Print Macro for win32 and linux
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

#endif // HANDMADE_H
