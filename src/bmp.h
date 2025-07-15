#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__)
#  include <sys/mman.h>
#endif
#if defined(_WIN32)
#  include <windows.h>
#endif

#include "base.h"

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

void print_bmp_image_fields(BitmapImage);
void write_bmp_file(char*, u32, u32, u32, u32, u32, u8*);
BitmapImage read_bmp_file(char* filepath);

void print_bmp_image_fields(BitmapImage bmp_image)
{
  printf(
      "BMP Image { signature = %#010x, filesize = %#010x, offset = %#010x, "
      "dib_header_size = %#010x, image_width = %d, image_height = %d }\n",
      bmp_image.header.signature,
      bmp_image.header.filesize,
      bmp_image.header.offset,
      bmp_image.header.dib_header_size,
      bmp_image.header.image_width,
      bmp_image.header.image_height
  );
}

void write_bmp_file(
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

  FILE* bmp_file = fopen(filepath, "wb+");

  if (bmp_file == NULL) {
    perror("new bmp file could not be created");
    exit(EXIT_FAILURE);
  }

  fwrite(&bmp_image.header, sizeof(BitmapHeader), 1, bmp_file);
  fwrite(bmp_image.data, bmp_image.header.image_size, 1, bmp_file);
}

BitmapImage read_bmp_file(char* filepath)
{
  BitmapImage bmp_image = {0};
  FILE* file            = fopen(filepath, "rb");

  if (file) {
    fseek(file, 0, SEEK_END);
    size_t filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    void* buffer = malloc(filesize + 1);
    (void)buffer;

    // https://www.tutorialspoint.com/c_standard_library/c_function_fread.htm
    // https://cplusplus.com/reference/cstdio/fread/
    // FIX: read each section with fread
    printf("size of BitmapImage struct -> %lu\n", sizeof(BitmapImage));

    usize signature_result =
        fread(&bmp_image.header.signature, sizeof(u16), 1, file);
    if (signature_result != 1)
      fprintf(stderr, "Error reading BMP file signature %s\n", filepath);
    else
      printf("BitmapImage.signature = %#010x\n", bmp_image.header.signature);

    usize filesize_result =
        fread(&bmp_image.header.filesize, sizeof(u32), 1, file);
    if (filesize_result != 1)
      fprintf(stderr, "Error reading BMP file size %s\n", filepath);
    else
      printf(
          "BitmapImage.filesize = %#010x or %d\n",
          bmp_image.header.filesize,
          bmp_image.header.filesize
      );

    // reversed sections
    fread(&bmp_image.header.reserved, sizeof(u16), 1, file);
    fread(&bmp_image.header.reserved2, sizeof(u16), 1, file);

    usize offset_result = fread(&bmp_image.header.offset, sizeof(u32), 1, file);
    if (offset_result != 1)
      fprintf(stderr, "Error reading BMP offset %s\n", filepath);
    else
      printf(
          "bitmap data offset -> %#010x or %d\n",
          bmp_image.header.offset,
          bmp_image.header.offset
      );

    fread(&bmp_image.header.dib_header_size, sizeof(u32), 1, file);
    fread(&bmp_image.header.image_width, sizeof(u32), 1, file);
    fread(&bmp_image.header.image_height, sizeof(u32), 1, file);
    fread(&bmp_image.header.number_of_color_planes, sizeof(u16), 1, file);
    fread(&bmp_image.header.bits_per_pixel, sizeof(u16), 1, file);
    fread(&bmp_image.header.compression_type, sizeof(u32), 1, file);
    fread(&bmp_image.header.image_size, sizeof(u32), 1, file);
    printf("image_size -> %d\n", bmp_image.header.image_size);

    fread(&bmp_image.header.xresolution_ppm, sizeof(s32), 1, file);
    fread(&bmp_image.header.yresolution_ppm, sizeof(s32), 1, file);

    fread(&bmp_image.header.number_of_colors, sizeof(u32), 1, file);
    fread(&bmp_image.header.important_colors, sizeof(u32), 1, file);

    // fseek(file, 0, SEEK_SET);
    fseek(file, bmp_image.header.offset, SEEK_SET);

    // printf("file pos -> %ld\n", ftell(file));
    // u8 first  = 0xb;
    // u8 second = 0xb;
    // u8 third  = 0xb;
    // fread(&first, sizeof(u8), 1, file);
    // printf("first byte -> %d\n", first);
    // fread(&second, sizeof(u8), 1, file);
    // printf("second byte -> %d\n", second);
    // fread(&third, sizeof(u8), 1, file);
    // printf("third byte -> %d\n", third);

    // u8 mydata[192];
    // u8* mydata;

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

    bmp_image.data = (u8*)mmap(addr, mmap_len, mmap_prot, mmap_flags, -1, 0);

    if (bmp_image.data == MAP_FAILED) {
      perror("mmap");
      exit(EXIT_FAILURE);
    }
#endif

#if defined(_WIN32)
    // VirtualALloc https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
    bmp_image.data = (u8*)VirtualAlloc(0, bmp_image.header.image_size, MEM_COMMIT, PAGE_READWRITE);

    if (bmp_image.data == NULL) {
      GetLastError();
      exit(-1);
    }
#endif

    fread(bmp_image.data, sizeof(u8), bmp_image.header.image_size, file);

    printf("\n");
    for (u8 i = 0; i < bmp_image.header.image_size; i += 1) {
      printf("pages[%d] -> %#010x\n", i, *(bmp_image.data + i));
    }

    FILE* new_bmp_file = fopen("test.bmp", "wb+");
    if (new_bmp_file == NULL) {
      perror("new bmp file could not be created");
      exit(EXIT_FAILURE);
    }

    printf("BitmapImage filesize -> %d\n", bmp_image.header.filesize);
    printf(
        "sizeof BitmapImage -> %lu\n",
        sizeof(BitmapHeader) + bmp_image.header.image_size
    );
    // bmp_image.header.filesize = 0x37;
    // bmp_image.header.offset   = 0x36;
    // u8 color = 0xff;
    // fwrite(&bmp_image.header, sizeof(BMPHeader), 1, new_bmp_file);
    // fwrite(&color, sizeof(u32), 1, new_bmp_file);

    fwrite(&bmp_image.header, sizeof(BitmapHeader), 1, new_bmp_file);
    fwrite(bmp_image.data, bmp_image.header.image_size, 1, new_bmp_file);

    u8* data = (u8 *)malloc(sizeof(u32));

    // Green pixel
    data[0] = 0x14;  // Red
    data[1] = 0xff;  // Blue
    data[2] = 0x0;   // Green

   write_bmp_file(
      "test_file.bmp",
      bmp_image.header.filesize,
      bmp_image.header.offset,
      bmp_image.header.image_width,
      bmp_image.header.image_height,
      bmp_image.header.image_size,
      data
    );

    // for (u8 i = 0; i < 192; i += 1) {
    //   printf("mydata[%d] -> %#010x\n", i, mydata[i]);
    // }
    // fread(pages, sizeof(u8*), bmp_image.header.image_size, file);

    // printf("0x1  -> %d\n", *(u8*)(pages + sizeof(u8) * 0));
    // printf("0x2  -> %d\n", *(u8*)(pages + sizeof(u8) * 1));
    // printf("0x3  -> %d\n", *(u8*)(pages + sizeof(u8) * 2));
    // printf("0x18 -> %d\n", *(u8*)(pages + (sizeof(u8) * 23)));
    // printf("mydata -> %d | %d | %d\n", mydata[0], mydata[1], mydata[2]);

    // usize result = fread(&bmp_image, sizeof(BitmapImage), 1, file);
    // if (result != 1) fprintf(stderr, "Error reading BMP file %s\n",
    // filepath); else print_bmp_image_fields(bmp_image);

    fclose(file);
  }

  return bmp_image;
}

// }}}
