#include <stdlib.h>
#include <string.h>

#include "task.h"
#include "list.h"
#include "cpu.h"

static struct node *head = NULL;
static int nextTid = 1;

static Task *pickNextTask()
{
    if (head == NULL) {
        return NULL;
    }

    return head->task;
}

static void appendTask(Task *task)
{
    struct node *newNode = malloc(sizeof(struct node));
    struct node *temp;

    newNode->task = task;
    newNode->next = NULL;

    if (head == NULL) {
        head = newNode;
        return;
    }

    temp = head;
    while (temp->next != NULL) {
        temp = temp->next;
    }

    temp->next = newNode;
}

static void removeFirstTask()
{
    struct node *temp = head;

    if (head == NULL) {
        return;
    }

    head = head->next;
    free(temp->task->name);
    free(temp->task);
    free(temp);
}

void add(char *name, int priority, int burst)
{
    Task *task = malloc(sizeof(Task));

    task->name = strdup(name);
    task->tid = nextTid++;
    task->priority = priority;
    task->burst = burst;

    appendTask(task);
}

void schedule()
{
    Task *task;

    while (head != NULL) {
        task = pickNextTask();
        run(task, task->burst);
        removeFirstTask();
    }
}
