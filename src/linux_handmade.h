#  define WINDOW_WIDTH    1480
#  define WINDOW_HEIGHT   860

#  define DARK_GREEN 0x8aa37f
#  define BLUE       0x0000ff
#  define PINK       0xffa6c9
#  define RED        0xcd1c18
#  define WHITE      0xffffff
#  define BLACK      0x000000

typedef struct LinuxSoundOutput LinuxSoundOutput;
struct LinuxSoundOutput
{
  int samples_per_second;
  int bytes_per_sample;
  u32 running_sample_index;
  int tone_hz;
  int tone_volume;
  int wave_period;
  f32 t_sine;
};

/* GUI MODE:
 * 0 -> "draw gradient mode"
 * 1 -> "draw white background for drawing"
 */
enum GUI_MODE { MODE_GRADIENT_ANIMATION = 0,
                MODE_DRAWING            = 1 };

b32 alsa_set_hardware_parameters(snd_pcm_t* pcm_handle, u32 audio_channels, u32 samples_per_second);
void x11_clear_buffer(XImage* image, u64 color);
void x11_render_buffer(Display* display, Window window, GC gc, XImage* image);
void x11_draw_pixel(XImage* image, int x, int y, u64 color);
GC create_x11_graphics_context(Display* display, Window window, int reverse_video);
void create_wav_sine_file(void);
