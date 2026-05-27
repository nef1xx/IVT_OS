/**
 * Implementation of thread pool.
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include "threadpool.h"

#define NUMBER_OF_THREADS 5

// this represents work that has to be 
// completed by a thread in the pool
typedef struct task
{
    void (*function)(void *p);
    void *data;
    struct task *next;
}
task;

static task *queue_head = NULL;
static task *queue_tail = NULL;
static pthread_t bees[NUMBER_OF_THREADS];
static pthread_mutex_t queue_mutex;
static pthread_cond_t all_tasks_done;
static sem_t available_tasks;
static int active_tasks = 0;
static int shutting_down = 0;

// insert a task into the queue
// returns 0 if successful or 1 otherwise, 
static int enqueue(task *t)
{
    t->next = NULL;

    if (queue_tail == NULL) {
        queue_head = t;
        queue_tail = t;
    } else {
        queue_tail->next = t;
        queue_tail = t;
    }

    return 0;
}

// remove a task from the queue
static task *dequeue(void)
{
    task *t = queue_head;

    if (t == NULL) {
        return NULL;
    }

    queue_head = queue_head->next;
    if (queue_head == NULL) {
        queue_tail = NULL;
    }

    t->next = NULL;
    return t;
}

// the worker thread in the thread pool
void *worker(void *param)
{
    (void)param;

    while (1) {
        task *worktodo;

        sem_wait(&available_tasks);

        pthread_mutex_lock(&queue_mutex);
        worktodo = dequeue();
        if (worktodo != NULL) {
            active_tasks++;
        }
        pthread_mutex_unlock(&queue_mutex);

        if (worktodo == NULL) {
            continue;
        }

        execute(worktodo->function, worktodo->data);
        free(worktodo);

        pthread_mutex_lock(&queue_mutex);
        active_tasks--;
        if (shutting_down && queue_head == NULL && active_tasks == 0) {
            pthread_cond_signal(&all_tasks_done);
        }
        pthread_mutex_unlock(&queue_mutex);
    }

    return NULL;
}

/**
 * Executes the task provided to the thread pool
 */
void execute(void (*somefunction)(void *p), void *p)
{
    (*somefunction)(p);
}

/**
 * Submits work to the pool.
 */
int pool_submit(void (*somefunction)(void *p), void *p)
{
    task *new_task;

    if (somefunction == NULL) {
        return 1;
    }

    new_task = malloc(sizeof(*new_task));
    if (new_task == NULL) {
        return 1;
    }

    new_task->function = somefunction;
    new_task->data = p;
    new_task->next = NULL;

    pthread_mutex_lock(&queue_mutex);
    if (shutting_down) {
        pthread_mutex_unlock(&queue_mutex);
        free(new_task);
        return 1;
    }

    enqueue(new_task);
    pthread_mutex_unlock(&queue_mutex);
    sem_post(&available_tasks);

    return 0;
}

// initialize the thread pool
void pool_init(void)
{
    int i;

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&all_tasks_done, NULL);
    sem_init(&available_tasks, 0, 0);

    queue_head = NULL;
    queue_tail = NULL;
    active_tasks = 0;
    shutting_down = 0;

    for (i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_create(&bees[i], NULL, worker, NULL);
    }
}

// shutdown the thread pool
void pool_shutdown(void)
{
    int i;

    pthread_mutex_lock(&queue_mutex);
    shutting_down = 1;
    while (queue_head != NULL || active_tasks > 0) {
        pthread_cond_wait(&all_tasks_done, &queue_mutex);
    }
    pthread_mutex_unlock(&queue_mutex);

    for (i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_cancel(bees[i]);
    }

    for (i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_join(bees[i], NULL);
    }

    sem_destroy(&available_tasks);
    pthread_cond_destroy(&all_tasks_done);
    pthread_mutex_destroy(&queue_mutex);
}
