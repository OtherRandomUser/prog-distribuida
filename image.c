#include "image.h"

#include <stdio.h>
#include <string.h>

struct Image* bmp_from_file(const char *const filename)
{
    FILE *input = fopen(filename, "rb");

    if (input == NULL)
    {
        printf("Unable to open input file: %s\n", filename);
        return NULL;
    }

    struct Image *image = (struct Image*) malloc(sizeof(struct Image));

    fread(&image->header, sizeof(struct Header), 1, input);

    int width = image->header.width;
    int height = image->header.height;
    int image_size = width * height;

    image->width = width;
    image->height = height;
    image->buffer_size = image_size;

    image->buffer = (struct BGR*) malloc(sizeof(struct BGR) * image_size);

    int offset = 0;
    char aux[4];

    int ali = (width * 3) % 4;

    if (ali != 0)
        ali = 4 - ali;

    for (int i = 0; i < height; i++)
    {
        fread(image->buffer + offset, sizeof(struct BGR), width, input);
        offset += width;

        if (ali)
            fread(aux, sizeof(unsigned char), ali, input);
    }

    fclose(input);

    return image;
}

struct Image* copy_bmp(struct Image *image)
{
    struct Image *out = (struct Image*) malloc(sizeof(struct Image));
    out->header = image->header;
    out->width = image->width;
    out->height = image->height;
    out->buffer_size = image->buffer_size;
    out->buffer = (struct BGR*) malloc(sizeof(struct BGR) * image->buffer_size);
    printf("Tamanho daaaaa imagem: %u\n", image->buffer_size);
    memcpy(out->buffer, image->buffer, sizeof(struct BGR) * image->buffer_size);
    return out;
}

void bmp_to_file(const char *const filename, struct Image *image)
{
    FILE *output = fopen(filename, "wb");

    if (output == NULL)
    {
        printf("Unable to open output file: %s\n", filename);
        return;
    }

    fwrite(&image->header, sizeof(struct Header), 1, output);

    int width = image->width;
    int height = image->height;
    int image_size = image->buffer_size;

    int offset = 0;

    int ali = (width * 3) % 4;

    if (ali != 0)
        ali = 4 - ali;

    for (int i = 0; i < height; i++)
    {
        fwrite(image->buffer + offset, sizeof(struct BGR), width, output);
        offset += width;

        if (ali)
            fwrite(0, sizeof(unsigned char), ali, output);
    }

    fclose(output);
}

void free_image(struct Image *image)
{
    free(image->buffer);
    free(image);
}

struct BGR* acc(struct Image *image, int i, int j);
