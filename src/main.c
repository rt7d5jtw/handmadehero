#if defined(_WIN32)

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#endif

#if defined(_WIN32)

// https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window
const wchar_t CLASS_NAME[] = L"Sample Window Class";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;

    switch(msg)
    {
        case WM_SIZE: {
          // https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-outputdebugstringa
          OutputDebugStringA("WM_SIZE\n");
        } break;
        case WM_DESTROY: {
          OutputDebugStringA("WM_DESTROY!\n");
          PostQuitMessage(0);
        } break;
        case WM_CLOSE: {
            DestroyWindow(hwnd);
        } break;
        case WM_ACTIVATEAPP: {
          OutputDebugStringA("WM_ACTIVEAPP\n");
        } break;
        case WM_PAINT: {
          // PAINTSTRUCT  https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-paintstruct
          // BeginPaint   https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint
          // EndPaint     https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-endpaint
          PAINTSTRUCT paint;

          HDC deviceContext = BeginPaint(hwnd, &paint);

          DWORD rasterOp = BLACKNESS;

          int x             = paint.rcPaint.left;
          int y             = paint.rcPaint.top;
          int height        = paint.rcPaint.bottom - paint.rcPaint.top;
          int width         = paint.rcPaint.right - paint.rcPaint.left;

          // PatBlt       https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-patblt
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
          EndPaint(hwnd, &paint);
        }
        default: {
          result = DefWindowProc(hwnd, msg, wParam, lParam);
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
) {
  MessageBox(NULL, TEXT("Hello, Windows 98!"), TEXT("HelloMsg"), 0);

  //wc.lpfnWndProc		= WindowProc;
  //wc.hInstance			= hInstance;
  //wc.lpszClassName	= CLASS_NAME;

  //RegisterClass(&wc);

  // WNDCLASS     https://learn.microsoft.com/en-us/previous-versions/ms942860(v=msdn.10)
  // WNDCLASSA    https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassa
  // WNDCLASSEXA  https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassexa
  //WNDCLASSEXA wc = {};
  WNDCLASSEX wc;
  HWND hwnd;
  MSG msg;

  wc.cbSize        = sizeof(WNDCLASSEX);
  wc.style         = 0;
  wc.lpfnWndProc   = WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc.lpszMenuName  = NULL;
  //wc.lpszClassName = g_szClassName;
  wc.lpszClassName = CLASS_NAME;
  wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

  if(!RegisterClassEx(&wc))
    {
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
  //LPCSTR title = "miukumauku";
  //DWORD dwStyle = WS_OVERLAPPEDWINDOW;
  //int x = 100;
  //int y = 100;
  //int width = 650;
  //int height = 450;
  //// handle of the parent window
  //HWND hWndParent = NULL;
  //// A handle to a menu
  //HMENU hMenu = NULL;
  //// Additiona application data
  //LPVOID lpParam = NULL;

  // CreateWindowExA  https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa
  // CreateWindowEx   https://learn.microsoft.com/en-us/previous-versions/ms960010(v=msdn.10)
  // CreateWindowA    https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowa
  //hwnd = CreateWindowExA(
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
    0,//WS_EX_CLIENTEDGE,
    CLASS_NAME,//g_szClassName,
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

  if (hwnd == NULL)
  {
      MessageBox(
        NULL,
        L"Window Creation Failed!",
        L"Error!",
        MB_ICONEXCLAMATION | MB_OK
        );
      return 0;
  }

  // ShowWindow https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
  ShowWindow(hwnd, nCmdShow);
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-updatewindow
  UpdateWindow(hwnd);

  // Run the message loop
  // https://learn.microsoft.com/en-us/windows/win32/winmsg/using-messages-and-message-queues
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessage
  while(GetMessage(&msg, NULL, 0, 0) > 0)
  {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
  }

  return msg.wParam;
}
#endif

#if defined(__linux__)
#include <unistd.h>
#include <sys/syscall.h>

int main(void) {
  syscall(SYS_write, 1, "I like pancakes\n", 17);
  return 0;
}
#endif
