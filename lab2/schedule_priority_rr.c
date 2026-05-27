#include <stdlib.h>
#include <string.h>

#include "task.h"
#include "list.h"
#include "cpu.h"
#include "schedulers.h"

static struct node *head = NULL;
static int nextTid = 1;

static void appendNode(struct node *node)
{
    struct node *temp;

    node->next = NULL;

    if (head == NULL) {
        head = node;
        return;
    }

    temp = head;
    while (temp->next != NULL) {
        temp = temp->next;
    }

    temp->next = node;
}

static void appendTask(Task *task)
{
    struct node *newNode = malloc(sizeof(struct node));

    newNode->task = task;
    newNode->next = NULL;
    appendNode(newNode);
}

static int highestPriority()
{
    struct node *temp = head;
    int priority = MIN_PRIORITY - 1;

    while (temp != NULL) {
        if (temp->task->priority > priority) {
            priority = temp->task->priority;
        }
        temp = temp->next;
    }

    return priority;
}

static Task *pickNextTask(int priority)
{
    struct node *temp = head;

    while (temp != NULL) {
        if (temp->task->priority == priority) {
            return temp->task;
        }
        temp = temp->next;
    }

    return NULL;
}

static struct node *detachTask(Task *task)
{
    struct node *temp = head;
    struct node *prev = NULL;

    while (temp != NULL && temp->task != task) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) {
        return NULL;
    }

    if (prev == NULL) {
        head = temp->next;
    } else {
        prev->next = temp->next;
    }

    temp->next = NULL;
    return temp;
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
    struct node *node;
    int priority;
    int slice;

    while (head != NULL) {
        priority = highestPriority();
        task = pickNextTask(priority);

        while (task != NULL) {
            node = detachTask(task);
            slice = node->task->burst < QUANTUM ? node->task->burst : QUANTUM;
            run(node->task, slice);
            node->task->burst -= slice;

            if (node->task->burst > 0) {
                appendNode(node);
            } else {
                free(node->task->name);
                free(node->task);
                free(node);
            }

            task = pickNextTask(priority);
        }
    }
}
