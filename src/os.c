/* vi: foldmethod=marker
 */

#include "os.h"

OS_Handle os_file_create(const char* filepath)
{
  OS_Handle result = {0};
  result.handle = cast(void*)-1;

#if defined(_WIN32)
  HANDLE file_handle = CreateFileA(
      filepath,
      GENERIC_WRITE,
      0,
      NULL,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL
      );

  if (file_handle != INVALID_HANDLE_VALUE)
  {
    result.handle = file_handle;
  }
#elif defined(__linux__)

  // Write only, create if doesnt exist, overwrite to 0 if it doesnt exist
  int linux_flags = O_WRONLY | O_CREAT | O_TRUNC;
  int linux_mode  = 0644;
  int file_descriptor = open(filepath, linux_flags, linux_mode);

  if (file_descriptor != -1)
  {
    result.handle = cast(void*)cast(ssize)file_descriptor;
  }

#else
  #error "os_file_create is not implemented for this platform!"
#endif

  return result;
}

OS_Handle os_file_open(const char* filepath)
{
  OS_Handle result = {0};
  result.handle = cast(void*)-1;

#if defined(_WIN32)
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
  HANDLE file_handle = CreateFileA(
      filepath,
      GENERIC_READ,
      FILE_SHARE_READ,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL
  );

  if (file_handle != INVALID_HANDLE_VALUE)
  {
    result.handle = file_handle;
  }

#elif defined(__linux__)
  int file_descriptor = open(
      filepath,
      O_RDONLY
      );

  if (file_descriptor != -1)
  {
    result.handle = cast(void*)cast(ssize)file_descriptor;
  }
#else
  #error "os_file_open is not implemented for this platform!"
#endif

  return result;
}

b32 os_file_read(OS_Handle file, void* buffer, u32 bytes_to_read)
{
  if (file.handle == cast(void*)-1)
  {
    return 0;
  }

#if defined(_WIN32)
  DWORD bytes_read = 0;

  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
  BOOL success = ReadFile(
    cast(HANDLE)file.handle,
    buffer,
    cast(DWORD)bytes_to_read,
    &bytes_read,
    NULL
  );

  if (!success || bytes_read != bytes_to_read)
  {
    DEBUG_LOG("Error reading file");
    return 0;
  }

  return 1;

#elif defined(__linux__)
  int file_descriptor = cast(int)cast(ssize)file.handle;
  ssize_t bytes_read = read(
      file_descriptor,
      buffer,
      cast(usize)bytes_to_read
      );

  if (bytes_read == -1 || cast(u32)bytes_read != bytes_to_read)
  {
    DEBUG_LOG("Error reading file contents");
    return 0;
  }

  return 1;
#else
  #error "os_file_read is not implemented for this platform!"
  return 0;
#endif
}


b32 os_file_write(OS_Handle file, const void* buffer, u32 bytes_to_write)
{
  if (file.handle == cast(void*)-1)
  {
    return 0;
  }

#if defined(_WIN32)

  DWORD bytes_written = 0;

  /* A pointer to an OVERLAPPED structure is required if the hFile parameter
   * was opened with FILE_FLAG_OVERLAPPED, otherwise this parameter can be NULL.
   */
  LPOVERLAPPED ptr_to_overlapped_struct_if_required = NULL;

  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile
  BOOL success = WriteFile(
      cast(HANDLE)file.handle,
      buffer,
      cast(DWORD)bytes_to_write,
      &bytes_written,
      NULL
  );

  if (!success || bytes_written != bytes_written)
  {
    DEBUG_LOG("Error writing file contents.");
    return 0;
  }

  return 1;
#elif defined(__linux__)

  int file_descriptor = cast(int)cast(ssize)file.handle;

  // https://linux.die.net/man/3/write
  ssize_t bytes_written = write(
      file_descriptor,
      buffer,
      cast(usize)&bytes_written
      );

  if (bytes_written == -1 || cast(u32)bytes_written != bytes_to_write)
  {
    DEBUG_LOG("Error writing file contents.");
    return 0;
  }

  return 1;
#else
  #error "os_file_write is not implemented for this platform!"
  return 0;
#endif
}

void os_file_close(OS_Handle file)
{
  if (file.handle != cast(void*)-1)
  {
#if defined(_WIN32)
    HANDLE filehandle = cast(HANDLE)file.handle;
    CloseHandle(filehandle);
#elif defined(__linux__)
    int file_descriptor = cast(int)cast(ssize)file.handle;
    close(file_descriptor);
#else
  #error "os_file_close is not implemented for this platform!"
#endif
  }
}

void os_file_seek(OS_Handle file, u32 offset)
{
  if (file.handle != cast(void*)-1)
  {
#if defined(_WIN32)
    // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfilepointer
    SetFilePointer(cast(HANDLE)file.handle, cast(LONG)offset, NULL, FILE_BEGIN);
#elif defined(__linux__)
    // https://linux.die.net/man/2/lseek
    lseek(cast(int)cast(ssize)file.handle, cast(off_t)offset, SEEK_SET);
#else
  #error "os_file_seek is not implemented for this platform!"
#endif
  }
}
