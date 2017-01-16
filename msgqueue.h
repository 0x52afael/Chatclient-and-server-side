/*
* msgqueue.h - A generic queue in C
* Created by Olof Holmlund 2016-09-16
* c14ohd@cs.umu.se
*/

#include <stdlib.h>
#include <string.h>


/*Free function for more comlex datastructures.*/
typedef void (*freeFunction)(void *);

typedef struct queue_node {

	void *data;
	struct queue_node* next;

} Queue_item;

typedef struct msgQueue {

	int length;
	freeFunction freefunc;
	Queue_item *front;
	Queue_item *rear;

} msg_Queue;


/*Creates a new queue with the pointer to a freefunction (or NULL if simple data type)*/
void msgQueue_new(msg_Queue *queue, freeFunction f_Fn);
/*Destroys the queue by freeing every node and data item. Does not free the queue object itself.*/
void msgQueue_destroy(msg_Queue *queue);

/*Saves the data of the node in front of the queue in **data*/
void msgQueue_peek(msg_Queue *queue, void **data);
/*Just like peek() but does also remove the item from the queue*/
void msgQueue_pop(msg_Queue *queue, void **data);
/*Adds *element to the queue. Data_size is the size of the element*/
void msgQueue_add(msg_Queue *queue, void *element, int data_size);
/*Returns the number of nodes in the queue*/
int msgQueue_length(msg_Queue *queue);