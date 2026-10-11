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

/*
 * NOTE: Windows GDI uses BGRA (blue, green, red, alpha) format
 */
void draw_random_gradient(
    u32 bytes_per_pixel,
    u32* bitmap_memory,
    u32 bitmap_width,
    u32 bitmap_height,
    u32 x_offset,
    u32 y_offset
)
{
  assert(bitmap_memory != NULL && "GDI Bitmap memory buffer must not be null!");
  assert(
      bitmap_width > 0 && bitmap_height > 0 &&
      "GDI Bitmap dimensions must be greater than zero!"
  );

  /* stride or pitch: the total number of bytes in one horizontal line of the bitmap */
  u32 pitch = bitmap_width * bytes_per_pixel;
  /* pointer to the first byte of the current row being processed */
  u8* row = (u8*)bitmap_memory;

  /* Iterate through each row (y-coordinate) */
  for (u32 y = 0; y < bitmap_height; ++y)
  {
    // first byte of the current pixel in the row
    u32* pixel = (u32*)row;
    /* Iterate through each pixel in the current row (x-coordinate) */
    for (u32 x = 0; x < bitmap_width; ++x)
    {
      u8 blue  = (u8)(x + x_offset);
      u8 green = (u8)(y + y_offset);
      u8 red   = 0;
      u8 alpha = 0;

      // pack all four channels into the u32 in BGRA order
      u32 packed_color_channels = (alpha << 24) | (red << 16) | (green << 8) | blue;

      // Write all the color channels for the pixel for this byte, and advance
      // the 4-byte pointer. NOTE: post-increment pixel++, after the assignment
      // is complete, advance the pixel pointer to the next memory location.
      *pixel++ = packed_color_channels;
    }

    // Move the row pointer down by the pitch (stride) to start the next row
    row += pitch;
  }
}

void draw_snow(u32* bitmap_memory, u32 bitmap_width, u32 bitmap_height)
{
  assert(bitmap_memory != NULL && "GDI Bitmap memory buffer must not be null!");
  assert(
      bitmap_width > 0 && bitmap_height > 0 &&
      "GDI Bitmap dimensions must be greater than zero!"
  );

  u32 pitch = bitmap_width * 4;
  u8* row   = (u8*)bitmap_memory;

  for (u32 y = 0; y < bitmap_height; ++y)
  {
    u32* pixel = (u32*)row;

    for (u32 x = 0; x < bitmap_width; ++x)
    {
      u32 bits = xorshift32();
      u8 noise = (u8)(bits & (bits >> 8)) & (bits >> 16);

      u8 blue  = noise;
      u8 green = noise;
      u8 red   = noise;
      u8 alpha = 0;

      // Pack channels into BGRA
      u32 packed_channels = (alpha << 24) | (red << 16) | (green << 8) | blue;
      *pixel++ = packed_channels;
    }
    row += pitch;
  }
}

#if defined(_WIN32)
#  include "win32_handmade.c"
#elif defined(__linux__)
#  include "linux_handmade.c"
#endif
