#include <stdio.h>
#include <omp.h>

#include "image.h"

int partition(unsigned char *buffer, int low, int high)
{
    unsigned char pivot = buffer[high];
    int i = low;

    for (int j = low; j < high; j++)
    {
        if (buffer[j] <= pivot)
        {
            unsigned char aux = buffer[i];
            buffer[i] = buffer[j];
            buffer[j] = aux;
            i++;
        }
    }

    unsigned char aux = buffer[i];
    buffer[i] = buffer[high];
    buffer[high] = aux;

    return i;
}

void sort_buffer(unsigned char *buffer, int low, int high)
{
    if (low < high)
    {
        int i = partition(buffer, low, high);

        sort_buffer(buffer, low, i - 1);
        sort_buffer(buffer, i + 1, high);
    }
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        printf("usage: %s <source-image> <destination-image> "
            "<filter-size> <num-threads>\n", argv[0]);
        return 1;
    }

    int filter_size = atoi(argv[3]);
    int num_threads = atoi(argv[4]);

    if (filter_size > 15)
    {
        printf("filter size must not be bigger than 15\n");
        return 1;
    }

    if (filter_size % 2 == 0)
    {
        printf("filter size must be odd\n");
        return 1;
    }

    struct Image *image = bmp_from_file(argv[1]);
    struct Image *output = copy_bmp(image);

    printf("Tamanho da imagem: %u\n", image->header.file_size);
    printf("Largura: %d\n", image->header.width);
    printf("Largura: %d\n", image->header.height);
    printf("Bits por pixel: %d\n", image->header.bits_per_pixel);

    omp_set_num_threads(num_threads);

    #pragma omp parallel
    {
        unsigned char filter_buffer_red[15 * 15];
        unsigned char filter_buffer_green[15 * 15];
        unsigned char filter_buffer_blue[15 * 15];
        int filter_reach = filter_size / 2;

        int width = image->width;
        int height = image->height;

        int id = omp_get_thread_num();

        for (int i = id; i < height; i += num_threads)
        {
            for (int j = 0; j < width; j++)
            {
                int filter_offset = 0;

                for (int r = -filter_reach; r <= filter_reach; r++)
                {
                    for (int c = -filter_reach; c <= filter_reach; c++)
                    {
                        int ii = i + r;
                        int jj = j + c;

                        if (ii > 0 && ii < height && jj > 0 && jj < width)
                        {
                            struct BGR *pixel = acc(image, i + r, j + c);
                            filter_buffer_red[filter_offset] = pixel->red;
                            filter_buffer_green[filter_offset] = pixel->green;
                            filter_buffer_blue[filter_offset] = pixel->blue;
                            filter_offset++;
                        }
                    }
                }

                int mean_offset = filter_offset / 2;
                sort_buffer(filter_buffer_red, 0, filter_offset - 1);
                sort_buffer(filter_buffer_green, 0, filter_offset - 1);
                sort_buffer(filter_buffer_blue, 0, filter_offset - 1);

                struct BGR *pixel = acc(output, i, j);
                pixel->red = filter_buffer_red[mean_offset];
                pixel->green = filter_buffer_green[mean_offset];
                pixel->blue = filter_buffer_blue[mean_offset];
            }
        }
    }

    bmp_to_file(argv[2], output);

    return 0;
}
