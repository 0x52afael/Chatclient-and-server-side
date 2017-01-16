/*
* msgqueue.c - A generic queue in C
* Created by Olof Holmlund 2016-09-16
* c14ohd@cs.umu.se
*/


#include "msgqueue.h"
#include <stdio.h>



/*
* Iterates through a queue and frees every item in it, the queue-structure itself is not freed. 
* If the data in the items is more complex than a single element, a freefunction must
* be given as an argument during msgQueue_new
*/
void msgQueue_destroy(msg_Queue *queue)
{
	Queue_item *tmp;

	if (queue == NULL)
		return;

	while(queue->front != NULL)
	{
		tmp = queue->front;
		queue->front = tmp->next;

		if(queue->freefunc) 
		{
			queue->freefunc(tmp->data);
		}

		free(tmp->data);
		free(tmp);
	}

	queue->length = 0;
}

/*
* Saves the data in the front of the queue in *data
* If the queue is empty data will point to a NULL-pointer.
* Peek does not remove the node containing the data from the queue.
*/
void msgQueue_peek(msg_Queue *queue, void **data)
{
	if(queue == NULL)
	{
		data = NULL;
		return;
	}

	if(queue->length == 0)
	{
		data = NULL;
		return;
	}

	//*data = malloc(queue->front->data_size);
	//memcpy(*data, queue->front->data, queue->front->data_size);

	*data = queue->front->data;
}



/*
* msgQueue_pop takes the data in the front of the queue and saves it in **data.
* The data is then taken out of the queue unlike peek() where it remains. 
*/
void msgQueue_pop(msg_Queue *queue, void **data)
{
	Queue_item *tmp;

	if(queue == NULL)
	{
		data = NULL;
		return;
	}

	if(queue->length == 0)
	{
		data = NULL;
		return;
	}

	tmp = queue->front;

	*data = tmp->data;
	queue->front = tmp->next;

	queue->length--;

	free(tmp);
}

/*
* Adds a node to the queue containing element. Function uses memcpy
* to copy from element pointer in to the queue node. 
*/
void msgQueue_add(msg_Queue *queue, void *element, int element_size)
{
	if(queue == NULL)
		return;

	Queue_item *itm = malloc(sizeof(Queue_item));
	itm->data = malloc(element_size);

	memcpy(itm->data, element, element_size);

	if(queue->length == 0)
	{
		itm->next = NULL;
		queue->front = itm;
		queue->rear = itm;

	} else
	{
		itm->next = NULL;
		queue->rear->next = itm;
		queue->rear = itm;
	}

	queue->length++;
}

/*
* Creates a new msgQueue. If the data that will be stored has nestled objects
* it will require a freefunc to be correctly freed. If the data is just single object
* no freefunc is required and NULL can be given as argument 2. 
*/
void msgQueue_new(msg_Queue *queue, freeFunction freefunc) 
{
	queue->length = 0;
	queue->front = NULL;
	queue->rear = NULL;
	queue->freefunc = freefunc;
}

/*
* Returns the length of the queue in nr of elements.
* Returns -1 on error
*/
int msgQueue_length(msg_Queue *queue)
{
	return (queue == NULL) ? -1 : queue->length;
}


/*int main(void)
{
	msg_Queue *q = malloc(sizeof(msg_Queue));
	msgQueue_new(q, NULL);
	printf("Size: %d\n", msgQueue_length(q));

	char *str = "HELLO WORLD!";
	for(int i = 0; i<500; i++)
	{
		msgQueue_add(q, str, strlen(str)+1);
	}

	for(int i = 0; i<50; i++)
	{
		void *data = NULL;
		msgQueue_peek(q, &data);
		msgQueue_pop(q, &data);
		printf("Poped queue, size: %d, data: %s\n", msgQueue_length(q), data);
		free(data);
	}

	for(int i = 0; i<20; i++)
	{
		msgQueue_add(q, str, strlen(str)+1);
	}
	printf("size: %d\n", msgQueue_length(q));


	msgQueue_destroy(q);
	printf("queue destroyed, size: %d\n", msgQueue_length(q));
	free(q);


	return 0;
}*/

