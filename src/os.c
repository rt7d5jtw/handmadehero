#include "os.h"

#if defined(_WIN32)

/* Memory operations */

void* os_vm_reserve(u64 reserve_size)
{
  LPVOID desired_base_address = NULL;
  SIZE_T region_size_in_bytes = (SIZE_T)reserve_size;
  DWORD  allocation_type      = MEM_RESERVE;
  DWORD  access_protection    = PAGE_NOACCESS;

  return VirtualAlloc(
    desired_base_address,
    region_size_in_bytes,
    allocation_type,
    access_protection
  );
}

b32 os_vm_commit(void* start_address, u64 commit_size)
{
  LPVOID desired_base_address = (LPVOID)start_address;
  SIZE_T region_size_in_bytes = (SIZE_T)commit_size;
  DWORD  allocation_type      = MEM_COMMIT;
  DWORD  access_protection    = PAGE_READWRITE;

  return VirtualAlloc(
    desired_base_address,
    region_size_in_bytes,
    allocation_type,
    access_protection
  ) != NULL;
}

b32 os_vm_decommit(void* start_address, u64 decommit_size)
{
  LPVOID base_address_of_region = (LPVOID)start_address;
  SIZE_T region_size_in_bytes   = (SIZE_T)decommit_size;
  DWORD  free_operation_type    = MEM_DECOMMIT;

  return VirtualFree(
    base_address_of_region,
    region_size_in_bytes,
    free_operation_type
  ) != 0;
}

b32 os_vm_release(void* start_address, u64 release_size)
{
  (void)release_size;
  LPVOID base_address_of_region = (LPVOID)start_address;
  SIZE_T size_for_release       = 0;
  DWORD  free_operation_type    = MEM_RELEASE;

  return VirtualFree(
    base_address_of_region,
    size_for_release,
    free_operation_type
  ) != 0;
}

u64 os_page_size(void)
{
  static u64 cached_page_size = 0;
  if (!cached_page_size)
  {
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    cached_page_size = (u64)system_info.dwPageSize;
  }

  return cached_page_size;
}

/* File Operations */

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

void os_file_seek(OS_Handle file, u64 offset)
{
  if (file.handle != cast(void*)-1)
  {
    LARGE_INTEGER distance;
    distance.QuadPart = (LONGLONG)offset;
    SetFilePointerEx(cast(HANDLE)file.handle, distance, NULL, FILE_BEGIN);
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
  if (global_win32_performance_counter_frequency == 0)
  {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    global_win32_performance_counter_frequency = (u64)freq.QuadPart;
  }

  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (u64)counter.QuadPart;
}

f32 os_get_time_elapsed_in_microseconds(u64 start_time_in_ticks, u64 end_time_in_ticks)
{
  u64 elapsed_ticks = end_time_in_ticks - start_time_in_ticks;
  f32 elapsed_microseconds =
    ((f32)elapsed_ticks * 1000000.0f) / (f32)global_win32_performance_counter_frequency;

  return elapsed_microseconds;
}

b32 os_file_size(const char* filepath, u64* filesize)
{
  WIN32_FILE_ATTRIBUTE_DATA fileinfo;
  if (GetFileAttributesExA(filepath, GetFileExInfoStandard, &fileinfo))
  {
    *filesize = ((u64)fileinfo.nFileSizeHigh << 32) | fileinfo.nFileSizeLow;
    return 1;
  }

  return 0;
}

// }}}
#elif defined(__linux__)

/* Memory Operations */

void* os_vm_reserve(u64 reserve_size)
{
  void*  hint_address            = NULL;
  size_t region_length           = (size_t)reserve_size;
  int    memory_protection       = PROT_NONE;
  int    mapping_flags           = MAP_PRIVATE | MAP_ANONYMOUS;
  int    backing_file_descriptor = -1;
  off_t  file_offset             = 0;

  void* memory = mmap(
    hint_address,
    region_length,
    memory_protection,
    mapping_flags,
    backing_file_descriptor,
    file_offset
  );

  if (memory == MAP_FAILED)
    return NULL;
  return memory;
}

b32 os_vm_commit(void* start_address, u64 commit_size)
{
  size_t region_length     = (size_t)commit_size;
  int    memory_protection = PROT_READ | PROT_WRITE;

  return mprotect(start_address, region_length, memory_protection) == 0;
}

b32 os_vm_decommit(void* start_address, u64 decommit_size)
{
  size_t region_length     = (size_t)decommit_size;
  int    advice            = MADV_DONTNEED;
  int    memory_protection = PROT_NONE;

  madvise(start_address, region_length, advice);
  return mprotect(start_address, region_length, memory_protection) == 0;
}

b32 os_vm_release(void* start_address, u64 release_size)
{
  size_t region_length = (size_t)release_size;

  return munmap(start_address, region_length) == 0;
}

u64 os_page_size(void)
{
  static u64 cached_page_size = 0;
  if (!cached_page_size)
  {
    long pagesize = sysconf(_SC_PAGESIZE);
    cached_page_size = (u64)pagesize;
  }

  return cached_page_size;
}

/* File Operations -- Linux */

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

void os_file_seek(OS_Handle file, u64 offset)
{
  if (file.handle != cast(void*)-1)
  {
    int file_descriptor = cast(int)cast(ssize)file.handle;
    lseek(file_descriptor, cast(off_t)offset, SEEK_SET);
  }
}

b32 os_file_size(const char* filepath, u64* filesize)
{
  struct stat fileinfo;
  if (stat(filepath, &fileinfo) == 0)
  {
    *filesize = (u64)fileinfo.st_size;
    return 1;
  }

  return 0;
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
#else
#  warning "Unsupported platform for OS layer!"
#endif
