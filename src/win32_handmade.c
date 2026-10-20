/* Forward declaration */
#include "handmade.h"

internal LRESULT CALLBACK win32WndProc(HWND, UINT, WPARAM, LPARAM);

#  define _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_DEPRECATE

#  ifndef UNICODE
#    define UNICODE
#  endif

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
global x_input_set_state* XInputSetState_ = XInputSetStateStub;
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

internal void win32_clear_sound_buffer(Win32SoundOutput* sound_output)
{
  VOID* region1;
  DWORD region1_size;
  VOID* region2;
  DWORD region2_size;

  DWORD write_cursor_offset = 0;
  DWORD bytes_to_write      = (DWORD) sound_output->secondary_buffer_size;

  HRESULT directsound_lock = secondary_directsound_buffer->lpVtbl->Lock(
      secondary_directsound_buffer,
      write_cursor_offset,
      bytes_to_write,
      &region1,
      &region1_size,
      &region2,
      &region2_size,
      0
  );

  if (SUCCEEDED(directsound_lock))
  {
    s16* dest_sample   = (s16*)region1;
    for (DWORD byte_index = 0; byte_index < region1_size; byte_index += 1)
    {
      *dest_sample++ = 0;
    }

    dest_sample = (s16*)region2;
    for (DWORD byte_index = 0; byte_index < region2_size; byte_index += 1)
    {
      *dest_sample++ = 0;
    }

    secondary_directsound_buffer->lpVtbl->Unlock(secondary_directsound_buffer, region1, region1_size, region2, region2_size);
  }
}

internal void win32_fill_sound_buffer(
    Win32SoundOutput* sound_output,
    DWORD byte_to_lock,
    DWORD bytes_to_write,
    AudioBuffer* source_buffer
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
    s16* dest_sample   = (s16*)region1;
    s16* source_sample = source_buffer->samples;
    DWORD region1_sample_count = region1_size / sound_output->bytes_per_sample;
    for (DWORD sampleIndex = 0; sampleIndex < region1_sample_count; sampleIndex += 1)
    {
      *dest_sample++    = *source_sample++;
      *dest_sample++    = *source_sample++;
      ++sound_output->running_sample_index;
    }

    dest_sample = (s16*)region2;
    DWORD region2_sample_count = region2_size / sound_output->bytes_per_sample;
    for (DWORD sampleIndex = 0; sampleIndex < region2_sample_count; sampleIndex += 1)
    {
      *dest_sample++    = *source_sample++;
      *dest_sample++    = *source_sample++;
      ++sound_output->running_sample_index;
    }

    secondary_directsound_buffer->lpVtbl->Unlock(secondary_directsound_buffer, region1, region1_size, region2, region2_size);
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
  width  = ClampTop(width, MAX_WIDTH);
  height = ClampTop(height, MAX_HEIGHT);

  offscreen_buffer->width           = width;
  offscreen_buffer->height          = height;
  offscreen_buffer->bytes_per_pixel = BYTES_PER_PIXEL;

  offscreen_buffer->info.bmiHeader.biSize        = sizeof(offscreen_buffer->info.bmiHeader);
  offscreen_buffer->info.bmiHeader.biWidth       = offscreen_buffer->width;
  offscreen_buffer->info.bmiHeader.biHeight      = -(s32)(offscreen_buffer->height);
  offscreen_buffer->info.bmiHeader.biPlanes      = 1;
  offscreen_buffer->info.bmiHeader.biBitCount    = 32;
  offscreen_buffer->info.bmiHeader.biCompression = BI_RGB;
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

  win32_load_xinput();

  (void)pCmdLine;
  (void)hPrevInstance;

  // WNDCLASS
  // https://learn.microsoft.com/en-us/previous-versions/ms942860(v=msdn.10)
  // WNDCLASSA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassa
  // WNDCLASSEXA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassexa

  // Contains window class information
  WNDCLASSEXW windowClass = {0};

  // https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window
  LPCWSTR window_class_name = L"Sample Window Class";

  HWND window_handle = NULL;
  static MSG msg     = {0};

  usize max_buffer_size = cast(usize)MAX_WIDTH * MAX_HEIGHT * BYTES_PER_PIXEL;
  Arena* arena = arena_alloc();
  win32_offscreen_buffer.pixels = push_array(arena, u8, max_buffer_size);

  win32_resize_dib_section(&win32_offscreen_buffer, 1280, 720);

  windowClass.cbSize        = sizeof(WNDCLASSEXW);
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

  if (!RegisterClassExW(&windowClass))
  {
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox
    MessageBoxW(
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
  window_handle = CreateWindowExW(
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
    MessageBoxW(
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
  //win32_fill_sound_buffer(&sound_output, 0, (sound_output.latency_sample_count * sound_output.bytes_per_sample), &sound_output);

  if (!sound_is_playing)
  {
    secondary_directsound_buffer->lpVtbl->Play(secondary_directsound_buffer, 0, 0, DSBPLAY_LOOPING);
    sound_is_playing = 1;
  }

  s16* samples = push_array(arena, s16, sound_output.secondary_buffer_size);

  u64 last_cycle_count = __rdtsc();
  LARGE_INTEGER last_counter;
  QueryPerformanceCounter(&last_counter);

  OffscreenBuffer offscreen_buffer = {0};
  offscreen_buffer.bytes_per_pixel = win32_offscreen_buffer.bytes_per_pixel;
  offscreen_buffer.width  = win32_offscreen_buffer.width;
  offscreen_buffer.height = win32_offscreen_buffer.height;
  offscreen_buffer.memory = win32_offscreen_buffer.pixels;

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

    //draw_random_gradient(win32_offscreen_buffer.bytes_per_pixel, win32_offscreen_buffer.width, win32_offscreen_buffer.height, x_offset, y_offset);
    //draw_snow(
    //    win32_offscreen_buffer.pixels,
    //    win32_offscreen_buffer.width,
    //    win32_offscreen_buffer.height
    //);

    // DirectSound API logic
    DWORD byte_to_lock    = 0;
    DWORD target_cursor   = 0;
    DWORD bytes_to_write  = 0;
    DWORD play_cursor     = 0;
    DWORD write_cursor    = 0;

    b32 sound_is_valid = false;
    HRESULT directsound_cursor_positions = secondary_directsound_buffer->lpVtbl->GetCurrentPosition(secondary_directsound_buffer, &play_cursor, &write_cursor);
    if (sound_is_playing && SUCCEEDED(directsound_cursor_positions))
    {
      byte_to_lock  = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.secondary_buffer_size;
      target_cursor = ((play_cursor + (sound_output.latency_sample_count * sound_output.bytes_per_sample)) % sound_output.secondary_buffer_size);

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

      sound_is_valid = true;
    }

    AudioBuffer audio_buffer = {0};
    audio_buffer.samples_per_second = audio_buffer.samples_per_second;
    audio_buffer.sample_count = bytes_to_write / sound_output.bytes_per_sample;
    audio_buffer.samples = samples;
    audio_buffer.tone_hz = sound_output.tone_hz;
    audio_buffer.tone_volume = sound_output.tone_volume;

    game_update_and_render(
      &audio_buffer,
      &offscreen_buffer,
      x_offset,
      y_offset
    );

    if (sound_is_valid)
    {
      win32_fill_sound_buffer(
          &sound_output,
          byte_to_lock,
          bytes_to_write,
          &audio_buffer
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

    u64 end_cycle_count = __rdtsc();
    s64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
    s64 cycles_elapsed = end_cycle_count - last_cycle_count;
    s32 ms_per_frame = (s32)((1000*counter_elapsed) / perf_count_frequency);
    s32 fps = (s32)(perf_count_frequency / counter_elapsed);
    s32 megacycles_per_frame = cast(s32)(cycles_elapsed / (1000 * 1000));

    //f32 ms_per_frame = 1000.0f*(f32)counter_elapsed / (f32)perf_count_frequency;
    //f32 fps = ((f32)perf_count_frequency / (f32)counter_elapsed)
    //f32 megacycles_per_frame = (f32)cycles_elapsed / (1000.0f * 1000.0f)
    //DEBUG_LOG("ms/frame: %.02f ms | fps: %.02f | Mc/frame %.02f", ms_per_frame, fps, megacycles_per_frame);

    DEBUG_LOG("ms/frame: %d | fps: %d | Mc/frame %d", ms_per_frame, fps, megacycles_per_frame);

    last_counter = end_counter;
    last_cycle_count = end_cycle_count;
  }

  return cast(int)msg.wParam;
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
      u32 vkCode  = cast(u32)wParam;
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

    //} break;
    /// }}}
    default: {
      result = DefWindowProcW(window_handle, msg, wParam, lParam);
    }
  }
  return result;
}
