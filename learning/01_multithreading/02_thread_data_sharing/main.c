#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <err.h>

#define RET_OK          0
#define MAX_THREAD_NUM  100

static int counter;
pthread_mutex_t mutex;

void *thread_func(void *arg)
{
    pthread_mutex_lock(&mutex);
    ++counter;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(void)
{
    pthread_t threads[MAX_THREAD_NUM];

    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < MAX_THREAD_NUM; ++i) {
        if (pthread_create(&threads[i], NULL, thread_func, NULL) != RET_OK) {
            pthread_mutex_destroy(&mutex);
            err(EXIT_FAILURE, "Failed to create threads[%d]", i);
        }
    }

    for (int i = 0; i < MAX_THREAD_NUM; ++i) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    printf("%d\n", counter);

    exit(EXIT_SUCCESS);
}