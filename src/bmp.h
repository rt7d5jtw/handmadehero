/* vi: foldmethod=marker
 */
#include <stdio.h>
#include <stdlib.h>
#include "base.h"
#include "os.h"

#pragma once

#if defined(__linux__)
#  include <sys/mman.h>
#endif
#if defined(_WIN32)
#  include <windows.h>
#endif

// source: https://engineering.purdue.edu/ece264/16au/hw/HW13
// source:
// http://www.ece.ualberta.ca/~elliott/ee552/studentAppNotes/2003_w/misc/bmp_file_format/bmp_file_format.htm
// For 24-bit representation, 8 bits (1 byte) for RED, 8 bits for GREEN, and 8
// bits for BLUE.

// BMP Definitions {{{

// CIEXYZ:
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-ciexyz
typedef struct CieXyz CieXyz;
struct CieXyz {
  u32 ciexyz_x;
  u32 ciexyz_y;
  u32 ciexyz_z;
};

// CIEXYZTRIPLE:
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-ciexyztriple
typedef struct CieXyz3 CieXyz3;
struct CieXyz3 {
  CieXyz ciexyz_red;
  CieXyz ciexyz_green;
  CieXyz ciexyz_blue;
};

typedef struct BitmapHeader BitmapHeader;
PACK(struct BitmapHeader {
  u16 signature; // 0x4d42 or "BM", 0x42 0x4d
  u32 filesize;
  u16 reserved;
  u16 reserved2;
  u32 offset; // Offset to image data in bytes from beginning of file (54 bytes)

  u32 dib_header_size; // DIB header size in bytes (40 bytes)
  u32 image_width;
  u32 image_height;
  u16 number_of_color_planes;
  u16 bits_per_pixel;
  u32 compression_type;
  u32 image_size;
  s32 xresolution_ppm; // pixels per meter
  s32 yresolution_ppm; // pixels per meter
  u32 number_of_colors;
  u32 important_colors;
});

typedef struct BitmapInfoHeader BitmapInfoHeader;
struct BitmapInfoHeader {
  u32 dib_header_size; // DIB header size in bytes (40 bytes)
  u32 image_width;
  u32 image_height;
  u16 number_of_color_planes;
  u16 bits_per_pixel;
  u32 compression_type;
  u32 image_size;
  s32 xresolution; // pixels per meter
  s32 yresolution; // pixels per meter
  u32 number_of_colors;
  u32 important_colors;
};

// Photoshop's undocumented info header, adds rgb bit mask
typedef struct BitmapInfoHeaderV2 BitmapInfoHeaderV2;
struct BitmapInfoHeaderV2 {
  u32 bi_red_mask;
  u32 bi_green_mask;
  u32 bi_blue_mask;
};

// Photoshop's alpha channel bitmask
// https://web.archive.org/web/20150127132443/https://forums.adobe.com/message/3272950
typedef struct BitmapInfoHeaderV3 BitmapInfoHeaderV3;
struct BitmapInfoHeaderV3 {
  u32 bi_alpha_mask;
};

// V4:
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapv4header
typedef struct BitmapInfoHeaderV4 BitmapInfoHeaderV4;
struct BitmapInfoHeaderV4 {
  u32 bi_cs_type;
  CieXyz3 bi_endpoints;
  u32 bi_gamma_red;
  u32 bi_gamma_green;
  u32 bi_gamma_blue;
};

// V5:
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapv5header
typedef struct BitmapInfoHeaderV5 BitmapInfoHeaderV5;
struct BitmapInfoHeaderV5 {
  u32 bi_intent;
  u32 bi_profile_data;
  u32 bi_profile_size;
  u32 bi_reserved;
};

// typedef struct BMPColorTable BMPColorTable;
// struct BMPColorTable {
//   u8 red;
//   u8 green;
//   u8 blue;
//   u8 reversed;
// };

typedef struct BitmapImage BitmapImage;
PACK(struct BitmapImage {
  BitmapHeader header;
  u8* data;
});

// }}}

// BMP functions {{{

/* debug print */
void print_bmp_image_fields(BitmapImage);
/* write 24 bit bmp file without color space information */
b32 write_bmp_file(char*, u32, u32, u32, u32, u32, u8*);
BitmapImage read_bmp_file(char* filepath);

void print_bmp_image_fields(BitmapImage bmp_image)
{
  DEBUG_LOG(
      "BMP Image { signature = %#010x, filesize = %#010x, offset = %#010x, "
      "dib_header_size = %#010x, image_width = %d, image_height = %d }",
      bmp_image.header.signature,
      bmp_image.header.filesize,
      bmp_image.header.offset,
      bmp_image.header.dib_header_size,
      bmp_image.header.image_width,
      bmp_image.header.image_height
  );
}

b32 write_bmp_file(
    char* filepath,
    u32 filesize,
    u32 offset,
    u32 image_width,
    u32 image_height,
    u32 image_size,
    u8* data_buffer
)
{
  BitmapImage bmp_image = {
      .header =
          {.signature = 0x4d42,
           .filesize  = filesize,
           .reserved  = 0x0,
           .reserved2 = 0x0,
           .offset    = offset,

           .dib_header_size        = 0x28,
           .image_width            = image_width,
           .image_height           = image_height,
           .number_of_color_planes = 0x01,
           .bits_per_pixel         = 0x18,
           .compression_type       = 0x0,
           .image_size             = image_size,
           .xresolution_ppm        = 0x2e23,
           .yresolution_ppm        = 0x2e23,
           .number_of_colors       = 0x0,
           .important_colors       = 0x0},

      .data = data_buffer
  };

  OS_Handle bmp_file = os_file_create(filepath);

  if (bmp_file.handle == cast(void*)-1)
  {
    DEBUG_LOG("Error: new BMP file could not be created at %s", filepath);
    return 0;
  }

  b32 header_written = os_file_write(bmp_file, &bmp_image.header, sizeof(BitmapHeader));
  b32 data_written   = os_file_write(bmp_file, bmp_image.data, bmp_image.header.image_size);

  if (!header_written || !data_written)
  {
    DEBUG_LOG("Error: Failed to completely write BMP data to %s", filepath);
    os_file_close(bmp_file);
    return 0;
  }

  DEBUG_LOG("Successfully wrote BMP to %s", filepath);

  os_file_close(bmp_file);

  return 1;
}

BitmapImage read_bmp_file(char* filepath)
{
  BitmapImage bmp_image = {0};
  OS_Handle file        = os_file_open(filepath);

  if (file.handle != cast(void*) - 1)
  {
     DEBUG_LOG("Successfully opened BMP: %s", filepath);

     b32 header_ok = os_file_read(file, &bmp_image.header, sizeof(BitmapHeader));

     if (!header_ok)
     {
       DEBUG_LOG("Error: failed to read BMP header from %s", filepath);
       os_file_close(file);
       return bmp_image;
     }

    DEBUG_LOG("BitmapImage.signature = %#010x", bmp_image.header.signature);
    DEBUG_LOG("BitmapImage.filesize = %d", bmp_image.header.filesize);
    DEBUG_LOG("Bitmap data offset = %d", bmp_image.header.offset);
    DEBUG_LOG("Image size = %d", bmp_image.header.image_size);

    // seek directly to the pixel data
    os_file_seek(file, bmp_image.header.offset);

#if defined(__linux__)
    // https://man7.org/linux/man-pages/man2/mmap.2.html

    /* addr - hint to the OS kernel to use this address at which the virtual
     * mapping should start in the virtual address space of the process. The
     * value can be specified as NULL to indicate that the kernel can place the
     * virtual mapping anywhere it sees fit. If not NULL, then addr should be a
     * multiple of the page size. */
    void* addr = NULL;
    /* length - This argument specifies the length as number of bytes for the
     * mapping. This length should be a multiple of the page size, although the
     * system automatically aligns the length to be multiple of the underlying
     * page size. */
    u32 mmap_len = bmp_image.header.image_size;
    /* protection for the mapped memory */
    u32 mmap_prot = PROT_READ | PROT_WRITE;
    /* mmap flags */
    u32 mmap_flags = MAP_ANONYMOUS | MAP_PRIVATE;

    bmp_image.data = cast(u8*)mmap(addr, mmap_len, mmap_prot, mmap_flags, -1, 0);

    if (bmp_image.data == MAP_FAILED)
    {
      perror("mmap");
      exit(EXIT_FAILURE);
    }
#endif

#if defined(_WIN32)
    // VirtualALloc
    // https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
    bmp_image.data = (u8*)VirtualAlloc(
        0, bmp_image.header.image_size, MEM_COMMIT, PAGE_READWRITE
    );

    if (bmp_image.data == NULL)
    {
      GetLastError();
      exit(-1);
    }
#endif

    b32 pixels_ok = os_file_read(file, bmp_image.data, bmp_image.header.image_size);
    if (!pixels_ok)
    {
      DEBUG_LOG("Error: failed to read bmp pixel data.");
    }
    else
    {
      DEBUG_LOG("Successfully loaded %d bytes of pixel data!", bmp_image.header.image_size);
    }

    os_file_close(file);
  }

  return bmp_image;
}

// }}}
