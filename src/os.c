/* vi: foldmethod=marker
 */

#include "os.h"

#if defined(_WIN32)
// Win32 implementation {{{

OS_Handle os_file_open(const char* filepath, u32 flags)
{
  OS_Handle result = {0};
  result.handle = cast(void*)-1;

  DWORD desired_access = 0;
  DWORD share_mode = 0;
  DWORD creation_disposition = OPEN_EXISTING;

  if (flags & OS_AccessFlags_Read)
  {
    desired_access |= GENERIC_READ;
    share_mode = FILE_SHARE_READ;
  }

  if (flags & OS_AccessFlags_Write)
  {
    desired_access |= GENERIC_WRITE;
  }

  if (flags & OS_AccessFlags_Execute)
  {
    desired_access |= GENERIC_EXECUTE;
  }

  if (flags & OS_AccessFlags_Create)
  {
    creation_disposition = CREATE_ALWAYS;
  }

  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
  HANDLE file_handle = CreateFileA(
      filepath,
      desired_access,
      share_mode,
      NULL,
      creation_disposition,
      FILE_ATTRIBUTE_NORMAL,
      NULL
  );

  if (file_handle != INVALID_HANDLE_VALUE)
  {
    result.handle = file_handle;
  }

  return result;
}

b32 os_file_read(OS_Handle file, void* buffer, u32 bytes_to_read)
{
  if (file.handle == cast(void*)-1)
  {
    return 0;
  }

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
}

b32 os_file_write(OS_Handle file, const void* buffer, u32 bytes_to_write)
{
  if (file.handle == cast(void*)-1)
  {
    return 0;
  }

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

  if (!success || bytes_written != bytes_to_write)
  {
    DEBUG_LOG("Error writing file contents.");
    return 0;
  }

  return 1;
}

void os_file_close(OS_Handle file)
{
  if (file.handle != cast(void*)-1)
  {
    HANDLE filehandle = cast(HANDLE)file.handle;
    CloseHandle(filehandle);
  }
}

void os_file_seek(OS_Handle file, u32 offset)
{
  if (file.handle != cast(void*)-1)
  {
    // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfilepointer
    SetFilePointer(cast(HANDLE)file.handle, cast(LONG)offset, NULL, FILE_BEGIN);
  }
}

// }}}

#elif defined(__linux__)
// Linux implementation {{{

OS_Handle os_file_open(const char* filepath, u32 flags)
{
  OS_Handle result = {0};
  result.handle    = cast(void*)-1;

  int linux_mode   = 0644; // rw-r--r--
  int linux_flags  = 0;

  if ((flags & OS_AccessFlags_Read) && (flags & OS_AccessFlags_Write))
  {
    linux_flags |= O_RDWR;
  }
  else if (flags & OS_AccessFlags_Write)
  {
    linux_flags |= O_WRONLY;
  }
  else if (flags & OS_AccessFlags_Read)
  {
    linux_flags |= O_RDONLY;
  }

  if (flags & OS_AccessFlags_Create)
  {
    linux_flags |= O_CREAT | O_TRUNC;
  }

  if (flags & OS_AccessFlags_Execute)
  {
    linux_mode = 0755; // rwxr-xr-x
  }

  int file_descriptor = open(
      filepath,
      linux_flags,
      linux_mode
      );

  if (file_descriptor != -1)
  {
    result.handle = cast(void*)cast(ssize)file_descriptor;
  }

  return result;
}

b32 os_file_read(OS_Handle file, void* buffer, u32 bytes_to_read)
{
  if (file.handle == cast(void*)-1)
  {
    return 0;
  }

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
}

b32 os_file_write(OS_Handle file, const void* buffer, u32 bytes_to_write)
{
  if (file.handle == cast(void*)-1)
  {
    return 0;
  }

  int file_descriptor = cast(int)cast(ssize)file.handle;

  // https://linux.die.net/man/3/write
  ssize_t bytes_written = write(
      file_descriptor,
      buffer,
      cast(usize)bytes_to_write
      );

  if (bytes_written == -1 || cast(u32)bytes_written != bytes_to_write)
  {
    DEBUG_LOG("Error writing file contents.");
    return 0;
  }

  return 1;
}

void os_file_close(OS_Handle file)
{
  if (file.handle != cast(void*)-1)
  {
    int file_descriptor = cast(int)cast(ssize)file.handle;
    close(file_descriptor);
  }
}

void os_file_seek(OS_Handle file, u32 offset)
{
  if (file.handle != cast(void*)-1)
  {
    // https://linux.die.net/man/2/lseek
    int file_descriptor = cast(int)cast(ssize)file.handle;
    lseek(file_descriptor, cast(off_t)offset, SEEK_SET);
  }
}

// }}}
#endif
