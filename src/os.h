#ifndef OS_H
#define OS_H

#include "base.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Memory operations
/* Returns NULL on failure. */
void* os_vm_reserve (u64 reserve_size);
/* Return non-zero (true) on success, zero (false) on failure. */
b32   os_vm_commit  (void* start_address, u64 commit_size);
b32   os_vm_decommit(void* start_address, u64 decommit_size);
b32   os_vm_release (void* start_address, u64 release_size);
u64   os_page_size(void);

/* We use a generic OS_Handle for files,
 * but could expanded later for network sockets,
 * threads, or memory maps.
 *
 * NOTE: On both Windows and Linux, the invalid handle state evaluates to -1.
 * - Windows: INVALID_HANDLE_VALUE is defined as (HANDLE)-1
 * - Linux: An invalid file descriptor returns -1
 */
typedef struct OS_Handle OS_Handle;
struct OS_Handle
{
  void* handle;
};

typedef enum OS_AccessFlags
{
  OS_AccessFlags_Read    = (1 << 0), // r
  OS_AccessFlags_Write   = (1 << 1), // w
  OS_AccessFlags_Execute = (1 << 2), // x
  OS_AccessFlags_Create  = (1 << 3)
} OS_AccessFlags;

// File operations
OS_Handle os_file_open(const char* filepath, u32 flags);
b32       os_file_read(OS_Handle, void* buffer, u32 bytes_to_read);
b32       os_file_write(OS_Handle file, const void* buffer, u32 bytes_to_write);
void      os_file_close(OS_Handle file);
void      os_file_seek(OS_Handle file, u64 offset);
b32       os_file_size(const char * filepath, u64* filesize);

// timer operations
u64 os_get_time(void);
f32 os_get_time_elapsed_in_microseconds(u64 start_time, u64 end_time);

#if defined(_WIN32)
#  include <windows.h>
#elif defined(__linux__)
#  include <sys/stat.h>
#  include <sys/mman.h>
#  include <fcntl.h>
#  include <unistd.h>
#  include <time.h>
#  define BILLION 1000000000ULL
#else
#  if defined(_MSC_VER)
#    pragma message("Unsupported platform for os layer!")
#  else
#    warning "Unsupported platform for os layer!"
#  endif
#endif

#endif /* OS_H */
