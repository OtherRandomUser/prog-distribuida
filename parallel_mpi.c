#include <stdio.h>
#include <string.h>
#include <mpi.h>

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
    // if (argc != 5)
    // {
    //     printf("usage: %s <source-image> <destination-image> "
    //         "<filter-size> <num-threads>\n", argv[0]);
    //     return EXIT_FAILURE;
    // }

    // int filter_size = atoi(argv[3]);
    int filter_size = 3;

    if (filter_size > 15)
    {
        printf("filter size must not be bigger than 15\n");
        return EXIT_FAILURE;
    }

    if (filter_size % 2 == 0)
    {
        printf("filter size must be odd\n");
        return EXIT_FAILURE;
    }

    MPI_Init(&argc, &argv);

    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    int np;
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    struct Image *image = NULL;
    struct BGR *out_buff = NULL;

    if (id == 0)
    {
        image = bmp_from_file("res/borboleta.bmp");
        out_buff = (struct BGR*) malloc(sizeof(struct BGR) * image->buffer_size);
    }

    // broadcast image
    MPI_Bcast(image, sizeof(struct Image), MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(image->buffer, image->buffer_size * sizeof(struct BGR), MPI_CHAR, 0, MPI_COMM_WORLD);
    struct Image *output = copy_bmp(image);

    // apply filter
    unsigned char filter_buffer_red[15 * 15];
    unsigned char filter_buffer_green[15 * 15];
    unsigned char filter_buffer_blue[15 * 15];
    int filter_reach = filter_size / 2;

    int width = image->width;
    int height = image->height;

    int chunk_size = height / np;
    int chunk_start = id * chunk_size;

    int chunk_end = chunk_start + chunk_size;
    chunk_end = chunk_end <= height ? chunk_end : height;

    for (int i = chunk_start; i <  chunk_end; i++)
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

    // gather result
    struct BGR *out_chunk_start =
        output->buffer + chunk_start * width * sizeof(struct BGR);
    int out_chunk_size = (chunk_end - chunk_start) * width * sizeof(struct BGR);

    MPI_Gather(
        // send
        out_chunk_start,
        out_chunk_size,
        MPI_CHAR,

        // recv
        out_buff,
        out_chunk_size,
        MPI_CHAR,

        // root
        0,
        MPI_COMM_WORLD
    );

    if (id == 0)
    {
        memcpy(output->buffer, out_buff, output->buffer_size);
        bmp_to_file("out/borboleta3x3.bmp", output);
    }

    return EXIT_SUCCESS;
}
