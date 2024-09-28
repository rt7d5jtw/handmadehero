#include <stdint.h>
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define s8 int8_t
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
#endif

#if defined(_WIN32)

#  include <stdbool.h>
global bool running;
global BITMAPINFO bitmapInfo;
global void* bitmapMemory;
global HBITMAP bitmapHandle;
global HDC bitmapDeviceContext;

// GDI is Windows Graphics API

internal void win32ResizeDIBSection(u32 width, u32 height)
{
  if (bitmapHandle) {
    // DeleteObject
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-deleteobject
    DeleteObject(bitmapHandle);
  }
  if (!bitmapDeviceContext) {
    // CreateCompatibleDC
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createcompatibledc
    bitmapDeviceContext = CreateCompatibleDC(0);
  }

  // BITMAPINFO
  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfo
  bitmapInfo.bmiHeader.biSize          = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth         = width;
  bitmapInfo.bmiHeader.biHeight        = height;
  bitmapInfo.bmiHeader.biPlanes        = 1;
  bitmapInfo.bmiHeader.biBitCount      = 32;
  bitmapInfo.bmiHeader.biCompression   = BI_RGB;
  bitmapInfo.bmiHeader.biSizeImage     = 0;
  bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
  bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
  bitmapInfo.bmiHeader.biClrUsed       = 0;
  bitmapInfo.bmiHeader.biClrImportant  = 0;

  // CreateDIBSection
  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createdibsection
  bitmapHandle = CreateDIBSection(
      bitmapDeviceContext, &bitmapInfo, DIB_RGB_COLORS, &bitmapMemory, 0, 0
  );
}

internal void
win32UpdateWindow(HDC deviceContext, u32 x, u32 y, u32 width, u32 height)
{
  // StretchDIBits
  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-stretchdibits
  StretchDIBits(
      deviceContext,
      x,
      y,
      width,
      height,
      x,
      y,
      width,
      height,
      &bitmapMemory,
      &bitmapInfo,
      DIB_RGB_COLORS,
      SRCCOPY
  );
}

// https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window
const wchar_t CLASS_NAME[] = L"Sample Window Class";

LRESULT CALLBACK
win32WndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;

  switch (msg) {
    case WM_SIZE: {
      RECT clientRect;
      // GetClientRect
      // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclientrect
      GetClientRect(window, &clientRect);

      u32 width  = clientRect.right - clientRect.left;
      u32 height = clientRect.bottom - clientRect.top;

      win32ResizeDIBSection(width, height);

      // OutputDebugStringA
      // https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-outputdebugstringa
      OutputDebugStringA("WM_SIZE\n");
    } break;
    case WM_DESTROY: {
      // running = false;
      OutputDebugStringA("WM_DESTROY!\n");
      // PostQuitMessage(int nExitCode)
      // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postquitmessage
      PostQuitMessage(0);
    } break;
    case WM_CLOSE: {
      // running = false;
      DestroyWindow(window);
    } break;
    case WM_ACTIVATEAPP: {
      OutputDebugStringA("WM_ACTIVEAPP\n");
    } break;
    case WM_PAINT: {
      // PAINTSTRUCT
      // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-paintstruct
      // BeginPaint
      // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint
      // EndPaint
      // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-endpaint
      PAINTSTRUCT paint;

      HDC deviceContext = BeginPaint(window, &paint);

      DWORD rasterOp = BLACKNESS;

      u32 x      = paint.rcPaint.left;
      u32 y      = paint.rcPaint.top;
      u32 height = paint.rcPaint.bottom - paint.rcPaint.top;
      u32 width  = paint.rcPaint.right - paint.rcPaint.left;

      // PatBlt
      // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-patblt
      PatBlt(
          // A handle to the device context.
          deviceContext,
          // Size and position args
          x,
          y,
          width,
          height,
          // The raster operation code
          rasterOp
      );
      EndPaint(window, &paint);
    }
    default: {
      result = DefWindowProc(window, msg, wParam, lParam);
    }
  }

  return result;
}

// https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-winmain
// https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow
)
{
  MessageBox(NULL, TEXT("Hello, Windows 98!"), TEXT("HelloMsg"), 0);

  // GetStdHandle https://learn.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN
  HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  if (stdout && stdout != INVALID_HANDLE_VALUE) {
    DWORD written = 0;
    const char* message = "foo bar";
    // WriteConsole https://learn.microsoft.com/en-us/windows/console/writeconsole
    WriteConsole(stdout, message, strlen(message), &written, NULL);
  }

  // wc.lpfnWndProc		= WindowProc;
  // wc.hInstance			= hInstance;
  // wc.lpszClassName	= CLASS_NAME;

  // RegisterClass(&wc);

  // WNDCLASS
  // https://learn.microsoft.com/en-us/previous-versions/ms942860(v=msdn.10)
  // WNDCLASSA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassa
  // WNDCLASSEXA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassexa
  // WNDCLASSEXA wc = {};
  WNDCLASSEX wc;
  HWND hwnd;
  MSG msg;

  wc.cbSize        = sizeof(WNDCLASSEX);
  wc.style         = 0;
  wc.lpfnWndProc   = win32WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszMenuName  = NULL;
  // wc.lpszClassName = g_szClassName;
  wc.lpszClassName = CLASS_NAME;
  wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

  if (!RegisterClassEx(&wc)) {
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

  hwnd = CreateWindowEx(
      0,          // WS_EX_CLIENTEDGE,
      CLASS_NAME, // g_szClassName,
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

  if (hwnd == NULL) {
    MessageBox(
        NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK
    );
    return 0;
  }

  // ShowWindow
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
  ShowWindow(hwnd, nCmdShow);
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-updatewindow
  UpdateWindow(hwnd);

  // Run the message loop
  // https://learn.microsoft.com/en-us/windows/win32/winmsg/using-messages-and-message-queues
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessage
  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}
#endif

#if defined(__linux__)
#  include <unistd.h>
#  include <sys/syscall.h>
#include <X11/Xlib.h>

int main(void)
{
  syscall(SYS_write, 1, "I like pancakes\n", 17);

  u32 x = 0;
  u32 y = 0;
  u32 width = 800;
  u32 height = 600;
  u32 borderWidth = 0;
  u32 windowDepth = CopyFromParent;
  u32 windowClass = CopyFromParent;
  Visual* windowVisual = CopyFromParent;

  u32 attributeValueMask = CWBackPixel;
  XSetWindowAttributes windowAttributes = {0};
  u64 DARK_GREEN = 0x8aa37f;
  windowAttributes.background_pixel = DARK_GREEN;

  // XOpenDisplay https://linux.die.net/man/3/xopendisplay
  Display* mainDisplay = XOpenDisplay(0);
  // XDefaultRootWindow https://tronche.com/gui/x/xlib/display/display-macros.html#DefaultRootWindow
  Window rootWindow = XDefaultRootWindow(mainDisplay);
  // XCreateSimpleWindow https://tronche.com/gui/x/xlib/window/XCreateWindow.html
  // XCreateWindow       https://tronche.com/gui/x/xlib/window/XCreateWindow.html
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
  //XFlush(mainDisplay);

  for (;;) {
    XEvent generalEvent = {0};
    XNextEvent(mainDisplay, &generalEvent);
  }

  return 0;
}
#endif
