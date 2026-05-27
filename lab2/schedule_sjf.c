#include <stdlib.h>
#include <string.h>

#include "task.h"
#include "list.h"
#include "cpu.h"

static struct node *head = NULL;
static int nextTid = 1;

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

static Task *pickNextTask()
{
    struct node *temp = head;
    Task *best = NULL;

    while (temp != NULL) {
        if (best == NULL || temp->task->burst < best->burst) {
            best = temp->task;
        }
        temp = temp->next;
    }

    return best;
}

static void removeTask(Task *task)
{
    struct node *temp = head;
    struct node *prev = NULL;

    while (temp != NULL && temp->task != task) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) {
        return;
    }

    if (prev == NULL) {
        head = temp->next;
    } else {
        prev->next = temp->next;
    }

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
        removeTask(task);
    }
}
