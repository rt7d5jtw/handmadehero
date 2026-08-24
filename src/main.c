/* vi: foldmethod=marker
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "base.h"

global u32 active_prng_seed = 123456789;

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

#if defined(_WIN32) // Windows code {{{

#  define _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_DEPRECATE

#  ifndef UNICODE
#    define UNICODE
#  endif

#  include <dsound.h>
#  include <windows.h>
#  include <xinput.h>

#  define WINDOW_WIDTH  1480
#  define WINDOW_HEIGHT 860

#  define GetlParamX(lp) ((int)(short)LOWORD(lp))
#  define GetlParamY(lp) ((int)(short)HIWORD(lp))

// DirectSound variables
global IDirectSoundBuffer* secondary_directsound_buffer;

/*
  Device-Independent Bitmaps (DIB)
  https://learn.microsoft.com/en-us/windows/win32/gdi/device-independent-bitmaps
*/

// START OF GDI Drawing declarations {{{

typedef struct Win32OffscreenBuffer Win32OffscreenBuffer;
struct Win32OffscreenBuffer {
  BITMAPINFO info;
  void* pixels; // pixel array for the bitmap
  u32 width;
  u32 height;
  u32 pitch;
  u32 bytes_per_pixel;
  HBITMAP bitmap_handle;
  HDC frame_device_context;
};

struct Win32OffscreenBuffer win32_offscreen_buffer = {.bytes_per_pixel = 4};
static int client_width;

typedef struct Win32WindowDimensions Win32WindowDimensions;
struct Win32WindowDimensions {
  u32 width;
  u32 height;
};

typedef struct Win32SoundOutput Win32SoundOutput;
struct Win32SoundOutput {
  int samples_per_second;
  int bytes_per_sample;
  int secondary_buffer_size;
  u32 running_sample_index;
  int tone_hz;
  int tone_volume;
  int wave_period;
  f32 t_sine;
  int latency_sample_count;
};

// clang-format off
#  define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global x_input_get_state* XInputGetState_ = XInputGetStateStub;
#  define XInputGetState XInputGetState_

#  define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global x_input_get_state* XInputSetState_ = XInputSetStateStub;
#  define XInputSetState XInputSetState_
// clang-format on

internal void win32_load_xinput(void)
{
  HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (!xInputLibrary)
  {
    xInputLibrary = LoadLibraryA("Xinput1_3.dll");
  }
  if (!xInputLibrary)
  {
    xInputLibrary = LoadLibraryA("Xinput9_1_0.dll");
  }

  if (xInputLibrary)
  {
    XInputGetState =
        (x_input_get_state*)GetProcAddress(xInputLibrary, "XInputGetState");
    if (!XInputGetState)
    {
      XInputGetState = XInputGetStateStub;
    }

    XInputSetState =
        (x_input_set_state*)GetProcAddress(xInputLibrary, "XInputSetState");
    if (!XInputSetState)
    {
      XInputSetState = XInputSetStateStub;
    }
  }
}

// DirectSoundCreate
// https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708921(v=vs.85)
#  define DIRECT_SOUND_CREATE(name)                             \
    HRESULT WINAPI name(                                        \
        LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter \
    );
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void win32_init_directsound(
    HWND window_handle,
    s32 samples_per_second,
    s32 buffer_size
)
{
  HMODULE directsound_library = LoadLibraryA("dsound.dll");
  if (directsound_library)
  {
    // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708921(v=vs.85)
    direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(directsound_library, "DirectSoundCreate");
    IDirectSound* DirectSound;

    // Create and Initialize IDirectSound interface
    // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708921(v=vs.85)
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
    {
      WAVEFORMATEX wave_format    = {0};
      wave_format.wFormatTag      = WAVE_FORMAT_PCM;
      wave_format.nChannels       = 2;
      wave_format.nSamplesPerSec  = samples_per_second;
      wave_format.wBitsPerSample  = 16;
      wave_format.nBlockAlign     = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
      wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;

      // SetCooperativeLevel
      // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708948(v=vs.85)#remarks
      HRESULT directsound_cooperative_level = DirectSound->lpVtbl->SetCooperativeLevel(DirectSound, window_handle, DSSCL_PRIORITY);
      if (SUCCEEDED(directsound_cooperative_level))
      {
        // Set buffer configuration for primary buffer
        // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416820(v=vs.85)
        DSBUFFERDESC buffer_description = {0};
        buffer_description.dwSize       = sizeof(buffer_description);
        buffer_description.dwFlags      = DSBCAPS_PRIMARYBUFFER;

        // Create a primary buffer
        IDirectSoundBuffer* primary_sound_buffer;

        // Create DirectSoundBuffer object to hold a sequence of audio samples
        // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708943(v=vs.85)
        HRESULT directsound_buffer = DirectSound->lpVtbl->CreateSoundBuffer(DirectSound, &buffer_description, &primary_sound_buffer, 0);
        if (SUCCEEDED(directsound_buffer))
        {
          // SetFormat - sets the format of the primary sound buffer
          // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708936(v=vs.85)
          HRESULT directsound_format = primary_sound_buffer->lpVtbl->SetFormat(primary_sound_buffer, &wave_format);
          if (SUCCEEDED(directsound_format))
          {
            OutputDebugStringA("Primary DirectSound buffer format was set.\n");
          }
        }
      }

      // Create a secondary buffer
      DSBUFFERDESC buffer_description  = {0};
      buffer_description.dwSize        = sizeof(buffer_description);
      buffer_description.dwBufferBytes = buffer_size;
      buffer_description.lpwfxFormat   = &wave_format;

      HRESULT secondary_buffer = DirectSound->lpVtbl->CreateSoundBuffer(DirectSound, &buffer_description, &secondary_directsound_buffer, 0);
      if (SUCCEEDED(secondary_buffer))
      {
        OutputDebugStringA("Secondary DirectSound buffer created Successfully.\n");
      }
    }
  }
}

internal Win32WindowDimensions win32_get_window_dimensions(HWND window_handle)
{
  Win32WindowDimensions dims = {0};
  RECT client_rect;
  GetClientRect(window_handle, &client_rect);

  dims.width  = client_rect.right - client_rect.left;
  dims.height = client_rect.bottom - client_rect.top;

  return dims;
}

internal void win32_fill_sound_buffer(
    Win32SoundOutput* sound_output,
    DWORD byte_to_lock,
    DWORD bytes_to_write
)
{
  VOID* region1;
  DWORD region1_size;
  VOID* region2;
  DWORD region2_size;

  HRESULT directsound_lock = secondary_directsound_buffer->lpVtbl->Lock(
      secondary_directsound_buffer,
      byte_to_lock,
      bytes_to_write,
      &region1,
      &region1_size,
      &region2,
      &region2_size,
      0
  );

  if (SUCCEEDED(directsound_lock))
  {
    s16* sample_out            = (s16*)region1;
    DWORD region1_sample_count = region1_size / sound_output->bytes_per_sample;
    for (DWORD sampleIndex = 0; sampleIndex < region1_sample_count; sampleIndex += 1)
    {
      f32 sine_value   = sinf(sound_output->t_sine);
      s16 sample_value = (s16)(sine_value * sound_output->tone_volume);
      *sample_out++    = sample_value;
      *sample_out++    = sample_value;

      sound_output->t_sine += (2.0f * PI_F32 * 1.0f) / (f32)sound_output->wave_period;
      ++sound_output->running_sample_index;
    }
    sample_out                 = (s16*)region2;
    DWORD region2_sample_count = region2_size / sound_output->bytes_per_sample;
    for (DWORD sampleIndex = 0; sampleIndex < region2_sample_count; sampleIndex += 1)
    {
      f32 sine_value   = sinf(sound_output->t_sine);
      s16 sample_value = (s16)(sine_value * sound_output->tone_volume);
      *sample_out++    = sample_value;
      *sample_out++    = sample_value;

      sound_output->t_sine += (2.0f * PI_F32 * 1.0f) / (f32)sound_output->wave_period;
      ++sound_output->running_sample_index;
    }

    secondary_directsound_buffer->lpVtbl->Unlock(secondary_directsound_buffer, region1, region1_size, region2, region2_size);
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

/*
 * This uses a technique to pack 4 bytes into one 32-bit integer (BGRA order).
 * This is done with bitwise shifts and OR operations.
 * NOTE: Windows GDI uses BGRA (blue, green, red, alpha) format
 */
void draw_random_gradient(
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
  u32 pitch = bitmap_width * win32_offscreen_buffer.bytes_per_pixel;
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
      // Blue channel
      u8 blue = (u8)(x + x_offset);
      // Green channel
      u8 green = (u8)(y + y_offset);
      // Red channel
      u8 red = 0;
      // Alpha channel
      u8 alpha = 0;

      // pack all four channels into the u32 in BGRA order
      u32 packed_channels = (alpha << 24) | (red << 16) | (green << 8) | blue;

      // Write all the color channels for the pixel for this byte, and advance
      // the 4-byte pointer. NOTE: post-increment pixel++, after the assignment
      // is complete, advance the pixel pointer to the next memory location.
      *pixel++ = packed_channels;
    }

    // Move the row pointer down by the pitch (stride) to start the next row
    row += pitch;
  }
}

typedef wchar_t wchar;

void draw_pixel(int x, int y, u32 color)
{
  DEBUG_LOG("Drawing pixel at [X: %d, Y: %d] with color: %u", x, y, color);

  u32* pixel = win32_offscreen_buffer.pixels;
  pixel += y * win32_offscreen_buffer.width + x;
  *pixel = color;
}

/* frees previous bitmap, allocates a new bitmap buffer, initializes and sets it up */
internal void win32_resize_dib_section(
    Win32OffscreenBuffer* offscreen_buffer,
    int width,
    int height
)
{
  if (offscreen_buffer->pixels)
  {
    VirtualFree(offscreen_buffer->pixels, 0, MEM_RELEASE);
  }

  offscreen_buffer->width           = width;
  offscreen_buffer->height          = height;
  offscreen_buffer->bytes_per_pixel = 4;

  offscreen_buffer->info.bmiHeader.biSize        = sizeof(offscreen_buffer->info.bmiHeader);
  offscreen_buffer->info.bmiHeader.biWidth       = offscreen_buffer->width;
  offscreen_buffer->info.bmiHeader.biHeight      = (s32)(offscreen_buffer->height);
  offscreen_buffer->info.bmiHeader.biPlanes      = 1;
  offscreen_buffer->info.bmiHeader.biBitCount    = 32;
  offscreen_buffer->info.bmiHeader.biCompression = BI_RGB;

  int bitmap_memory_size = offscreen_buffer->bytes_per_pixel *
                           (offscreen_buffer->width * offscreen_buffer->height);

  offscreen_buffer->pixels = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

internal void
win32_resize_bitmap(Win32OffscreenBuffer* offscreen_buffer, LPARAM lParam)
{
  offscreen_buffer->info.bmiHeader.biWidth  = LOWORD(lParam);
  offscreen_buffer->info.bmiHeader.biHeight = -HIWORD(
      lParam
  );
  // DIBs are based in a coordinate system that is upside down relative to Windows, source:
  // https://learn.microsoft.com/en-us/previous-versions/ms969901(v=msdn.10)?redirectedfrom=MSDN

  // Delete already existing bitmap
  if (offscreen_buffer->bitmap_handle)
  {
    DeleteObject(offscreen_buffer->bitmap_handle);
  }

  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createdibsection
  // Create a bitmap

  // hdc      - Handle to a device context.
  // pbmi     - Pointer to bitmap info.
  // usage    - type of data contained in the bmiColors array member of the BITMAPINFO structure pointed to by pbmi.
  // ppvBits  - a pointer to a variable that receives a pointer ot the location of the DIB bit values.
  // hSection - a handle to a file-mapping object that hte function will use to create the DIB.
  // offset   - the offset form the beginning of the file-mapping object referenced by hSection where storage for the bitmap bit values is to begin.

  offscreen_buffer->bitmap_handle = CreateDIBSection(
      NULL,
      &offscreen_buffer->info,
      DIB_RGB_COLORS,
      (void**)&offscreen_buffer->pixels,
      0,
      0
  );

  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectobject
  // point device context to the bitmap
  SelectObject(
      offscreen_buffer->frame_device_context, offscreen_buffer->bitmap_handle
  );

  offscreen_buffer->width  = LOWORD(lParam);
  offscreen_buffer->height = HIWORD(lParam);
}

internal void
win32_paint_bitmap(HWND window_handle, PAINTSTRUCT paint, HDC device_context)
{
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint
  device_context = BeginPaint(
      window_handle,
      &paint
  );

  // NOTE: origin (0,0) is conventionally located at the top-left corner for
  // Windows GDI.

  /* https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt
   * Painting function to copy the pixel array over to the window in the
   * specified rectangle. Performs a bit-block transfer between two device
   * contexts.
   */
  BitBlt(
      /* destination device context */
      device_context,
      /* x coordinate of the top-left corner of the destination rectangle */
      paint.rcPaint.left,
      /* y coordinate of the top-left corner of the destination rectangle */
      paint.rcPaint.top,
      /* width and height */
      paint.rcPaint.right - paint.rcPaint.left,
      paint.rcPaint.bottom - paint.rcPaint.top,
      /* source device context */
      win32_offscreen_buffer.frame_device_context,
      /* x coordinate of the top-left corner of the source rectangle */
      paint.rcPaint.left,
      /* y coordinate of the top-left corner of the source rectangle */
      paint.rcPaint.top,
      /* copy from source bitmap to destination bitmap */
      SRCCOPY
  );

  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint
  EndPaint(window_handle, &paint);
}

internal void win32_display_buffer_in_window(
    Win32OffscreenBuffer* buffer,
    HDC device_context,
    u32 width,
    u32 height
)
{
  /*
   * Copies Device-Independent Bitmap (DIB) (an pixel array) over to destination
   * bitmap. has stretching and compressing capabilities.
   */
  StretchDIBits(
      device_context,
      /*
       * xDest, yDest, destWidth, destHeight
       * xSrc,  ySrc,  srcWidth,  srcHeight
       */
      0,
      0,
      width,
      height,
      0,
      0,
      buffer->width,
      buffer->height,
      /* Source DIB bits (the raw pixel array) */
      buffer->pixels,
      /* Pointer to the BITMAPINFO structure for pixel format */
      &buffer->info,
      DIB_RGB_COLORS,
      SRCCOPY
  );
}

/// }}}

/* main loop */
b32 running = true;

/* Forward declaration */
LRESULT CALLBACK win32WndProc(HWND, UINT, WPARAM, LPARAM);

/**
 * Entrypoint for Windows
 * https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-winmain
 * https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
 */
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR pCmdLine,
    int nCmdShow
)
{
  // https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency
  LARGE_INTEGER perf_count_frequency_result;
  QueryPerformanceFrequency(&perf_count_frequency_result);
  s64 perf_count_frequency = perf_count_frequency_result.QuadPart;

  (void)pCmdLine;
  (void)hPrevInstance;

  // WNDCLASS
  // https://learn.microsoft.com/en-us/previous-versions/ms942860(v=msdn.10)
  // WNDCLASSA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassa
  // WNDCLASSEXA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassexa

  // Contains window class information
  WNDCLASSEX windowClass = {0};

  // https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window
  wchar_t const window_class_name[] = L"Sample Window Class";

  HWND window_handle = NULL;
  static MSG msg     = {0};

  win32_resize_dib_section(&win32_offscreen_buffer, 1280, 720);

  windowClass.cbSize        = sizeof(WNDCLASSEX);
  windowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  windowClass.lpszClassName = window_class_name;
  windowClass.lpfnWndProc   = win32WndProc; // Long Pointer to the Windows Procedure function
  windowClass.cbClsExtra    = 0;
  windowClass.cbWndExtra    = 0;
  windowClass.hInstance     = hInstance;
  windowClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  windowClass.lpszMenuName  = NULL;
  windowClass.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

  if (!RegisterClassEx(&windowClass))
  {
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox
    MessageBox(
        window_handle,
        L"Window Registration Failed!",
        L"Error!",
        MB_ICONEXCLAMATION | MB_OK
    );

    return -1;
  }

  // GDI Drawing code initialization {{{

  // Dimensions and color information for the bitmap
  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfo
  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
  win32_offscreen_buffer.info.bmiHeader.biSize        = sizeof(win32_offscreen_buffer.info.bmiHeader); // the number of bytes required by the structure
  win32_offscreen_buffer.info.bmiHeader.biPlanes      = 1; // the number of planes for the target device, must be set to 1
  win32_offscreen_buffer.info.bmiHeader.biBitCount    = 32; // the number of of bits per pixel (bpp)
  win32_offscreen_buffer.info.bmiHeader.biCompression = BI_RGB; // uncompressed RGB format

  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createcompatibledc
  win32_offscreen_buffer.frame_device_context = CreateCompatibleDC(0);

  /// }}}

  // window parameters
  DWORD extended_window_style = WS_EX_CLIENTEDGE;
  LPCWSTR window_name         = L"The title of my window";
  DWORD window_style          = WS_OVERLAPPEDWINDOW;
  int window_x                = CW_USEDEFAULT; // horizontal position of the window
  int window_y                = CW_USEDEFAULT; // vertical position of the window
  int window_width            = WINDOW_WIDTH;
  int window_height           = WINDOW_HEIGHT;
  HWND window_parent          = NULL;
  HMENU window_menu           = NULL;
  LPVOID lp_param             = NULL;

  // Window handle for
  window_handle = CreateWindowEx(
      extended_window_style,
      window_class_name,
      window_name,
      window_style,
      window_x,
      window_y,
      window_width,
      window_height,
      window_parent,
      window_menu,
      hInstance,
      lp_param
  );

  if (window_handle == NULL)
  {
    MessageBox(
        window_handle,
        L"Window Creation Failed!",
        L"Error!",
        MB_ICONEXCLAMATION | MB_OK
    );
    return GetLastError();
  }

  // ShowWindow
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
  ShowWindow(window_handle, nCmdShow);

  RECT rect;
  GetClientRect(window_handle, &rect);
  client_width = rect.right - rect.left;

  u32 x_offset = 0;
  u32 y_offset = 0;

  Win32SoundOutput sound_output      = {0};
  sound_output.samples_per_second    = 48000;
  sound_output.bytes_per_sample      = sizeof(s16) * 2;
  sound_output.secondary_buffer_size = 2 * sound_output.samples_per_second * sound_output.bytes_per_sample;
  sound_output.running_sample_index  = 0;
  sound_output.tone_hz               = 256;
  sound_output.tone_volume           = 3000;
  sound_output.wave_period           = sound_output.samples_per_second / sound_output.tone_hz;
  sound_output.latency_sample_count  = sound_output.samples_per_second / 15;

  b32 sound_is_playing = 0;

  win32_init_directsound(window_handle, sound_output.samples_per_second, sound_output.secondary_buffer_size);
  win32_fill_sound_buffer(&sound_output, 0, (sound_output.latency_sample_count * sound_output.bytes_per_sample));

  if (!sound_is_playing)
  {
    secondary_directsound_buffer->lpVtbl->Play(secondary_directsound_buffer, 0, 0, DSBPLAY_LOOPING);
    sound_is_playing = 1;
  }

  LARGE_INTEGER last_counter;
  QueryPerformanceCounter(&last_counter);

  while (running)
  {
    HDC device_context = GetDC(window_handle);

    // Run the message loop
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagew
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dispatchmessagea
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
    {
      if (msg.message == WM_QUIT)
      {
        running = false;
      }

      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // Xinput API
    for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT;
         controllerIndex += 1)
    {
      XINPUT_STATE controllerState;
      if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
      {
        // Controller is plugged in
        XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

        b32 up           = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
        b32 down         = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
        b32 left         = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
        b32 right        = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
        b32 start        = pad->wButtons & XINPUT_GAMEPAD_START;
        b32 back         = pad->wButtons & XINPUT_GAMEPAD_BACK;
        b32 leftShoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
        b32 righthoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
        b32 buttonA      = pad->wButtons & XINPUT_GAMEPAD_A;
        b32 buttonB      = pad->wButtons & XINPUT_GAMEPAD_B;
        b32 buttonX      = pad->wButtons & XINPUT_GAMEPAD_X;
        b32 buttonY      = pad->wButtons & XINPUT_GAMEPAD_Y;

        s16 stickX = pad->sThumbLX;
        s16 stickY = pad->sThumbLY;

        x_offset += stickX / 4096;
        y_offset += stickY / 4096;

        sound_output.tone_hz     = 512 + (int)(256.0f * ((f32)stickY / 30000.0f));
        sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
      }
      else
      {
        // Controller is not available
      }

      // test xinput vibration
      // XINPUT_VIBRATION xinput_vibration;
      // xinput_vibration.wLeftMotorSpeed  = 60000;
      // xinput_vibration.wRightMotorSpeed = 60000;
      // XInputSetState(0, &xinput_vibration);
    }

    // GDI Drawing logic

    //draw_random_gradient(win32_offscreen_buffer.pixels, win32_offscreen_buffer.width, win32_offscreen_buffer.height, x_offset, y_offset);
    draw_snow(
        win32_offscreen_buffer.pixels,
        win32_offscreen_buffer.width,
        win32_offscreen_buffer.height
    );

    // DirectSound API logic
    DWORD play_cursor;
    DWORD write_cursor;

    HRESULT directsound_cursor_positions = secondary_directsound_buffer->lpVtbl->GetCurrentPosition(secondary_directsound_buffer, &play_cursor, &write_cursor);
    if (sound_is_playing && SUCCEEDED(directsound_cursor_positions))
    {
      DWORD byte_to_lock  = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.secondary_buffer_size;
      DWORD target_cursor = ((play_cursor + (sound_output.latency_sample_count * sound_output.bytes_per_sample)) % sound_output.secondary_buffer_size);
      DWORD bytes_to_write;

      if (byte_to_lock > target_cursor)
      {
        // play cursor is behind
        bytes_to_write = sound_output.secondary_buffer_size - byte_to_lock; // region 1
        bytes_to_write += target_cursor;                         // region 2
      }
      else
      {
        // play cursor is in front
        bytes_to_write = target_cursor - byte_to_lock; // region 1
      }

      win32_fill_sound_buffer(
          &sound_output,
          byte_to_lock,
          bytes_to_write
      );
    }

    Win32WindowDimensions dims = win32_get_window_dimensions(window_handle);
    win32_display_buffer_in_window(&win32_offscreen_buffer, device_context, dims.width, dims.height);
    ReleaseDC(window_handle, device_context);

    ++x_offset;
    y_offset += 2;

    /*
     InvalidateRect marks a section of the window invalid and
     needing to be redrawn. Passing in NULL invalidates the entire window.

     UpdateWindow immediately passes a WM_PAINT message to the
     window process message function, rather than waiting for
     the next message processing loop. This allows us to redraw
     the window whenever we want rather than waiting for Windows to tell us to.
    */

    // InvalidateRct(
    //     window_handle, NULL, FALSE
    //); //
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-invalidaterect
    // UpdateWindow(
    //     window_handle
    //); //
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-updatewindow

    LARGE_INTEGER end_counter;
    QueryPerformanceCounter(&end_counter);

    s64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
    s32 ms_per_frame = (s32)((1000*counter_elapsed) / perf_count_frequency);
    DEBUG_LOG("ms/frame: %d ms", ms_per_frame);
    last_counter = end_counter;
  }

  return msg.wParam;
}

/**
 * Windows entrypoint
 */
internal LRESULT CALLBACK
win32WndProc(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;
  switch (msg)
  {
    //case WM_KEYDOWN: {
    //  switch (wParam)
    //  {
    //    // Close window from 'Q'
    //    case 'Q': {
    //      DestroyWindow(window_handle);
    //    }
    //  }
    //}
    break;
    case WM_QUIT:
    case WM_DESTROY: {
      running = false;
    }
    break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
      u32 vkCode  = wParam;
      b32 wasDown = ((lParam & (1 << 30)) != 0);
      b32 isDown  = ((lParam & (1 << 31)) == 0);

      if (wasDown != isDown)
      {
        if (vkCode == 'W')
        {
          OutputDebugStringA("W\n");
        }
        else if (vkCode == 'A')
        {
          OutputDebugStringA("A\n");
        }
        else if (vkCode == 'S')
        {
          OutputDebugStringA("S\n");
        }
        else if (vkCode == 'D')
        {
          OutputDebugStringA("D\n");
        }
        else if (vkCode == 'Q')
        {
          running = false;
        }
        else if (vkCode == 'E') {}
        else if (vkCode == VK_UP) {}
        else if (vkCode == VK_DOWN) {}
        else if (vkCode == VK_LEFT) {}
        else if (vkCode == VK_RIGHT) {}
        else if (vkCode == VK_ESCAPE)
        {
          OutputDebugStringA("ESCAPE: ");
          if (isDown)
          {
            OutputDebugStringA("isDown ");
          }
          if (wasDown)
          {
            OutputDebugStringA("wasDown");
          }
          OutputDebugStringA("\n");
        }
        else if (vkCode == VK_SPACE) {}
        b32 altKeyWasDown = ((lParam & (1 << 29)) != 0);
        if ((vkCode == VK_F4) && altKeyWasDown)
        {
          running = false;
        }
      }
    }
    // case WM_LBUTTONUP: {
    //     int x = GetlParamX(lParam);
    //     int y = GetlParamY(lParam);
    //     u32 color = 0xffffff;
    //     draw_pixel(x, y, color);
    // } break;
    // case WM_MOUSEMOVE: {
    //   if (wParam == MK_LBUTTON) {
    //     int x = GetlParamX(lParam);
    //     int y = GetlParamY(lParam);
    //     u32 color = 0xffffff;
    //     draw_pixel(x, y, color);
    //   }

    //} break;
    // GDI Drawing logic {{{
    case WM_PAINT: {
      /*
      static PAINTSTRUCT paint;
      static HDC device_context;
      win32_paint_bitmap(window_handle, paint, device_context);
      */

      PAINTSTRUCT paint;
      HDC device_context = BeginPaint(window_handle, &paint);

      Win32WindowDimensions dims = win32_get_window_dimensions(window_handle);
      win32_display_buffer_in_window(
          &win32_offscreen_buffer, device_context, dims.width, dims.height
      );

      EndPaint(window_handle, &paint);
    }
    break;
    // Set the size of the pixel array and finish setting up GDI bitmap
    // case WM_SIZE: {
    //  // NOTE: When the biHeight field is negative, this is the clue to
    //  Windows to treat this bitmap as top-down, instead of bottom-up, meaning
    //  that the first three bytes of the image are the color for the top left
    //  pixel in the bitmap, not the bottom left.

    //  /*
    //  RECT client_rect;
    //  GetClientRect(window_handle, &client_rect);
    //  int width   = client_rect.right - client_rect.left;
    //  int height  = client_rect.bottom - client_rect.top;
    //  win32_resize_dib_section(win32_offscreen_buffer, width, height);
    //  */

    //  win32_resize_bitmap(&win32_offscreen_buffer, lParam);
    //} break;
    /// }}}
    default: {
      result = DefWindowProc(window_handle, msg, wParam, lParam);
    }
  }
  return result;
}
#endif // }}}

/* Linux entrypoint {{{
 */

#if defined(__linux__)

#  include <stdbool.h>
#  include <stdio.h>
#  include <sys/mman.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#  include <math.h>

#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/keysym.h>

#  include <alsa/asoundlib.h>

#  include "bmp.h"

typedef struct LinuxSoundOutput LinuxSoundOutput;
struct LinuxSoundOutput {
  int samples_per_second;
  int bytes_per_sample;
  u32 running_sample_index;
  int tone_hz;
  int tone_volume;
  int wave_period;
  f32 t_sine;
};

b32 keyboard[256]                = {0};
global usize current_buffer_size = 0;
global b32 running               = true;

/* GUI MODE:
 * 0 -> "draw gradient mode"
 * 1 -> "draw white background for drawing"
 */
enum GUI_MODE { MODE_GRADIENT_ANIMATION = 0,
                MODE_DRAWING            = 1 };

global enum GUI_MODE current_gui_mode = MODE_GRADIENT_ANIMATION;

#  define WINDOW_WIDTH    1480
#  define WINDOW_HEIGHT   860
#  define BYTES_PER_PIXEL 4 // 4 bytes for 32-bit color depth (e.g. BGRA)

#  define DARK_GREEN 0x8aa37f
#  define BLUE       0x0000ff
#  define PINK       0xffa6c9
#  define RED        0xcd1c18
#  define WHITE      0xffffff
#  define BLACK      0x000000

void x11_clear_buffer(XImage* image, u64 color)
{
  assert(
      image != NULL && image->data != NULL &&
      "XImage and XImage buffer must not be null"
  );

  int width        = image->width;
  int height       = image->height;
  u8* buffer_start = (u8*)image->data;
  int pitch        = image->bytes_per_line;

  for (int y = 0; y < height; ++y)
  {
    u32* pixel = (u32*)(buffer_start + (y * pitch));
    for (int x = 0; x < width; ++x)
    {
      *pixel++ = color;
    }
  }
}

void x11_render_buffer(Display* display, Window window, GC gc, XImage* image)
{
  assert(image && display && window && gc && "Render parameters must not be null");

  if (image && display && window && gc)
  {
    XPutImage(
        display, window, gc, image, 0, 0, 0, 0, image->width, image->height
    );
    XFlush(display);
  }
}

void draw_snow(XImage* image)
{
  assert(
      image != NULL && image->data != NULL &&
      "XImage and XImage buffer must not be null"
  );

  int height       = image->height;
  int width        = image->width;
  int pitch        = image->bytes_per_line;
  u8* buffer_start = (u8*)image->data;

  for (int y = 0; y < height; y += 1)
  {
    u32* pixel = (u32*)(buffer_start + (y * pitch));

    for (int x = 0; x < width; x += 1)
    {
      u32 bits = xorshift32();
      u8 noise = (u8)(bits & (bits >> 8)) & (bits >> 16);

      u8 blue  = noise;
      u8 green = noise;
      u8 red   = noise;
      u8 alpha = 0;

      u32 packed_channels = (alpha << 24) | (red << 16) | (green << 8) | blue;
      *pixel++            = packed_channels;
    }
  }
}

void draw_random_gradient(u64 x_offset, u64 y_offset, XImage* image)
{
  assert(
      image != NULL && image->data != NULL &&
      "XImage and XImage buffer must not be null"
  );

  int height = image->height;
  int width  = image->width;
  // get stride (pitch) from the XImage struct
  int pitch        = image->bytes_per_line;
  u8* buffer_start = (u8*)image->data;

  for (int y = 0; y < height; y += 1)
  {
    // calculate the starting address of the current row
    u32* pixel = (u32*)(buffer_start + (y * pitch));
    for (int x = 0; x < width; x += 1)
    {
      // PIXEL COLOR CALCULATION
      u8 blue  = 150 + (u8)((x + y_offset) % 100);
      u8 green = 50 + (u8)((y + x_offset) % 100);
      u8 red   = 25 + (u8)(x - x_offset);
      u8 alpha = 0;

      // NOTE: explicitly packing 32-bits, BGRA/BGR packing assumed, X11's byte
      // order can vary.
      u32 packed_channels = (alpha << 24) | (red << 16) | (green << 8) | blue;
      // direct memory write
      *pixel++ = packed_channels;
    }
  }
}

void x11_draw_pixel(XImage* image, int x, int y, u64 color)
{
  if (!image || x < 0 || y < 0 || x >= image->width || y >= image->height)
  {
    return; // bounds check
  }

  u8* buffer_start = (u8*)image->data;
  int pitch        = image->bytes_per_line;
  // calculate memory offset for 32 bit format
  u32* pixel = (u32*)(buffer_start + (y * pitch) + (x * BYTES_PER_PIXEL));
  // direct write to memory
  *pixel = (u32)color;
}

GC create_x11_graphics_context(
    Display* display,
    Window window,
    int reverse_video
)
{
  GC gc;
  unsigned long valuemask = 0;

  XGCValues values;
  unsigned int line_width = 2;
  int line_style          = LineSolid;
  int cap_style           = CapButt;
  int join_style          = JoinBevel;
  int screen_num          = DefaultScreen(display);

  gc = XCreateGC(display, window, valuemask, &values);

  // if (gc < 0) {
  //   fprintf(stderr, "XCreatedGC: \n");
  // }

  if (reverse_video)
  {
    XSetForeground(display, gc, WhitePixel(display, screen_num));
    XSetBackground(display, gc, WhitePixel(display, screen_num));
  }
  else
  {
    XSetForeground(display, gc, BlackPixel(display, screen_num));
    XSetBackground(display, gc, WhitePixel(display, screen_num));
  }

  // Define the style of lines that will be drawn for this GC
  XSetLineAttributes(
      display, gc, line_width, line_style, cap_style, join_style
  );

  // Define the fill style for the GC
  XSetFillStyle(display, gc, FillSolid);

  return gc;
}

int main(void)
{
  // syscall(SYS_write, 1, "I like pancakes\n", 17);

  // BitmapImage bmp_image = read_bmp_file("blue_pixel_24.bmp");
  //(void)bmp_image;
  //  printf("bmp_image -> %d", (int*)bmp_image);

  u32 x                = 0;
  u32 y                = 0;
  u32 width            = WINDOW_WIDTH;
  u32 height           = WINDOW_HEIGHT;
  u32 borderWidth      = 0;
  u32 windowDepth      = CopyFromParent;
  u32 windowClass      = CopyFromParent;
  Visual* windowVisual = CopyFromParent;

  u32 attributeValueMask =
      CWBackPixmap | CWEventMask |
      CWBitGravity; // tell x11 to look at background_pixmap and event_mask

  XSetWindowAttributes windowAttributes = {0};

  windowAttributes.background_pixmap = None; // prevent flickering by suppressing erase
  windowAttributes.background_pixel  = DARK_GREEN;
  windowAttributes.event_mask =
      KeyPressMask | KeyReleaseMask | ExposureMask | ButtonPressMask |
      ButtonReleaseMask | Button1MotionMask |
      StructureNotifyMask; // StructureNotifyMask for resizing

  // XOpenDisplay https://linux.die.net/man/3/xopendisplay
  // Open connection with the X server
  Display* mainDisplay = XOpenDisplay(0);

  // XDefaultRootWindow
  // https://tronche.com/gui/x/xlib/display/display-macros.html#DefaultRootWindow
  Window rootWindow = XDefaultRootWindow(mainDisplay);

  // XCreateSimpleWindow
  // https://tronche.com/gui/x/xlib/window/XCreateWindow.html XCreateWindow
  // https://tronche.com/gui/x/xlib/window/XCreateWindow.html
  Window mainWindow = XCreateWindow(
      mainDisplay,
      rootWindow,
      x,
      y,
      width,
      height,
      borderWidth,
      windowDepth,
      windowClass,
      windowVisual,
      attributeValueMask,
      &windowAttributes
  );

  // XMapWindow https://tronche.com/gui/x/xlib/window/XMapWindow.html
  XMapWindow(mainDisplay, mainWindow);
  // XFlush https://tronche.com/gui/x/xlib/event-handling/XFlush.html
  // XFlush(mainDisplay);

  // https://tronche.com/gui/x/xlib/GC/XCreateGC.html
  // Create a simple graphics context
  GC gc = XCreateGC(mainDisplay, mainWindow, 0, NULL);

  // XSetForeground(mainDisplay, gc, BlackPixel(mainDisplay, screen));
  int screen = DefaultScreen(mainDisplay);

  usize buffer_size    = (usize)WINDOW_WIDTH * WINDOW_HEIGHT * BYTES_PER_PIXEL;
  char* x11_backbuffer = mmap(
      NULL,
      buffer_size,
      PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS,
      -1,
      0
  );

  if (x11_backbuffer == MAP_FAILED)
  {
    fprintf(
        stderr, "Fatal Error: Failed to allocate initial backbuffer for X11.\n"
    );
    XFreeGC(mainDisplay, gc);
    XDestroyWindow(mainDisplay, mainWindow);
    XCloseDisplay(mainDisplay);
    return 1;
  }

  current_buffer_size = buffer_size;

  XImage* image = XCreateImage(
      mainDisplay,
      DefaultVisual(mainDisplay, screen),
      DefaultDepth(mainDisplay, screen),
      ZPixmap,
      0,
      x11_backbuffer,
      WINDOW_WIDTH,
      WINDOW_HEIGHT,
      32,
      0
  );

  // https://tronche.com/gui/x/xlib/event-handling/XSelectInput.html
  // https://tronche.com/gui/x/xlib/events/mask.html
  // XSelectInput(
  //    mainDisplay,
  //    mainWindow,
  //    KeyPressMask | KeyReleaseMask | ExposureMask | ButtonPressMask |
  //        ButtonReleaseMask | Button1MotionMask
  //);

  // https://gitlab.com/UltimaN3rd/croaking-kero-programming-tutorials/blob/master/opening_a_window_on_linux_with_xlib/opening_a_window_with_xlib.c
  // GC gc = create_x11_graphics_context(mainDisplay, mainWindow, 0);
  //  https://tronche.com/gui/x/xlib/event-handling/XSync.html
  //  Flush the output buffer and wait until all request have been received and
  //  processed by the X server
  // XSync(mainDisplay, False);
  // XFlush(mainDisplay);

  // image->data = malloc(image->bytes_per_line * image->height);
  x11_clear_buffer(image, BLACK);

  XEvent generalEvent;
  u64 x_offset = 0;
  u64 y_offset = 0;

  LinuxSoundOutput sound_output     = {0};
  sound_output.samples_per_second   = 48000;
  sound_output.bytes_per_sample     = sizeof(s16) * 2;
  sound_output.running_sample_index = 0;
  sound_output.tone_hz              = 256;
  sound_output.tone_volume          = 3000;
  sound_output.wave_period          = sound_output.samples_per_second / sound_output.tone_hz;
  sound_output.t_sine               = 0.0f;

  s16 samples[48000 * 2] = {0};
  u32 audio_channels     = 2;

  // https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html#ga8340c7dc0ac37f37afe5e7c21d6c528b
  snd_pcm_t* pcm_handle = NULL;
  int alsa_open         = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);

  if (alsa_open < 0)
  {
    fprintf(stderr, "[ALSA] Unable to open PCM device: %s\n", snd_strerror(alsa_open));
  }
  else
  {
    // https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html#ga1ca0dc120a484965e26cabf966502330
    snd_pcm_hw_params_t* pcm_hardware_configuration;
    snd_pcm_hw_params_alloca(&pcm_hardware_configuration);
    snd_pcm_hw_params_any(pcm_handle, pcm_hardware_configuration);
    snd_pcm_hw_params_set_access(pcm_handle, pcm_hardware_configuration, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, pcm_hardware_configuration, SND_PCM_FORMAT_S16_LE);

    snd_pcm_hw_params_set_channels(pcm_handle, pcm_hardware_configuration, audio_channels);
    snd_pcm_hw_params_set_rate(pcm_handle, pcm_hardware_configuration, sound_output.samples_per_second, 0);
    snd_pcm_hw_params_set_periods(pcm_handle, pcm_hardware_configuration, 10, 0);
    snd_pcm_hw_params_set_period_time(pcm_handle, pcm_hardware_configuration, 100000, 0);

    // install PCM hardware configuration
    int alsa_hw_params = snd_pcm_hw_params(pcm_handle, pcm_hardware_configuration);
    if (alsa_hw_params < 0)
    {
      fprintf(stderr, "[ALSA] Unable to set hardware parameters: %s\n", snd_strerror(alsa_hw_params));
      snd_pcm_close(pcm_handle);
      pcm_handle = NULL;
    }
    else
    {
      // set alsa to non-blocking mode
      // https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html#ga6bd90de1d1527b5804090dcce51079ad
      snd_pcm_nonblock(pcm_handle, 1);
    }
  }

  // https://tronche.com/gui/x/xlib/event-handling/XPending.html
  while (running)
  {
    // poll (don't wait for all events) (similar to PeekMessageW)
    while (XPending(mainDisplay) > 0)
    {
      XNextEvent(mainDisplay, &generalEvent);

      if (generalEvent.type == KeyPress)
      {
        DEBUG_LOG("[DEBUG] KeyPress: %x", generalEvent.xkey.keycode);
      }
      else
      {
        DEBUG_LOG("[DEBUG] X11 Event: %d", generalEvent.type);
      }

      switch (generalEvent.type)
      {
        case ConfigureNotify: {
          int new_width  = generalEvent.xconfigure.width;
          int new_height = generalEvent.xconfigure.height;

          if (new_width != image->width || new_height != image->height)
          {
            printf("[DEBUG] Resizing buffer to %dx%d\n", new_width, new_height);

            char* old_data = image->data;
            usize old_size = current_buffer_size;

            // Prevents XDestroyImage from deallocating the backbuffer
            image->data = NULL;
            XDestroyImage(image);

            if (munmap(old_data, old_size) == -1)
            {
              perror("[ERROR] munmap for resizing backbuffer failed");
            }

            image = XCreateImage(
                mainDisplay,
                DefaultVisual(mainDisplay, screen),
                DefaultDepth(mainDisplay, screen),
                ZPixmap,
                0,
                NULL, // data pointer is null initially
                new_width,
                new_height,
                32,
                0
            );

            // reallocate for the new size
            usize new_buffer_size = image->bytes_per_line * image->height;
            current_buffer_size   = new_buffer_size;
            image->data           = mmap(
                NULL,
                new_buffer_size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1,
                0
            );

            if (image->data == MAP_FAILED)
            {
              fprintf(
                  stderr,
                  "ERROR: Failed to reallocate image data during resize.\n"
              );
              running = false;
            }

            if (current_gui_mode == MODE_DRAWING)
            {
              x11_clear_buffer(image, WHITE);
            }
            else if (current_gui_mode == MODE_GRADIENT_ANIMATION)
            {
              draw_random_gradient(x_offset, y_offset, image);
            }
          }
          break;
        }
        // https://tronche.com/gui/x/xlib/events/exposure/expose.html
        case Expose: {
          printf("X11 Expose Event: %d\n", generalEvent.xexpose.type);
          x11_render_buffer(mainDisplay, mainWindow, gc, image);
          // draw_to_buffer(y_color, x_color, image, gc, mainWindow,
          // mainDisplay);
          //  if (generalEvent.xexpose.count) break;
          ////XSetForeground(mainDisplay, gc, WhitePixel(mainDisplay,
          /// screen_num)); /XDrawString(mainDisplay, mainWindow, gc, 10, 10,
          /// mytext, strlen(mytext));
          // XFillRectangle(mainDisplay, mainWindow, gc, 0, 100, 50, 50);
          break;
        }
        case MotionNotify: {
          if (current_gui_mode == MODE_DRAWING &&
              (generalEvent.xmotion.state & Button1Mask))
          {
            int x_m = generalEvent.xmotion.x;
            int y_m = generalEvent.xmotion.y;
            x11_draw_pixel(image, x_m, y_m, BLACK);
            x11_render_buffer(mainDisplay, mainWindow, gc, image);
          }

          // int symbol = XLookupKeysym(&generalEvent.xkey, 0);
          //// Mouse position
          // int x = generalEvent.xkey.x;
          // int y = generalEvent.xkey.y;

          // printf(
          //     "[DEBUG] MotionNotify symbol: %d, coordinates: [%d, %d]\n",
          //     symbol,
          //     x,
          //     y
          //);
          // XDrawPoint(mainDisplay, mainWindow, gc, x, y);
          break;
        }
        case ButtonPress: {
          if (current_gui_mode == MODE_DRAWING &&
              generalEvent.xbutton.button == Button1)
          {
            int symbol = XLookupKeysym(&generalEvent.xkey, 0);
            // Mouse position
            int x = generalEvent.xkey.x;
            int y = generalEvent.xkey.y;

#  ifdef DEBUG
            printf(
                "[DEBUG] ButtonPress symbol: %d, coordinates: [%d, %d]\n",
                symbol,
                generalEvent.xkey.x,
                generalEvent.xkey.y
            );
#  endif

            x11_draw_pixel(image, x, y, BLACK);
            x11_render_buffer(mainDisplay, mainWindow, gc, image);
          }
          break;
        }
        case ButtonRelease: {
#  ifdef DEBUG
          int symbol = XLookupKeysym(&generalEvent.xkey, 0);

          // Mouse position
          int x = generalEvent.xkey.x;
          int y = generalEvent.xkey.y;

          printf(
              "[DEBUG] Mouse button released, symbol: %d, [%d, %d]\n",
              symbol,
              x,
              y
          );
#  endif
          break;
        }
        case KeyPress: {
          int symbol           = XLookupKeysym(&generalEvent.xkey, 0);
          keyboard[(u8)symbol] = true;

          printf("KeyPress: %x\n", generalEvent.xkey.keycode);

          switch (symbol)
          {
            case XK_Escape: {
              printf("Escape pressed\n");
            }
            break;
            // case XK_Pointer_Button1: {
            //   printf("Pointer Button 1 pressed\n");
            //   break;
            // }
            case XK_d: {
              // Toggle DRAWING gui mode
              current_gui_mode = MODE_DRAWING;
              x11_clear_buffer(image, WHITE);
              x11_render_buffer(mainDisplay, mainWindow, gc, image);
              printf("[DEBUG] Mode set to DRAWING: White canvas setup.\n");
            }
            break;
            case XK_r: {
              if (current_gui_mode == MODE_DRAWING)
              {
                x11_clear_buffer(image, WHITE);
              }
              else
              {
                x_offset = 0;
                y_offset = 0;
              }
              x11_render_buffer(mainDisplay, mainWindow, gc, image);
            }
            break;
            case XK_a: {
              // printf("\"a\" pressed\n");
              current_gui_mode = MODE_GRADIENT_ANIMATION;
              printf("[DEBUG] Mode set to ANIMATION: Gradient setup.\n");
            }
            break;
            case XK_q: {
              printf("Closing application\n");
              running = false;
            }
            break;
          }
        }
        break;
      }
    }

    // ---------------------------------------------------------
    // ALSA Audio Ring Buffer Logic
    // ---------------------------------------------------------

    // https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html#ga8bb836bd0c414b59789d51a5f5379c08
    if (pcm_handle)
    {
      snd_pcm_sframes_t frames_ready = snd_pcm_avail_update(pcm_handle);

      if (frames_ready < 0)
      {
        if (frames_ready == -EPIPE)
        {
          fprintf(stderr, "[ALSA] Buffer undderrun (xrun) occurred. Recovering...\n");
          snd_pcm_prepare(pcm_handle);
        }
        else
        {
          fprintf(stderr, "[ALSA] Unknown ALSA avail update return value (%ld)\n", frames_ready);
        }

        // Skip rest of the aduio handling for this frame
        continue;
      }

      if (frames_ready > 0)
      {
        frames_ready    = ClampTop(frames_ready, 2000);
        s16* sample_out = samples;

        // Generate audio frames
        for (snd_pcm_sframes_t idx = 0; idx < frames_ready; idx += 1)
        {
          f32 sine_value   = sinf(sound_output.t_sine);
          s16 sample_value = (s16)(sine_value * sound_output.tone_volume);

          *sample_out++ = sample_value; // Left channel
          *sample_out++ = sample_value; // Right channel

          sound_output.t_sine += (2.0f * PI_F32 * 1.0) / (f32)sound_output.wave_period;
          ++sound_output.running_sample_index;
        }

        snd_pcm_sframes_t frames_written = snd_pcm_writei(pcm_handle, samples, frames_ready);
        if (frames_written == -EPIPE)
        {
          fprintf(stderr, "[ALSA] Buffer underrun during write. Recovering...\n");
          snd_pcm_prepare(pcm_handle);
        }
      }
    }

    if (current_gui_mode == MODE_GRADIENT_ANIMATION)
    {
      // move the animation
      ++x_offset;
      y_offset += 2;
      // render
      // draw_random_gradient(x_offset, y_offset, image);
      draw_snow(image);
      x11_render_buffer(mainDisplay, mainWindow, gc, image);
    }
  }

  // Wait for the X Server to process all buffered requests and clear the queue
  // before cleanup
  XSync(mainDisplay, False);

  if (image && image->data)
  {
    // Prevent XDestroyImage from deallocating the backbuffer
    char* imgdata = image->data;
    image->data   = NULL;

    if (munmap(imgdata, current_buffer_size) == -1)
    {
      perror("[ERROR] munmap for final cleanup backbuffer failed");
    }
  }

  // Cleanup

  if (pcm_handle)
  {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    pcm_handle = NULL;
  }

  XDestroyImage(image);
  XFreeGC(mainDisplay, gc);
  XDestroyWindow(mainDisplay, mainWindow);

  // https://tronche.com/gui/x/xlib/display/XCloseDisplay.html
  XCloseDisplay(mainDisplay);

  return 0;
}
#endif // }}}
