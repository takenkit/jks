#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *thread_func(void *arg)
{
    puts("Hello from thread!");
    return NULL;
}

int main(void)
{
    pthread_t thread;

    if (pthread_create(&thread, NULL, thread_func, NULL) == 0) {
       pthread_join(thread, NULL); 
       exit(EXIT_SUCCESS);
    }

    exit(EXIT_FAILURE);
}