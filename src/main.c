#include <stdint.h>
#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define s8  int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t

#define internal static
#define local    static
#define global   static

#if defined(_WIN32)

#  ifndef UNICODE
#    define UNICODE
#  endif

#  include <windows.h>

#  define GetlParamX(lp) ((int)(short)LOWORD(lp))
#  define GetlParamY(lp) ((int)(short)HIWORD(lp))

void* memory;
int clientWidth;
int clientHeight;

#  include <stdbool.h>

#  if RAND_MAX == 32767
#    define Rand32() ((rand() << 16) + (rand() << 1) + (rand() & 1))
#  else
#    define Rand32() rand()
#  endif

/*
  Device-Independent Bitmaps (DIB) https://learn.microsoft.com/en-us/windows/win32/gdi/device-independent-bitmaps
*/

global bool running;

const wchar_t CLASS_NAME[] = L"Sample Window Class";

struct {
  u32 width;
  u32 height;
  u32 *pixels;
} frame = {0};

// Tells GDI the dimensions and color info for DBI, https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfo
global BITMAPINFO bitmapInfo;
global void* bitmapMemory;

// Handle to a GDI bitmap, encapsulates the info and array data
global HBITMAP frameBitmap = 0;
// Device context handle to point to the bitmap handle
global HDC frameDeviceContext = 0;

global void* bitmapMemory;
global u32 bitmapWidth;
global u32 bitmapHeight;
global u32 bytes_per_pixel = 4;

void drawPixel(int x, int y, u32 color) {
  u32* pixel = (u32*) bitmapMemory;
  pixel += y * clientWidth + x;
  *pixel = color;
}

void clearScreen(u32 color) {
  u32* pixel = (u32*) bitmapMemory;
  for (int i = 0; i < clientWidth * clientHeight; ++i) {
    *pixel++ = color;
  }
}

internal void RenderWeirdGradient(u32 x_offset, u32 y_offset) {
  int width = bitmapWidth;
  int height = bitmapHeight;

  // Pitch is the distance, in bytes, between two memory addresses that represent the beginning of one bitmap line and the beginning of the next bitmap line. Because pitch is measured in bytes rather than pixels
  int pitch = width * bytes_per_pixel;
  u8 *row = (u8 *)bitmapMemory;

  for (int y = 0; y < bitmapHeight; ++y) {
    u32 *pixel = (u32 *)row;
    for (int x = 0; x < bitmapWidth; ++x) {
      u8 blue = (x + x_offset);
      u8 green = (y + y_offset);

      *pixel++ = ((green << 8) | blue);
    }

    row += pitch;
  }
}

// GDI is Windows Graphics API

internal void win32ResizeDIBSection(u32 width, u32 height)
{
  if (bitmapMemory) {
   // VirtualFree https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree
   VirtualFree(bitmapMemory, 0, MEM_RELEASE);
  }

  bitmapWidth = width;
  bitmapHeight = height;

  // BITMAPINFO
  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfo
  bitmapInfo.bmiHeader.biSize          = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth         = bitmapWidth;
  bitmapInfo.bmiHeader.biHeight        = -bitmapHeight;
  bitmapInfo.bmiHeader.biPlanes        = 1;
  bitmapInfo.bmiHeader.biBitCount      = 32;
  bitmapInfo.bmiHeader.biCompression   = BI_RGB;

  u32 bytesPerPixel = 4;
  u32 bitmapMemorySize = (bitmapWidth * bitmapHeight) * bytesPerPixel;

  // VirtualALloc https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
  bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  // NOTE: Any time you add or subtract something from a pointer to move it around in memory,
  //       C will silently multiply that movement by the size of the thing being pointed to.
  int pitch = width * bytesPerPixel;
  u8* row = (u8 *)bitmapMemory;
  for (int y = 0; y < bitmapHeight; ++y) {
    u8* pixel = (u8 *)row;
    for (int x = 0; x < bitmapWidth; ++x) {
      /*
                           1  2  3  4
        Pixel in memory:  00 00 00 00
                          RR GG BB xx
        LITTLE ENDIAN     BB GG RR xx
      */
      *pixel = 0;
      ++pixel;

      *pixel = 255;
      ++pixel;

      *pixel = 0;
      ++pixel;

      *pixel = 0;
      ++pixel;
    }

    row += pitch;
  }
}

internal void
win32UpdateWindow(HDC deviceContext, RECT* windowRect, u32 x, u32 y, u32 width, u32 height)
{
  /*
    Stride: is the number of bytes from one row of pixels in memory to the next row of pixels in memory
    Stride is also called pitch. If padding bytes are present, the stride is wider than the width of the image,
    as shown in the following illustration.
  */

  u32 windowWidth = windowRect->right - windowRect->left;
  u32 windowHeight = windowRect->top - windowRect->bottom;

  // StretchDIBits
  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-stretchdibits
  StretchDIBits(
      deviceContext,
      0,//x,
      0,//y,
      bitmapWidth,
      bitmapHeight,
      0,//x,
      0,//y,
      windowWidth,
      windowHeight,
      bitmapMemory,
      &bitmapInfo,
      DIB_RGB_COLORS,
      SRCCOPY
  );
}

LRESULT CALLBACK
win32WndProc(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;

  switch (msg) {
    case WM_KEYDOWN:
    {
      switch (wParam) {
        // Close window from 'Q'
        case 'Q':
        {
          DestroyWindow(windowHandle);
        }
      }
    } break;
    case WM_MOUSEMOVE: {
      if (wParam == MK_LBUTTON) {
        drawPixel(
          GetlParamX(lParam),
          GetlParamY(lParam),
          0xffffff
        );
      }
      break;
    }
    case WM_DESTROY: {
      // PostQuitMessage(int nExitCode)
      // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postquitmessage
      PostQuitMessage(0);
    } break;
    default: {
      result = DefWindowProc(windowHandle, msg, wParam, lParam);
    }
  }

  return result;
}

// https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-winmain
// https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
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
  //MessageBox(NULL, TEXT("Hello, Windows 98!"), TEXT("HelloMsg"), 0);

  // GetStdHandle
  // https://learn.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN
  //HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  //if (stdout && stdout != INVALID_HANDLE_VALUE) {
  //  DWORD written       = 0;
  //  char const* message = "foo bar";
  //  // WriteConsole
  //  // https://learn.microsoft.com/en-us/windows/console/writeconsole
  //  WriteConsole(stdout, message, strlen(message), &written, NULL);
  //}

  // windowClass.lpfnWndProc		= WindowProc;
  // windowClass.hInstance			= hInstance;
  // windowClass.lpszClassName	= CLASS_NAME;

  // RegisterClass(&windowClass);

  // WNDCLASS
  // https://learn.microsoft.com/en-us/previous-versions/ms942860(v=msdn.10)
  // WNDCLASSA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassa
  // WNDCLASSEXA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassexa
  // WNDCLASSEXA wc = {};

  // https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window
  const wchar_t className[] = L"Sample Window Class";
  // Contains window class information
  WNDCLASSEX windowClass = {0};

  // Window handle for
  HWND windowHandle;
  static MSG msg = {0};

  windowClass.cbSize        = sizeof(WNDCLASSEX);
  windowClass.style         = 0;
  windowClass.lpfnWndProc   = win32WndProc;
  windowClass.cbClsExtra    = 0;
  windowClass.cbWndExtra    = 0;
  windowClass.hInstance     = hInstance;
  windowClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  windowClass.lpszMenuName  = NULL;
  // windowClass.lpszClassName = g_szClassName;
  windowClass.lpszClassName = CLASS_NAME;
  windowClass.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

  if (!RegisterClassEx(&windowClass)) {
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox
    MessageBox(
        NULL,
        L"Window Registration Failed!",
        L"Error!",
        MB_ICONEXCLAMATION | MB_OK
    );
    return 0;
  }

  // LPCTSTR
  // https://softwareengineering.stackexchange.com/questions/194764/what-is-lpctstr
  // LPCSTR title = "miukumauku";
  // DWORD dwStyle = WS_OVERLAPPEDWINDOW;
  // int x = 100;
  // int y = 100;
  // int width = 650;
  // int height = 450;
  //// handle of the parent window
  // HWND hWndParent = NULL;
  //// A handle to a menu
  // HMENU hMenu = NULL;
  //// Additiona application data
  // LPVOID lpParam = NULL;

  // CreateWindowExA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa
  // CreateWindowEx
  // https://learn.microsoft.com/en-us/previous-versions/ms960010(v=msdn.10)
  // CreateWindowA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowa
  // hwnd = CreateWindowExA(
  //  // dwExStyle
  //  0,
  //  // lpClassName
  //  title,
  //  // lpWindowName
  //  title,
  //  // https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles
  //  dwStyle,
  //  x,
  //  y,
  //  width,
  //  height,
  //  hWndParent,
  //  hMenu,
  //  hInstance,
  //  lpParam
  //);

  windowHandle = CreateWindowEx(
      0,          // WS_EX_CLIENTEDGE,
      className, // g_szClassName,
      L"The title of my window",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      NULL,
      NULL,
      hInstance,
      NULL
  );

  if (windowHandle == NULL) {
    MessageBox(
        NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK
    );
    return -1;
  }

  if (windowHandle == NULL) {
    MessageBox(
        NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK
    );
    return GetLastError();
  }

  RECT rect;
  GetClientRect(windowHandle, &rect);
  clientWidth = rect.right - rect.left;
  clientHeight = rect.bottom - rect.top;

  // Create the device context handle
  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createcompatibledc
  frameDeviceContext = CreateCompatibleDC(0);

  // GetDC https://learn.microsoft.com/en-
  HDC deviceContext = GetDC(windowHandle);

  bitmapMemory = VirtualAlloc(
    0,
    clientWidth * clientHeight * 4,
    MEM_RESERVE | MEM_COMMIT,
    PAGE_READWRITE
    );

  running = true;

  // ShowWindow
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
  //ShowWindow(hwnd, nCmdShow);
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-updatewindow
  //UpdateWindow(hwnd);

  while (running) {
    // Run the message loop
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagew
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dispatchmessagea
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE) > 0) {
      if (msg.message == WM_QUIT) running = false;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    //static unsigned int p = 0;
    //frame.pixels[(p++) % (frame.width * frame.height)] = Rand32();
    //frame.pixels[Rand32() % (frame.width * frame.height)] = 0;

    InvalidateRect(windowHandle, NULL, FALSE);
    UpdateWindow(windowHandle);
  }

  return msg.wParam;
}
#endif

/*
 * Linux entrypoint
 */

#if defined(__linux__)
#  include <stdio.h>
#  include <unistd.h>
#  include <sys/syscall.h>
#  include <stdbool.h>
#  include <string.h>
#  include <stdlib.h>
#  include <X11/Xlib.h>
#  include <X11/keysym.h>
#  include <X11/Xutil.h>

bool keyboard[256] = {0};
global bool running = true;

#define DARK_GREEN  0x8aa37f
#define BLUE        0x0000ff
#define PINK        0xffa6c9
#define RED         0xcd1c18
#define WHITE       0xffffff

void drawToBuffer(u64 yColor, u64 xColor, XImage* image, GC gc, Window mainWindow, Display* mainDisplay) {
  for (int y = 0; y < 600; y += 1) {
    for (int x = 0; x < 800; x += 1) {
      // Checker pattern
      unsigned long pixel = ((x ^ y) & 1) ? yColor : xColor;
      XPutPixel(image, x, y, pixel);
    }
  }

  //https://tronche.com/gui/x/xlib/graphics/XPutImage.html
  XPutImage(mainDisplay, mainWindow, gc, image, 0, 0, 0, 0, 800, 600);
}


GC create_x11_graphics_context(Display* display, Window window, int reverse_video) {
  GC gc;
  unsigned long valuemask = 0;

  XGCValues values;
  unsigned int line_width = 2;
  int line_style = LineSolid;
  int cap_style = CapButt;
  int join_style = JoinBevel;
  int screen_num = DefaultScreen(display);

  gc = XCreateGC(display, window, valuemask, &values);

  //if (gc < 0) {
  //  fprintf(stderr, "XCreatedGC: \n");
  //}

  if (reverse_video) {
    XSetForeground(display, gc, WhitePixel(display, screen_num));
    XSetBackground(display, gc, WhitePixel(display, screen_num));
  } else {
    XSetForeground(display, gc, BlackPixel(display, screen_num));
    XSetBackground(display, gc, WhitePixel(display, screen_num));
  }

  // Define the style of lines that will be drawn for this GC
  XSetLineAttributes(display, gc, line_width, line_style, cap_style, join_style);

  // Define the fill style for the GC
  XSetFillStyle(display, gc, FillSolid);

  return gc;
}

int main(void)
{
  syscall(SYS_write, 1, "I like pancakes\n", 17);

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

  //XSetForeground(mainDisplay, gc, BlackPixel(mainDisplay, screen));
  int screen = DefaultScreen(mainDisplay);

  XImage * image = XCreateImage(
    mainDisplay,
    DefaultVisual(mainDisplay, screen), DefaultDepth(mainDisplay, screen),
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
  XSelectInput(mainDisplay, mainWindow, KeyPressMask | KeyReleaseMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask);

  // https://tronche.com/gui/x/xlib/event-handling/XPending.html
  //while (XPending(mainDisplay) > 0) {}

  // https://gitlab.com/UltimaN3rd/croaking-kero-programming-tutorials/blob/master/opening_a_window_on_linux_with_xlib/opening_a_window_with_xlib.c

  //GC gc = create_x11_graphics_context(mainDisplay, mainWindow, 0);
  // https://tronche.com/gui/x/xlib/event-handling/XSync.html
  // Flush the output buffer and wait until all request have been received and processed by the X server
  //XSync(mainDisplay, False);
  //XFlush(mainDisplay);

  ////XDrawArc(mainDisplay, mainWindow, gc, 50-(30/2), 100-(30/2), 30, 30, 0, 360*64);
  //XDrawLine(mainDisplay, mainWindow, gc, 10, 60, 180, 20);
  //XFlush(mainDisplay);
  //char* mytext = "This is some text";

  image->data = malloc(image->bytes_per_line * image->height);

  while (running) {
    XNextEvent(mainDisplay, &generalEvent);

    if (generalEvent.type == KeyPress) printf("KeyPress: %x\n", generalEvent.xkey.keycode);
    else printf("X11 Event: %d\n", generalEvent.type);

    switch (generalEvent.type) {
      // https://tronche.com/gui/x/xlib/events/exposure/expose.html
      case Expose: {
        printf("X11 Expose Event: %d\n", generalEvent.xexpose.type);

        drawToBuffer(WHITE, WHITE, image, gc, mainWindow, mainDisplay);

        //if (generalEvent.xexpose.count) break;
        ////XSetForeground(mainDisplay, gc, WhitePixel(mainDisplay, screen_num));
        ////XDrawString(mainDisplay, mainWindow, gc, 10, 10, mytext, strlen(mytext));
        //XFillRectangle(mainDisplay, mainWindow, gc, 0, 100, 50, 50);
        break;
      }
      case MotionNotify: {
        int symbol           = XLookupKeysym(&generalEvent.xkey, 0);
        // Mouse position
        int x = generalEvent.xkey.x;
        int y = generalEvent.xkey.y;

        printf("[DEBUG] MotionNotify symbol: %d, coordinates: [%d, %d]\n", symbol, x, y);

        XDrawPoint(mainDisplay, mainWindow, gc, x, y);
        break;
      }
      case ButtonPress: {
        int symbol           = XLookupKeysym(&generalEvent.xkey, 0);
        // Mouse position
        int x = generalEvent.xkey.x;
        int y = generalEvent.xkey.y;

        printf("[DEBUG] ButtonPress symbol: %d, coordinates: [%d, %d]\n", symbol, generalEvent.xkey.x, generalEvent.xkey.y);

        XDrawPoint(mainDisplay, mainWindow, gc, x, y);
        break;
      }
      case ButtonRelease: {
        int symbol           = XLookupKeysym(&generalEvent.xkey, 0);

        // Mouse position
        int x = generalEvent.xkey.x;
        int y = generalEvent.xkey.y;

        printf("[DEBUG] Mouse button released, symbol: %d, [%d, %d]\n", symbol, x, y);
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
          //case XK_Pointer_Button1: {
          //  printf("Pointer Button 1 pressed\n");
          //  break;
          //}
          case XK_r: {
            XFlush(mainDisplay);
            XSync(mainDisplay, 1);
            drawToBuffer(WHITE, WHITE, image, gc, mainWindow, mainDisplay);
            break;
          }
          case XK_a: {
            //printf("\"a\" pressed\n");
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
#endif
