/* vi: foldmethod=marker
 */

#include "os.h"

#if defined(_WIN32)

// Memory functions -- win32 {{{

void* os_memory_reserve(usize allocation_size)
{
  // https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc2
  void* memory = VirtualAlloc(0, allocation_size, MEM_RESERVE, PAGE_READWRITE);
  return memory;
}

/* returns true based on if memory was committed */
internal b32 os_memory_commit(void* memptr, usize allocation_size)
{
  // https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree#return-value
  // unlike in unix systems, the return value is nonzero for success and zero for failure
  // so we return true on success and false on failure.
  b32 result = (VirtualAlloc(memptr, allocation_size, MEM_COMMIT, PAGE_READWRITE) != 0);
  return result;
}

/* returns 0 on failure, on success returns nonzero value.
 * NOTE: usize is not used for win32 implementation, but is used for other
 *      operating systems, so it is still provided for the sake of completeness.
 */
internal b32 os_memory_decommit(void* memptr, usize allocation_size)
{
  // https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree
  return (VirtualFree(memptr, allocation_size, MEM_DECOMMIT) != 0);
}

/* returns 0 on failure, on success returns nonzero value. */
internal b32 os_memory_release(void* memptr, usize allocation_size)
{
  // https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree
  return (VirtualFree(memptr, allocation_size, MEM_RELEASE) != 0);
}

// }}}

// File Operations -- win32 {{{

global u64 global_win32_performance_counter_frequency = 0;

OS_Handle os_file_open(const char* filepath, u32 flags)
{
  OS_Handle result = {0};
  result.handle = cast(void*)-1;

  DWORD desired_access = 0;
  DWORD share_mode = 0;
  LPSECURITY_ATTRIBUTES security_attributes = NULL;
  DWORD creation_disposition = OPEN_EXISTING;
  DWORD flags_and_attributes = FILE_ATTRIBUTE_NORMAL;
  HANDLE template_file = NULL;

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
      security_attributes,
      creation_disposition,
      flags_and_attributes,
      template_file
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
  LPOVERLAPPED overlapped = NULL;

  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
  BOOL success = ReadFile(
    cast(HANDLE)file.handle,
    buffer,
    cast(DWORD)bytes_to_read,
    &bytes_read,
    overlapped
  );

  if (!success || bytes_read != bytes_to_read)
  {
    //DEBUG_LOG("Error reading file");
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
      ptr_to_overlapped_struct_if_required
  );

  if (!success || bytes_written != bytes_to_write)
  {
    //DEBUG_LOG("Error writing file contents.");
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

/*
 * Returns hardware ticks.
 * QueryPerformanceCounter can have a resolution of up to ONE millisecond.
 *
 * QueryPerformanceCounter obtains the current reading of the system's
 * high-performance timer. The high-performance timer is a counter that the
 * system increments many times a second. To find out how quickly the timer is
 * incremented, use QueryPerformanceFrequency. If you know its frequency, you
 * can use the high-performance timer to measure small intervals of time precisely.
 *
 * However, keep in mind that not all computers support a high-performance timer.
 *
 * Windows 95: Supported.
 * Windows 98: Supported.
 * Windows NT: Required Windows NT 3.1 or later.
 * Windows 2000: Supported.
 * Windows CE: Not Supported.
 */
u64 os_get_time(void)
{
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  u64 hardware_ticks = (u64)counter.QuadPart;
  return hardware_ticks;
}

f32 os_get_time_elapsed_in_microseconds(u64 start_time_in_ticks, u64 end_time_in_ticks)
{
  u64 elapsed_ticks = end_time_in_ticks - start_time_in_ticks;
  f32 elapsed_microseconds =
    ((f32)elapsed_ticks * 1000000.0f) / (f32)global_win32_performance_counter_frequency;

  return elapsed_microseconds;
}

b32 os_file_size(char * filepath, u64* filesize)
{
  b32 result = 0;

  HANDLE filehandle = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (filehandle == INVALID_HANDLE_VALUE)
  {
    return 0;
  }

  LARGE_INTEGER win32_filesize;
  if (GetFileSizeEx(filehandle, &win32_filesize))
  {
    *filesize = (u64)win32_filesize.QuadPart;
    result = 1;
  }

  CloseHandle(filehandle);
  return result;
}

// }}}

#elif defined(__linux__)

// Memory functions -- Linux {{{

internal void* os_memory_reserve(usize allocation_size)
{
  // https://man7.org/linux/man-pages/man2/mmap.2.html
  void* memory = mmap(0, allocation_size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (memory == MAP_FAILED)
  {
    memory = 0;
  }

  return memory;
}

/* returns true based on if memory was committed
 * NOTE:
 *     VirtualAlloc returns NULL on failure and mprotect returns -1 on error,
 *     but on success it returns 0 (opposite for VirtualProtect since NULL == 0).
 */
internal b32 os_memory_commit(void* memptr, usize allocation_size)
{
  // https://man7.org/linux/man-pages/man2/mprotect.2.html
  // NOTE: https://man7.org/linux/man-pages/man2/mprotect.2.html#ERRORS
  b32 result = (mprotect(memptr, allocation_size, PROT_READ | PROT_WRITE) != -1);
  return result;
}

/* returns -1 on failure, returns 0 on success */
internal b32 os_memory_decommit(void* memptr, usize allocation_size)
{
  // https://man7.org/linux/man-pages/man2/madvise.2.html
  b32 madv_result = madvise(memptr, allocation_size, MADV_DONTNEED);
  // https://man7.org/linux/man-pages/man2/mprotect.2.html
  b32 prot_result = mprotect(memptr, allocation_size, PROT_NONE);

  return !(madv_result  | prot_result);
}

/* returns -1 on failure, otherwise returns 0 */
internal b32 os_memory_release(void* memory_ptr, usize allocation_size)
{
  // https://man7.org/linux/man-pages/man3/munmap.3p.html
  //u32 result = munmap(memory_ptr, allocation_size);
  //return result;
  return (munmap(memory_ptr, allocation_size) != -1);
}

// }}}

// File Operations -- Linux {{{

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
    //DEBUG_LOG("Error reading file contents");
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
    //DEBUG_LOG("Error writing file contents.");
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

b32 os_file_size(char * filepath, u64* filesize)
{
  b32 result = 0;
  struct stat fileinfo;

  int fd = open(filepath, O_RDONLY);
  if (fd == -1)
  {
    return 0;
  }

  if (fstat(fd, &fileinfo) == 0)
  {
    *filesize = (u64)fileinfo.st_size;
    result = 1;
  }

  close(fd);
  return result;
}

/*
 * tv_sec: Whole seconds since the computer booted.
 * tv_nsec: The leftover nanoseconds that haven't quite made up a full second yet.
 *
 * If your computer has been running for exactly 5 seconds and 500
 * nanoseconds, the struct looks like this:
 *   tv_sec = 5
 *   tv_nsec = 500
 *
 * Gives back monotonic nanoseconds, that is, nanoseconds since the system boot.
 */
u64 os_get_time(void)
{
  struct timespec counter;
  clock_gettime(CLOCK_MONOTONIC_RAW, &counter);
  u64 nanoseconds_since_boot = ((u64)counter.tv_sec * BILLION) + (u64)counter.tv_nsec;
  return nanoseconds_since_boot;
}

f32 os_get_time_elapsed_in_microseconds(u64 start_time_in_nanoseconds, u64 end_time_in_nanoseconds)
{
  f32 elapsed_microseconds = ((f32)(end_time_in_nanoseconds - start_time_in_nanoseconds) / 1000.0f);
  return elapsed_microseconds;
}
// }}}
#endif
