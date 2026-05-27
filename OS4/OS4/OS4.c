#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMBER_OF_THREADS 4

typedef struct task {
    void (*function)(void *);
    void *data;
    struct task *next;
} task_t;

static pthread_t workers[NUMBER_OF_THREADS];
static pthread_mutex_t queue_mutex;
static pthread_cond_t all_tasks_done;
static sem_t available_tasks;

static task_t *queue_head = NULL;
static task_t *queue_tail = NULL;
static int active_tasks = 0;
static int shutting_down = 0;

static void execute(task_t *task)
{
    task->function(task->data);
}

static void enqueue(task_t *task)
{
    task->next = NULL;

    if (queue_tail == NULL) {
        queue_head = task;
        queue_tail = task;
    } else {
        queue_tail->next = task;
        queue_tail = task;
    }
}

static task_t *dequeue(void)
{
    task_t *task = queue_head;

    if (task == NULL) {
        return NULL;
    }

    queue_head = queue_head->next;
    if (queue_head == NULL) {
        queue_tail = NULL;
    }

    task->next = NULL;
    return task;
}

static void *worker(void *param)
{
    (void)param;

    for (;;) {
        sem_wait(&available_tasks);

        pthread_mutex_lock(&queue_mutex);
        task_t *task = dequeue();
        if (task != NULL) {
            active_tasks++;
        }
        pthread_mutex_unlock(&queue_mutex);

        if (task == NULL) {
            continue;
        }

        execute(task);
        free(task);

        pthread_mutex_lock(&queue_mutex);
        active_tasks--;
        if (shutting_down && queue_head == NULL && active_tasks == 0) {
            pthread_cond_signal(&all_tasks_done);
        }
        pthread_mutex_unlock(&queue_mutex);
    }

    return NULL;
}

void pool_init(void)
{
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&all_tasks_done, NULL);
    sem_init(&available_tasks, 0, 0);

    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_create(&workers[i], NULL, worker, NULL);
    }
}

int pool_submit(void (*somefunction)(void *p), void *p)
{
    if (somefunction == NULL) {
        return 1;
    }

    task_t *task = malloc(sizeof(*task));
    if (task == NULL) {
        return 1;
    }

    task->function = somefunction;
    task->data = p;
    task->next = NULL;

    pthread_mutex_lock(&queue_mutex);
    if (shutting_down) {
        pthread_mutex_unlock(&queue_mutex);
        free(task);
        return 1;
    }

    enqueue(task);
    pthread_mutex_unlock(&queue_mutex);
    sem_post(&available_tasks);

    return 0;
}

void pool_shutdown(void)
{
    pthread_mutex_lock(&queue_mutex);
    shutting_down = 1;
    while (queue_head != NULL || active_tasks > 0) {
        pthread_cond_wait(&all_tasks_done, &queue_mutex);
    }
    pthread_mutex_unlock(&queue_mutex);

    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_cancel(workers[i]);
    }

    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_join(workers[i], NULL);
    }

    sem_destroy(&available_tasks);
    pthread_cond_destroy(&all_tasks_done);
    pthread_mutex_destroy(&queue_mutex);
}

static void print_task(void *param)
{
    int value = *(int *)param;

    printf("Task %d started\n", value);
    sleep(1);
    printf("Task %d finished\n", value);
}

int main(void)
{
    int values[8];

    pool_init();

    for (int i = 0; i < 8; i++) {
        values[i] = i + 1;
        if (pool_submit(print_task, &values[i]) != 0) {
            fprintf(stderr, "Failed to submit task %d\n", values[i]);
        }
    }

    pool_shutdown();
    return 0;
}
