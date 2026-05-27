/**
 * Example client program that uses thread pool.
 */

#include <stdio.h>
#include <unistd.h>
#include "threadpool.h"

struct data
{
    int a;
    int b;
};

void add(void *param)
{
    struct data *temp;
    temp = (struct data*)param;

    printf("I add two values %d and %d result = %d\n",temp->a, temp->b, temp->a + temp->b);
}

int main(void)
{
    // initialize the thread pool
    pool_init();

    // create some work to do
    struct data work1 = {5, 10};
    struct data work2 = {15, 25};
    struct data work3 = {100, 200};
    struct data work4 = {200, 200};
    struct data work5 = {10, 200};
    struct data work6 = {20, 300};

    // submit the work to the queue
    if (pool_submit(&add, &work1) != 0)
        fprintf(stderr, "Unable to submit work1 to thread pool\n");

    if (pool_submit(&add, &work2) != 0)
        fprintf(stderr, "Unable to submit work2 to thread pool\n");

    if (pool_submit(&add, &work3) != 0)
        fprintf(stderr, "Unable to submit work3 to thread pool\n");

    if (pool_submit(&add, &work4) != 0)
        fprintf(stderr, "Unable to submit work4 to thread pool\n");

    if (pool_submit(&add, &work5) != 0)
        fprintf(stderr, "Unable to submit work5 to thread pool\n");

    if (pool_submit(&add, &work6) != 0)
        fprintf(stderr, "Unable to submit work6 to thread pool\n");

    // give the worker threads time to finish the tasks before shutdown
    sleep(3);

    pool_shutdown();

    return 0;
}
