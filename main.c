#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "help.h"
#include "string.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in below.
 *
 * === User information ===
 * User 1: Juliusg13
 * SSN: 2801922799
 * User 2:
 * SSN:
 * === End User Information ===
 ********************************************************/


/* I have all the ringbuffer logic inside of chairs, front of the buffer, rear, queuesize to help me finding the right place and then the semaphores for locking,
 slots are empty chairs & items are customers */
struct chairs
{
    struct customer **customer; /* Array of customers */
    int max;
    /* TODO: Add more variables related to threads */
    /* Hint: Think of the consumer producer thread problem */
    int front;
    int rear;
    int queueSize;
    sem_t mutexRing;
    sem_t slotsRing;
    sem_t itemsRing;

};

struct barber
{
    int room;
    struct simulator *simulator;
};

struct simulator
{
    struct chairs chairs;
    
    pthread_t *barberThread;
    struct barber **barber;
};

/*
	The barber waits for a customer which then lockes the ringbuffer, the customes is chosen from the buffer and the queueSize is lowered so we wont have phantom seats.
	after we release  the buffer and waiting chair we cut the hair and then dismiss the customer.
*/
static void *barber_work(void *arg)
{
    struct barber *barber = arg;
    struct chairs *chairs = &barber->simulator->chairs;
    struct customer *customer = 0; //chairs->customer[0]; /* TODO: Fetch a customer from a chair */

    /* Main barber loop */
    while (true) {
	/* TODO: Here you must add you semaphores and locking logic */
        int x = sem_wait(&chairs->itemsRing);
        int y = sem_wait(&chairs->mutexRing);
	posix_wrapper(x, y);


        customer = chairs->customer[(++chairs->front)%(chairs->max)];
	chairs->queueSize--;

	thrlab_prepare_customer(customer, barber->room);

        x = sem_post(&chairs->mutexRing);
        y = sem_post(&chairs->slotsRing);
	posix_wrapper(x, y);

        thrlab_sleep(5 * (customer->hair_length - customer->hair_goal));

        thrlab_dismiss_customer(customer, barber->room);
	x = sem_post(&customer->mutex);
	posix_wrapper(x, x);
    }
    return NULL;
}

/**
 * Initialize data structures and create waiting barber threads.
 */
/*
	MutexRing is a binary lock but slots and items(chairs & customers) are both semaphore counters, my wrapper function is only simple and displays + exit if either pthread fails.
*/
static void setup(struct simulator *simulator)
{
    struct chairs *chairs = &simulator->chairs;
    /* Setup semaphores*/
    chairs->max = thrlab_get_num_chairs();
    
    /* Create chairs*/
    chairs->customer = malloc(sizeof(struct customer *) * thrlab_get_num_chairs());

    int x = sem_init(&chairs->mutexRing, 0, 1);
    int y = sem_init(&chairs->slotsRing, 0, thrlab_get_num_chairs());
    posix_wrapper(x, y);
    x = sem_init(&chairs->itemsRing, 0, 0);
    posix_wrapper(x, x);

    chairs->front = chairs->rear = 0;
    chairs->queueSize = 0;
    /* Create barber thread data */
    simulator->barberThread = malloc(sizeof(pthread_t) * thrlab_get_num_barbers());
    simulator->barber = malloc(sizeof(struct barber*) * thrlab_get_num_barbers());

    /* Start barber threads */
    struct barber *barber;
    for (unsigned int i = 0; i < thrlab_get_num_barbers(); i++) {
	barber = calloc(sizeof(struct barber), 1);
	barber->room = i;
	barber->simulator = simulator;
	simulator->barber[i] = barber;


	int x = (pthread_create(&simulator->barberThread[i], 0, barber_work, barber));
	int y = (pthread_detach(simulator->barberThread[i]));

	posix_wrapper(x, y);
    }
}

/**
 * Free all used resources and end the barber threads.
 */
static void cleanup(struct simulator *simulator)
{
    /* Free chairs */
    free(simulator->chairs.customer);

    /* Free barber thread data */
    free(simulator->barber);
    free(simulator->barberThread);
}

/**
 * Called in a new thread each time a customer has arrived.
 */

/*
	a customer arrives into the barbershop and is set as locked right away. if we have chair space then the buffer will be locked and customer will be set into the queue.
	once he has been accepted he will remain locked until the barber releases him once again. otherwise the customer will be sent packin', hopefully he "wont be back".
*/
static void customer_arrived(struct customer *customer, void *arg)
{
    struct simulator *simulator = arg;
    struct chairs *chairs = &simulator->chairs;
    int x = sem_init(&customer->mutex, 0, 0);
    int y = 0;
    /* TODO: Accept if there is an available chair */

    if (chairs->queueSize < chairs->max){ 
	y = sem_wait(&chairs->slotsRing);
	posix_wrapper(x, y);
        x = sem_wait(&chairs->mutexRing); // Lock ringbuffer

	chairs->customer[(++chairs->rear)%(chairs->max)] = customer;
	thrlab_accept_customer(customer);
	chairs->queueSize++;

        y = sem_post(&chairs->mutexRing); // Realease Ringbuffer
	posix_wrapper(x, y);
        x = sem_post(&chairs->itemsRing);
	y = sem_wait(&customer->mutex);
	posix_wrapper(x, y);

    } else {
    /* TODO: Reject if there are no available chairs */
	thrlab_reject_customer(customer);
	y = sem_post(&customer->mutex);
	posix_wrapper(x, y);
   }
}
/*
	if either pthread create or detach returns an error value then we will quit the program right away.
*/
void posix_wrapper(int x, int y)
{
	if(x != 0 || y != 0) {
		printf("Error occured");
		exit(0);
	}
}

int main (int argc, char **argv)
{
    struct simulator simulator;

    thrlab_setup(&argc, &argv);
    setup(&simulator);

    thrlab_wait_for_customers(customer_arrived, &simulator);

    thrlab_cleanup();
    cleanup(&simulator);

    return EXIT_SUCCESS;
}
