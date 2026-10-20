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

// NOTE: used to be `output_sound(SoundOutputBuffer sound_buffer)`
internal void fill_audio_buffer(AudioBuffer* audio_buffer)
{
  local f32 sine_phase = 0.0f;

  s16* sample_cursor = audio_buffer->samples;
  s32  wave_period   = audio_buffer->samples_per_second / audio_buffer->tone_hz;

  for (s32 sample_index = 0; sample_index < audio_buffer->sample_count; sample_index += 1)
  {
    f32 sine_value   = sinf(sine_phase);
    s16 sample_value = cast(s16)(sine_value * audio_buffer->tone_volume);

    *sample_cursor++ = sample_value; // left channel
    *sample_cursor++ = sample_value; // right channel

    sine_phase += TAU_F32 / (f32)wave_period;
    if (sine_phase > TAU_F32)
    {
      sine_phase -= TAU_F32;
    }
  }
}

/*
 * NOTE: Windows GDI uses BGRA (blue, green, red, alpha) format
 */
internal void draw_random_gradient(
    OffscreenBuffer* offscreen_buffer,
    u32 x_offset,
    u32 y_offset
)
{
  assert(offscreen_buffer->memory != NULL && "GDI Bitmap memory buffer must not be null!");
  assert(
      offscreen_buffer->width > 0 && offscreen_buffer->height > 0 &&
      "GDI Bitmap dimensions must be greater than zero!"
  );

  /* stride or pitch: the total number of bytes in one horizontal line of the bitmap */
  u32 pitch = offscreen_buffer->width * offscreen_buffer->bytes_per_pixel;
  /* pointer to the first byte of the current row being processed */
  u8* row = (u8*)offscreen_buffer->memory;

  u32 height = offscreen_buffer->height;
  u32 width  = offscreen_buffer->width;

  /* Iterate through each row (y-coordinate) */
  for (u32 y = 0; y < height; ++y)
  {
    // first byte of the current pixel in the row
    u32* pixel = (u32*)row;
    /* Iterate through each pixel in the current row (x-coordinate) */
    for (u32 x = 0; x < width; ++x)
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

internal void draw_snow(u32* bitmap_memory, u32 bitmap_width, u32 bitmap_height)
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

internal void game_update_and_render(
    AudioBuffer* audio_buffer,
    OffscreenBuffer* offscreen_buffer,
    u32 y_offset,
    u32 x_offset
)
{
  fill_audio_buffer(audio_buffer);
  draw_random_gradient(
    offscreen_buffer,
    x_offset,
    y_offset
  );
}

#if defined(_WIN32)
#  include "win32_handmade.c"
#elif defined(__linux__)
#  include "linux_handmade.c"
#endif
