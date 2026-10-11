//#include "bmp.h"

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

/* GUI MODE:
 * 0 -> "draw gradient mode"
 * 1 -> "draw white background for drawing"
 */
enum GUI_MODE { MODE_GRADIENT_ANIMATION = 0,
                MODE_DRAWING            = 1 };

global enum GUI_MODE current_gui_mode = MODE_GRADIENT_ANIMATION;

#  define WINDOW_WIDTH    1480
#  define WINDOW_HEIGHT   860

#  define DARK_GREEN 0x8aa37f
#  define BLUE       0x0000ff
#  define PINK       0xffa6c9
#  define RED        0xcd1c18
#  define WHITE      0xffffff
#  define BLACK      0x000000

b32 alsa_set_hardware_parameters(snd_pcm_t* pcm_handle, u32 audio_channels, u32 samples_per_second)
{
  snd_pcm_hw_params_t* pcm_hardware_configuration;
  snd_pcm_hw_params_alloca(&pcm_hardware_configuration);

  int init_hardware_params = snd_pcm_hw_params_any(pcm_handle, pcm_hardware_configuration);
  if (init_hardware_params < 0)
  {
    DEBUG_LOG("[ALSA] Unable to allocate hardware parameter structure (%s)", snd_strerror(init_hardware_params));
    return 0;
  }

  // https://www.alsa-project.org/alsa-doc/alsa-lib/pcm_2pcm_8c.html#a875bd8c29ac0febc6f9c42dbb0f750e3
  int set_access = snd_pcm_hw_params_set_access(pcm_handle, pcm_hardware_configuration, SND_PCM_ACCESS_RW_INTERLEAVED);
  if (set_access < 0)
  {
    DEBUG_LOG("[ALSA] Unable set access type (%s)", snd_strerror(set_access));
    return 0;
  }

  // https://www.alsa-project.org/alsa-doc/alsa-lib/pcm_2pcm_8c.html#a6cf998c0383baba561edb8921c0f672e
  int set_format = snd_pcm_hw_params_set_format(pcm_handle, pcm_hardware_configuration, SND_PCM_FORMAT_S16_LE);
  if (set_format < 0)
  {
    DEBUG_LOG("[ALSA] Unable set sample format (%s)", snd_strerror(set_format));
    return 0;
  }

  // https://www.alsa-project.org/alsa-doc/alsa-lib/pcm_2pcm_8c.html#a3a5b2a05c5d9869cc743dac71c0d270a
  int set_channels = snd_pcm_hw_params_set_channels(pcm_handle, pcm_hardware_configuration, audio_channels);
  if (set_channels < 0)
  {
    DEBUG_LOG("[ALSA] Unable set channels count (%u), (%s)", audio_channels, snd_strerror(set_channels));
    return 0;
  }

  // https://www.alsa-project.org/alsa-doc/alsa-lib/pcm_2pcm_8c.html#a29cd2571738847813af1489043d1af5a
  int set_rate = snd_pcm_hw_params_set_rate(pcm_handle, pcm_hardware_configuration, samples_per_second, 0);
  if (set_rate < 0)
  {
    DEBUG_LOG("[ALSA] Unable set rate %uHz, (%s)", samples_per_second, snd_strerror(set_rate));
    return 0;
  }

  // https://www.alsa-project.org/alsa-doc/alsa-lib/pcm_2pcm_8c.html#a472aa3f2d8ce4040caa874fe60aba961
  int set_period = snd_pcm_hw_params_set_periods(pcm_handle, pcm_hardware_configuration, 10, 0);
  if (set_period < 0)
  {
    DEBUG_LOG("[ALSA] Unable set period (%s)", snd_strerror(set_period));
    return 0;
  }

  // https://www.alsa-project.org/alsa-doc/alsa-lib/pcm_2pcm_8c.html#aa2bc2a32d3971521064741a30e10c92f
  int set_period_time = snd_pcm_hw_params_set_period_time(pcm_handle, pcm_hardware_configuration, 100000, 0);
  if (set_period_time < 0)
  {
    DEBUG_LOG("[ALSA] Unable set period time (%s)", snd_strerror(set_period_time));
    return 0;
  }

  // install PCM hardware configuration
  int alsa_hw_params = snd_pcm_hw_params(pcm_handle, pcm_hardware_configuration);
  if (alsa_hw_params < 0)
  {
    DEBUG_LOG("[ALSA] Unable to set hardware parameters: %s", snd_strerror(alsa_hw_params));
    return 0;
  }

  return 1;
}

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

void create_wav_sine_file(void)
{
  Arena* wave_arena = arena_alloc();
  OS_Handle file = os_file_open("test.wav", OS_AccessFlags_Create | OS_AccessFlags_Write);
  if (file.handle != (void*)-1)
  {
    // We are just using this for generating a sine wave and storing it's state
    u32 SAMPLE_RATE = 48000;
    u16 BIT_DEPTH = 16;
    u16 NUM_CHANNELS = 2;
    f32 frequency = 440.0f;
    s32 duration = 6;
    u32 max_amplitude = (1 << (BIT_DEPTH - 1)) - 1;

    u32 total_samples = SAMPLE_RATE * duration * NUM_CHANNELS;
    u32 datasize = total_samples * (BIT_DEPTH / 8);
    u32 total_filesize = sizeof(RIFFHeader) + sizeof(RIFFChunk) + sizeof(WAVEChunk) + sizeof(RIFFChunk) + datasize;

    u8* buffer = push_array(wave_arena, u8, total_filesize);
    u8* cursor = buffer;

    // Write the RIFF header
    RIFFHeader* riff_header = (RIFFHeader*)cursor;
    riff_header->riff_id = AsciiID4('R', 'I', 'F', 'F');
    riff_header->filesize = total_filesize - 8;
    riff_header->wave_id  = AsciiID4('W', 'A', 'V', 'E');
    cursor += sizeof(RIFFHeader);

    // Write the "fmt " chunk header
    RIFFChunk* fmt_header = (RIFFChunk*)cursor;
    fmt_header->id = AsciiID4('f', 'm', 't', ' ');
    fmt_header->size = sizeof(WAVEChunk);
    cursor += sizeof(RIFFChunk);

    // Write "fmt " payload
    WAVEChunk* fmt = (WAVEChunk*)cursor;
    fmt->audio_format = 1;
    fmt->num_channels = NUM_CHANNELS;
    fmt->sample_rate = SAMPLE_RATE;
    fmt->block_align = NUM_CHANNELS * (BIT_DEPTH / 8);
    fmt->byte_rate = SAMPLE_RATE * fmt->block_align;
    fmt->bits_per_sample = BIT_DEPTH;
    cursor += sizeof(WAVEChunk);

    // Write "data" chunk header
    RIFFChunk* data_header = (RIFFChunk*)cursor;
    data_header->id = AsciiID4('d', 'a', 't', 'a');
    data_header->size = datasize;
    cursor += sizeof(RIFFChunk);

    ToneGenerator tone_generator = {
      .frequency = frequency,
      .amplitude = 0.5f,
      .angle = 0.0f,
      .phase_increment = 2.0 * PI_F64 * frequency / SAMPLE_RATE
    };

    s16* audio_samples = (s16*)cursor;
    for (u32 idx = 0; idx < (SAMPLE_RATE * duration); idx += 1)
    {
      f32 sample = generate_sine_sample(&tone_generator);
      s16 pcm_sample = (s16)(sample * max_amplitude);

      // Duplicate for left and right channels
      *audio_samples++ = pcm_sample; // Channel 1 left
      *audio_samples++ = pcm_sample; // Channel 2 right
    }

    if (os_file_write(file, buffer, total_filesize))
    {
      DEBUG_LOG("Successfully generated and wrote %u bytes to test.wav!", total_filesize);
    }
  }
  os_file_close(file);
}

int main(void)
{

  // BEGIN WAVE TEST
  create_wav_sine_file();
  // END WAVE TEST

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

  usize max_buffer_size = cast(usize)MAX_WIDTH * MAX_HEIGHT * BYTES_PER_PIXEL;
  Arena* arena = arena_alloc();
  char* x11_backbuffer = push_array(arena, char, max_buffer_size);

  if (x11_backbuffer == 0)
  {
    fprintf(
        stderr, "Fatal Error: Failed to allocate initial backbuffer for X11.\n"
    );
    XFreeGC(mainDisplay, gc);
    XDestroyWindow(mainDisplay, mainWindow);
    XCloseDisplay(mainDisplay);
    return 1;
  }

  current_buffer_size = max_buffer_size;

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

  u64 last_cycle_count = __rdtsc();
  // Linux QPC / QPF
  u64 start_time       = os_get_time();

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
    if (!alsa_set_hardware_parameters(pcm_handle, audio_channels, sound_output.samples_per_second))
    {
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
            DEBUG_LOG("[DEBUG] Resizing buffer to %dx%d\n", new_width, new_height);

            // clamp to max suported resolution
            new_width  = ClampTop(new_width, MAX_WIDTH);
            new_height = ClampTop(new_height, MAX_HEIGHT);

            // Prevents XDestroyImage from deallocating the backbuffer
            image->data = NULL;
            XDestroyImage(image);

            image = XCreateImage(
                mainDisplay,
                DefaultVisual(mainDisplay, screen),
                DefaultDepth(mainDisplay, screen),
                ZPixmap,
                0,
                (char*)x11_backbuffer, // data pointer is null initially
                new_width,
                new_height,
                32,
                0
            );

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

    // Get ending time
    u64 end_time = os_get_time();
    f32 elapsed_microseconds = os_get_time_elapsed_in_microseconds(start_time, end_time);
    f32 fps = elapsed_microseconds ? (f32)(1000000.0f / elapsed_microseconds) : 0.0f;

    u64 end_cycle_count = __rdtsc();
    s64 cycles_elapsed = end_cycle_count - last_cycle_count;
    s32 megacycles_per_frame = cycles_elapsed / (1000 * 1000);

    DEBUG_LOG("µs/frame: %.2f | fps: %.2f | Mc/frame %d", elapsed_microseconds, fps, megacycles_per_frame);

    // Reset counters for the next frame
    last_cycle_count = end_cycle_count;
    start_time = end_time;

  } // End of while(running)

  // Wait for the X Server to process all buffered requests and clear the queue
  // before cleanup
  XSync(mainDisplay, False);

  if (image && image->data)
  {
    // Prevent XDestroyImage from deallocating the backbuffer
    image->data = NULL;
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
