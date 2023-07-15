#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(_WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine,
                   int iCmdShow) {
  MessageBox(NULL, TEXT("Hello, Windows 98!"), TEXT("HelloMsg"), 0);
  return 0;
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
