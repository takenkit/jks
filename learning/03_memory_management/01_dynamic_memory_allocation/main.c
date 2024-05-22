#include <stdio.h>
#include <stdlib.h>
#include <err.h>

int *create_int_array(int size, int init_value)
{
    int *arr;
    arr = (int *)malloc(sizeof(int) * size);
    for (int i = 0; i < size; ++i) {
        arr[i] = init_value;
    }
    return arr;
}

int main(int argc, char *argv[])
{
    int *arr;
    int size;
    int init_value = 0;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <size> [<initial value>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    size = atoi(argv[1]);
    if (size == 0) {
        fprintf(stderr, "size should be int value\n");
        exit(EXIT_FAILURE);
    }

    if (argc == 3) {
        init_value = atoi(argv[2]);
    }
    
    arr = create_int_array(size, init_value);
    if (arr == NULL) {
        fprintf(stderr, "create_int_array() failed\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; ++i) {
        printf("%d", arr[i]);
        if (i == size - 1) {
            printf("\n");
        } else {
            printf(" ");
        }
    }

    if (arr != NULL) {
        free(arr);
    }

    exit(EXIT_SUCCESS);
}