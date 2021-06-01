#include <stdio.h>
#include <pthread.h>

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

struct ApplyMeanFilterArgs
{
    int filter_size;
    struct Image* input;
    struct Image* output;
    int starting_row;
    int stride;
};

void* apply_mean_filter(void *args)
{
    if (args == NULL)
    {
        printf("args must not be null\n");
        return NULL;
    }

    struct ApplyMeanFilterArgs* cargs = (struct ApplyMeanFilterArgs*) args;

    if (cargs->filter_size > 15)
    {
        printf("filter size must not be bigger than 15\n");
        return NULL;
    }

    if (cargs->filter_size % 2 == 0)
    {
        printf("filter size must be odd\n");
        return NULL;
    }

    struct Image *input = cargs->input;

    if (input == NULL)
    {
        printf("input image must not be null\n");
        return NULL;
    }

    struct Image *output = cargs->output;

    if (output == NULL)
    {
        printf("output image must not be null\n");
        return NULL;
    }

    if (cargs->starting_row < 0 || cargs->starting_row >= input->height)
    {
        printf("starting row must be within the bounds of the image\n"
            "was given: %d\n", cargs->starting_row);
        return NULL;
    }

    int stride = cargs->stride;

    if (stride <= 0)
    {
        printf("stride must be positive\n");
        return NULL;
    }

    unsigned char filter_buffer_red[15 * 15];
    unsigned char filter_buffer_green[15 * 15];
    unsigned char filter_buffer_blue[15 * 15];
    int filter_reach = cargs->filter_size / 2;

    int width = input->width;
    int height = input->height;

    for (int i = cargs->starting_row; i < height; i += stride)
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
                        struct BGR *pixel = acc(input, i + r, j + c);
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

    return NULL;
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

    struct ApplyMeanFilterArgs *args = (struct ApplyMeanFilterArgs*) malloc(
        sizeof(struct ApplyMeanFilterArgs) * num_threads);
    pthread_t *threads = (pthread_t*) malloc(sizeof(pthread_t) * num_threads);

    for (int i = 0; i < num_threads; i++)
    {
        args[i].filter_size = filter_size;
        args[i].input = image;
        args[i].output = output;
        args[i].starting_row = i;
        args[i].stride = num_threads;

        printf("output %p\n", output);

        int error_code = pthread_create(
            &threads[i], NULL, apply_mean_filter, &args[i]);

        if (error_code > 0)
        {
            printf("failed to create thread %d with error code %d\n",
                i, error_code);
            return 1;
        }

        printf("started thread %d\n", i);
    }

    for (int i = 0; i < num_threads; i++)
    {
        int error_code = pthread_join(threads[i], NULL);

        if (error_code > 0)
        {
            printf("failed to join thread %d\n", i);
        }
        else
        {
            printf("successfully joined thread %d\n", i);
        }
    }

    bmp_to_file(argv[2], output);

    free(threads);
    free(args);
    free_image(image);
    free_image(output);

    return 0;
}
