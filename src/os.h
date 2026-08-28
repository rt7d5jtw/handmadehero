/* vi: foldmethod=marker
 */

#pragma once

#include "base.h"

#if defined(_WIN32)
#  include <windows.h>
#elif defined(__linux__)
#  include <fcntl.h>
#  include <unistd.h>
#endif

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

OS_Handle os_file_open(const char* filepath, u32 flags);
OS_Handle os_file_create(const char* filepath, u32 flags);
b32 os_file_read(OS_Handle, void* buffer, u32 bytes_to_read);
b32 os_file_write(OS_Handle file, const void* buffer, u32 bytes_to_write);
void os_file_close(OS_Handle file);
void os_file_seek(OS_Handle file, u32 offset);

/*
void* os_memory_reserve(usize allocation_size);
b32 os_memory_commit(void* memory_ptr,  usize allocation_size);
b32 os_memory_commit(void* memory_ptr,  usize allocation_size);
b32 os_memory_release(void* memory_ptr, usize allocation_size);
*/
