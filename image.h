#include <stdlib.h>

#pragma pack(push, 1)

struct Header
{
    unsigned short type;
    unsigned int file_size;
    unsigned short reserved_1;
    unsigned short reserved_2;
    unsigned int offset;
    unsigned int header_size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bits_per_pixel;
    unsigned int compression;
    unsigned int image_size;
    int resolution_width;
    int resolution_height;
    unsigned int total_colors;
    unsigned int important_colors;
};

#pragma pack(pop)

struct BGR
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
};

struct Image
{
    struct Header header;
    struct BGR *buffer;

    int width;
    int height;
    int buffer_size;
};

struct Image* bmp_from_file(const char *const filename);
struct Image* copy_bmp(struct Image *image);
void bmp_to_file(const char *const filename, struct Image *image);
void free_image(struct Image *image);


inline struct BGR* acc(struct Image *image, int i, int j)
{
    if (i < 0 || i > image->height)
        return NULL;

    if (j < 0 || j > image->width)
        return NULL;

    int offset = (i * image->width) + j;
    return image->buffer + offset;
}
