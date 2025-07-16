/* vi: foldmethod=marker
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "base.h"

#if defined(_WIN32) // Windows code {{{

#define _CRT_SECURE_NO_WARNINGS

#  ifndef UNICODE
#    define UNICODE
#  endif

#  include <windows.h>

#ifdef DEBUG

#  define MACRO_STMT(__stmt__) do { __stmt__ } while (0)
#  define __TO_STR(str) #str

/*
  TCHAR         - _stprintf - OutputDebugString
  CHAR/char     - sprintf   - OutputDebugStringA
  WCHAR/wchar_t - swprintf  - OutputDebugStringW
*/

// debug print macro for win32
#  define __win32_debug_print(argname, argvalue) MACRO_STMT( \
    char dbg_msg[255] = {0};                                 \
    sprintf(dbg_msg, __TO_STR(argname)" -> %d\n", argvalue); \
    OutputDebugStringA(dbg_msg);                             \
  )

#endif

#  define WINDOW_WIDTH  1480
#  define WINDOW_HEIGHT 860

#  define GetlParamX(lp) ((int)(short)LOWORD(lp))
#  define GetlParamY(lp) ((int)(short)HIWORD(lp))

/*
  Device-Independent Bitmaps (DIB)
  https://learn.microsoft.com/en-us/windows/win32/gdi/device-independent-bitmaps
*/

// START OF GDI Drawing declarations {{{

typedef struct Win32_OffscreenBuffer;
struct Win32_OffscreenBuffer {
  BITMAPINFO info;
  void* pixels; // pixel array for the bitmap
  u32 width;
  u32 height;
  u32 bytes_per_pixel;
  HBITMAP bitmap_handle;
  HDC frame_device_context;
};

struct Win32_OffscreenBuffer win32_offscreen_buffer = {.bytes_per_pixel = 4};
static int client_width;

void draw_random_gradient(
    u32* bitmap_memory,
    u32 bitmap_width,
    u32 bitmap_height,
    u32 x_offset,
    u32 y_offset
)
{
  u32 pitch = bitmap_width * win32_offscreen_buffer.bytes_per_pixel;
  u8* row   = (u8*)bitmap_memory;

  for (u32 y = 0; y < bitmap_height; ++y) {
    u8* pixel = (u8*)row;
    for (u32 x = 0; x < bitmap_width; ++x) {
      // Blue channel
      *pixel = (u8)(x + x_offset);
      ++pixel;

      // Green channel
      *pixel = (u8)(y + y_offset);
      ++pixel;

      // Red channel
      *pixel = 0;
      ++pixel;

      // Alpha channel
      *pixel = 0;
      ++pixel;
    }

    row += pitch;
  }
}

typedef wchar_t wchar;

void draw_pixel(int x, int y, u32 color) {
#ifdef DEBUG
  __win32_debug_print(x, x);
  __win32_debug_print(y, y);
#endif

  u32* pixel = win32_offscreen_buffer.pixels;
  pixel += y * win32_offscreen_buffer.width + x;
  *pixel = color;
}

/// }}}

/* main loop */
bool running = true;

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
    PWSTR pCmdLine,
    int nCmdShow
)
{
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

  HWND windowHandle = NULL;
  static MSG msg    = {0};

  windowClass.cbSize        = sizeof(WNDCLASSEX);
  windowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  windowClass.lpszClassName = window_class_name;
  windowClass.lpfnWndProc =
      win32WndProc; // Long Pointer to the Windows Procedure function
  windowClass.cbClsExtra    = 0;
  windowClass.cbWndExtra    = 0;
  windowClass.hInstance     = hInstance;
  windowClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  windowClass.lpszMenuName  = NULL;
  windowClass.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

  if (!RegisterClassEx(&windowClass)) {
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox
    MessageBox(
        windowHandle,
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
  win32_offscreen_buffer.info.bmiHeader.biSize = sizeof(
      win32_offscreen_buffer.info.bmiHeader
  ); // the number of bytes required by the structure
  win32_offscreen_buffer.info.bmiHeader.biPlanes =
      1; // the number of planes for the target device, must be set to 1
  win32_offscreen_buffer.info.bmiHeader.biBitCount =
      32; // the number of of bits per pixel (bpp)
  win32_offscreen_buffer.info.bmiHeader.biCompression = BI_RGB; // uncompressed RGB format

  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createcompatibledc
  win32_offscreen_buffer.frame_device_context = CreateCompatibleDC(0);

  /// }}}

  // window parameters
  DWORD extended_window_style = WS_EX_CLIENTEDGE;
  LPCWSTR window_name         = L"The title of my window";
  DWORD window_style          = WS_OVERLAPPEDWINDOW;
  u32 window_x       = CW_USEDEFAULT; // horizontal position of the window
  u32 window_y       = CW_USEDEFAULT; // vertical position of the window
  u32 window_width   = WINDOW_WIDTH;
  u32 window_height  = WINDOW_HEIGHT;
  HWND window_parent = NULL;
  HMENU window_menu  = NULL;
  LPVOID lp_param    = NULL;

  // Window handle for
  windowHandle = CreateWindowEx(
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

  if (windowHandle == NULL) {
    MessageBox(
      windowHandle,
      L"Window Creation Failed!",
      L"Error!",
      MB_ICONEXCLAMATION | MB_OK
    );
    return GetLastError();
  }

  // ShowWindow
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
  ShowWindow(windowHandle, nCmdShow);

  RECT rect;
  GetClientRect(windowHandle, &rect);
  client_width = rect.right - rect.left;

  u32 x_offset = 0;
  u32 y_offset = 0;

  while (running) {
    // Run the message loop
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagew
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dispatchmessagea
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT)
        running = false;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // GDI Drawing logic {{{

    //draw_random_gradient(win32_offscreen_buffer.pixels, win32_offscreen_buffer.width, win32_offscreen_buffer.height, x_offset, y_offset);
    //++x_offset;

    /*
     InvalidateRect marks a section of the window invalid and
     needing to be redrawn. Passing in NULL invalidates the entire window.

     UpdateWindow immediately passes a WM_PAINT message to the
     window process message function, rather than waiting for
     the next message processing loop. This allows us to redraw
     the window whenever we want rather than waiting for Windows to tell us to.
    */

    InvalidateRect(
        windowHandle, NULL, FALSE
    ); // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-invalidaterect
    UpdateWindow(
        windowHandle
    ); // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-updatewindow
    // }}}
  }

  return msg.wParam;
}

/**
 * Windows entrypoint
 */
LRESULT CALLBACK
win32WndProc(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;
  switch (msg) {
    case WM_KEYDOWN: {
      switch (wParam) {
        // Close window from 'Q'
        case 'Q': {
          DestroyWindow(windowHandle);
        }
      }
    } break;
    case WM_QUIT:
    case WM_DESTROY: {
      running = false;
    } break;
    case WM_LBUTTONUP: {
        int x = GetlParamX(lParam);
        int y = GetlParamY(lParam);
        u32 color = 0xffffff;
        draw_pixel(x, y, color);
    } break;
    case WM_MOUSEMOVE: {
      if (wParam == MK_LBUTTON) {
        int x = GetlParamX(lParam);
        int y = GetlParamY(lParam);
        u32 color = 0xffffff;
        draw_pixel(x, y, color);
      }

    } break;
    // GDI Drawing logic {{{
    case WM_PAINT: {
      static PAINTSTRUCT paint;
      static HDC deviceContext;

      deviceContext = BeginPaint(
          windowHandle, &paint
      ); // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint

      // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt
      // Painting function to copy the pixel array over to the window in the
      // specified rectangle
      BitBlt(
          deviceContext,
          paint.rcPaint.left,
          paint.rcPaint.top,
          paint.rcPaint.right - paint.rcPaint.left,
          paint.rcPaint.bottom - paint.rcPaint.top,
          win32_offscreen_buffer.frame_device_context,
          paint.rcPaint.left,
          paint.rcPaint.top,
          SRCCOPY
      );

      EndPaint(
          windowHandle, &paint
      ); // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint
    } break;
    // Set the size of the pixel array and finish setting up GDI bitmap
    case WM_SIZE: {
      win32_offscreen_buffer.info.bmiHeader.biWidth  = LOWORD(lParam);
      win32_offscreen_buffer.info.bmiHeader.biHeight = -HIWORD(lParam); // DIBs are based in a coordinate system that is upside down relative to Windows, source: https://learn.microsoft.com/en-us/previous-versions/ms969901(v=msdn.10)?redirectedfrom=MSDN

      // Delete already existing bitmap
      if (win32_offscreen_buffer.bitmap_handle)
        DeleteObject(win32_offscreen_buffer.bitmap_handle);

      // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createdibsection
      // Create a bitmap
      win32_offscreen_buffer.bitmap_handle = CreateDIBSection(
          NULL,             /* hdc      - Handle to a device context */
          &win32_offscreen_buffer.info, /* pbmi     - Pointer to bitmap info */
          DIB_RGB_COLORS, /* usage    - type of data contained in the bmiColors
                             array member of the BITMAPINFO structure pointed to
                             by pbmi */
          (
              void**
          )&win32_offscreen_buffer.pixels, /* ppvBits  - a pointer to a variable that receives a
                             pointer ot the location of the DIB bit values */
          0, /* hSection - a handle to a file-mapping object that hte function
                will use to create the DIB. */
          0  /* offset   - the offset form the beginning of the file-mapping
                object referenced by hSection where storage for the bitmap bit
                values is to begin */
      );

      // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectobject
      // point device context to the bitmap
      SelectObject(win32_offscreen_buffer.frame_device_context, win32_offscreen_buffer.bitmap_handle);

      win32_offscreen_buffer.width  = LOWORD(lParam);
      win32_offscreen_buffer.height = HIWORD(lParam);
    } break;
    /// }}}
    default: {
      result = DefWindowProc(windowHandle, msg, wParam, lParam);
    }
  }
  return result;
}
#endif // }}}

/* Linux entrypoint {{{
 */

#if defined(__linux__)

#  include <stdio.h>
#  include <unistd.h>
#  include <sys/syscall.h>
#  include <stdbool.h>
#  include <stdlib.h>
#  include <X11/Xlib.h>
#  include <X11/keysym.h>
#  include <X11/Xutil.h>

#  include "bmp.h"

bool keyboard[256]  = {0};
global bool running = true;

#  define DARK_GREEN 0x8aa37f
#  define BLUE       0x0000ff
#  define PINK       0xffa6c9
#  define RED        0xcd1c18
#  define WHITE      0xffffff

void drawToBuffer(
    u64 yColor,
    u64 xColor,
    XImage* image,
    GC gc,
    Window mainWindow,
    Display* mainDisplay
)
{
  for (int y = 0; y < 600; y += 1) {
    for (int x = 0; x < 800; x += 1) {
      // Checker pattern
      unsigned long pixel = ((x ^ y) & 1) ? yColor : xColor;
      XPutPixel(image, x, y, pixel);
    }
  }

  // https://tronche.com/gui/x/xlib/graphics/XPutImage.html
  XPutImage(mainDisplay, mainWindow, gc, image, 0, 0, 0, 0, 800, 600);
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

  if (reverse_video) {
    XSetForeground(display, gc, WhitePixel(display, screen_num));
    XSetBackground(display, gc, WhitePixel(display, screen_num));
  } else {
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
  //syscall(SYS_write, 1, "I like pancakes\n", 17);

  BitmapImage bmp_image = read_bmp_file("blue_pixel_24.bmp");
  (void)bmp_image;
  // printf("bmp_image -> %d", (int*)bmp_image);

  u32 x                = 0;
  u32 y                = 0;
  u32 width            = 800;
  u32 height           = 600;
  u32 borderWidth      = 0;
  u32 windowDepth      = CopyFromParent;
  u32 windowClass      = CopyFromParent;
  Visual* windowVisual = CopyFromParent;

  u32 attributeValueMask                = CWBackPixel;
  XSetWindowAttributes windowAttributes = {0};
  windowAttributes.background_pixel     = DARK_GREEN;

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
  XEvent generalEvent;

  // XMapWindow https://tronche.com/gui/x/xlib/window/XMapWindow.html
  XMapWindow(mainDisplay, mainWindow);
  // XFlush https://tronche.com/gui/x/xlib/event-handling/XFlush.html
  // XFlush(mainDisplay);

  // https://tronche.com/gui/x/xlib/GC/XCreateGC.html
  // Create a simple graphics context
  GC gc = XCreateGC(mainDisplay, mainWindow, 0, NULL);

  // XSetForeground(mainDisplay, gc, BlackPixel(mainDisplay, screen));
  int screen = DefaultScreen(mainDisplay);

  XImage* image = XCreateImage(
      mainDisplay,
      DefaultVisual(mainDisplay, screen),
      DefaultDepth(mainDisplay, screen),
      ZPixmap,
      0,
      NULL,
      800,
      600,
      32,
      0
  );

  // https://tronche.com/gui/x/xlib/event-handling/XSelectInput.html
  // https://tronche.com/gui/x/xlib/events/mask.html
  XSelectInput(
      mainDisplay,
      mainWindow,
      KeyPressMask | KeyReleaseMask | ExposureMask | ButtonPressMask |
          ButtonReleaseMask | Button1MotionMask
  );

  // https://tronche.com/gui/x/xlib/event-handling/XPending.html
  // while (XPending(mainDisplay) > 0) {}

  // https://gitlab.com/UltimaN3rd/croaking-kero-programming-tutorials/blob/master/opening_a_window_on_linux_with_xlib/opening_a_window_with_xlib.c

  // GC gc = create_x11_graphics_context(mainDisplay, mainWindow, 0);
  //  https://tronche.com/gui/x/xlib/event-handling/XSync.html
  //  Flush the output buffer and wait until all request have been received and
  //  processed by the X server
  // XSync(mainDisplay, False);
  // XFlush(mainDisplay);

  ////XDrawArc(mainDisplay, mainWindow, gc, 50-(30/2), 100-(30/2), 30, 30, 0,
  /// 360*64);
  // XDrawLine(mainDisplay, mainWindow, gc, 10, 60, 180, 20);
  // XFlush(mainDisplay);
  // char* mytext = "This is some text";

  image->data = malloc(image->bytes_per_line * image->height);

  while (running) {
    XNextEvent(mainDisplay, &generalEvent);

    if (generalEvent.type == KeyPress)
      printf("KeyPress: %x\n", generalEvent.xkey.keycode);
    else
      printf("X11 Event: %d\n", generalEvent.type);

    switch (generalEvent.type) {
      // https://tronche.com/gui/x/xlib/events/exposure/expose.html
      case Expose: {
        printf("X11 Expose Event: %d\n", generalEvent.xexpose.type);

        drawToBuffer(WHITE, WHITE, image, gc, mainWindow, mainDisplay);

        // if (generalEvent.xexpose.count) break;
        ////XSetForeground(mainDisplay, gc, WhitePixel(mainDisplay,
        /// screen_num)); /XDrawString(mainDisplay, mainWindow, gc, 10, 10,
        /// mytext, strlen(mytext));
        // XFillRectangle(mainDisplay, mainWindow, gc, 0, 100, 50, 50);
        break;
      }
      case MotionNotify: {
        int symbol = XLookupKeysym(&generalEvent.xkey, 0);
        // Mouse position
        int x = generalEvent.xkey.x;
        int y = generalEvent.xkey.y;

        printf(
            "[DEBUG] MotionNotify symbol: %d, coordinates: [%d, %d]\n",
            symbol,
            x,
            y
        );

        XDrawPoint(mainDisplay, mainWindow, gc, x, y);
        break;
      }
      case ButtonPress: {
        int symbol = XLookupKeysym(&generalEvent.xkey, 0);
        // Mouse position
        int x = generalEvent.xkey.x;
        int y = generalEvent.xkey.y;

        printf(
            "[DEBUG] ButtonPress symbol: %d, coordinates: [%d, %d]\n",
            symbol,
            generalEvent.xkey.x,
            generalEvent.xkey.y
        );

        XDrawPoint(mainDisplay, mainWindow, gc, x, y);
        break;
      }
      case ButtonRelease: {
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
        break;
      }
      case KeyPress: {
        int symbol           = XLookupKeysym(&generalEvent.xkey, 0);
        keyboard[(u8)symbol] = true;

        printf("KeyPress: %x\n", generalEvent.xkey.keycode);

        switch (symbol) {
          case XK_Escape: {
            printf("Escape pressed\n");
          } break;
          // case XK_Pointer_Button1: {
          //   printf("Pointer Button 1 pressed\n");
          //   break;
          // }
          case XK_r: {
            XFlush(mainDisplay);
            XSync(mainDisplay, 1);
            drawToBuffer(WHITE, WHITE, image, gc, mainWindow, mainDisplay);
            break;
          }
          case XK_a: {
            // printf("\"a\" pressed\n");
            XFlush(mainDisplay);
            XSync(mainDisplay, 1);
            drawToBuffer(BLUE, BLUE, image, gc, mainWindow, mainDisplay);
            break;
          }
          case XK_b: {
            drawToBuffer(RED, RED, image, gc, mainWindow, mainDisplay);
            break;
          }
          case XK_q: {
            printf("Closing application\n");
            running = false;
          } break;
        }

      } break;
    }
  }

  // Cleanup
  XDestroyImage(image);
  XFreeGC(mainDisplay, gc);
  XDestroyWindow(mainDisplay, mainWindow);
  // https://tronche.com/gui/x/xlib/display/XCloseDisplay.html
  XCloseDisplay(mainDisplay);

  return 0;
}
#endif // }}}
