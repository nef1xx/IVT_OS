#include <stdlib.h>
#include <string.h>

#include "task.h"
#include "list.h"
#include "cpu.h"

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

static struct node *pickNextNode()
{
    struct node *node = head;

    if (head != NULL) {
        head = head->next;
        node->next = NULL;
    }

    return node;
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
    struct node *node;
    int slice;

    while (head != NULL) {
        node = pickNextNode();
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
    }
}
