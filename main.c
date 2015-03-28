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
 * User 1: 
 * SSN:
 * User 2:
 * SSN:
 * === End User Information ===
 ********************************************************/

typedef struct {
	int* buf;
	int n;
	int front;
	int rear;
	sem_t mutex;
	sem_t slots;
	sem_t items;
} ringbuf;

struct chairs
{
    struct customer **customer; /* Array of customers */
    int max;
    /* TODO: Add more variables related to threads */
    /* Hint: Think of the consumer producer thread problem */
    int front;
    int rear;
//    int freeChairs;
    int queueSize;
    sem_t mutexRing;
    sem_t slotsRing;
    sem_t itemsRing;

    sem_t mutex;
    sem_t chair;
    sem_t barber;
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

static int freeChairs;

/*void ringbuf_init(ringbuf *sp)
{
	int max_chairs = thrlab_get_num_chairs();
	void* dest = sp->buf;
	struct customer **customer;
	void* source = customer;

	memcpy(dest, source, sizeof(customer));
	sp->n = max_chairs;
	sp->front = sp->rear = 0;
	sem_init(&sp->mutex, 0, 1);
        sem_init(&sp->mutex, 0, max_chairs);
        sem_init(&sp->mutex, 0, 0);
}

void ringbuf_deinit(ringbuf *sp)
{
	free(sp->buf);
}

void ringbuf_insert(ringbuf *sp, void *item)
{
	sem_wait(&sp->slots);
        sem_wait(&sp->mutex);

	sp->buf[(++sp->rear)%(sp->n)] = (void *)item;
        sem_post(&sp->mutex);
        sem_post(&sp->slots);
}
*/

/*customer ringbuf_remove(struct chairs *sp)
{
	customer item;
        sem_wait(&sp->items);
        sem_wait(&sp->mutex);

	item = sp->buf[(++sp->front)%(sp->n)];

        sem_post(&sp->mutex);
        sem_post(&sp->slots);

	return item;
}
*/
static void *barber_work(void *arg)
{
    struct barber *barber = arg;
    struct chairs *chairs = &barber->simulator->chairs;
    struct customer *customer = chairs->customer[0]; /* TODO: Fetch a customer from a chair */

    /* Main barber loop */
    while (true) {
	sem_wait(&chairs->barber);
        sem_wait(&chairs->mutex);

	/* TODO: Here you must add you semaphores and locking logic */

        sem_wait(&chairs->itemsRing);
        sem_wait(&chairs->mutexRing);

        customer = chairs->customer[(++chairs->front)%(chairs->max)];

        sem_post(&chairs->mutexRing);
        sem_post(&chairs->slotsRing);




//        sem_wait(&chairs->barber);
//        sem_wait(&chairs->mutex);

//------------------------------------------------
//	chairs->freeChairs++;
	freeChairs++;
	chairs->queueSize--;
//	customer = chairs->customer[0]; /* TODO: You must choose the customer */
	sem_post(&chairs->chair);
	thrlab_prepare_customer(customer, barber->room);
//	sem_post(&chairs->chair);
	sem_post(&chairs->mutex);
//	sem_post(&chairs->chair);

        thrlab_sleep(5 * (customer->hair_length - customer->hair_goal));
        thrlab_dismiss_customer(customer, barber->room);

	sem_post(&customer->mutex);
    }
    return NULL;
}

/**
 * Initialize data structures and create waiting barber threads.
 */
static void setup(struct simulator *simulator)
{
    struct chairs *chairs = &simulator->chairs;
    /* Setup semaphores*/
    chairs->max = thrlab_get_num_chairs();
    
    /* Create chairs*/
    chairs->customer = malloc(sizeof(struct customer *) * thrlab_get_num_chairs());

    sem_init(&chairs->mutex, 0, 1);
    sem_init(&chairs->chair, 0, 1);
    sem_init(&chairs->barber, 0, 0);

    sem_init(&chairs->mutexRing, 0, 1);
    sem_init(&chairs->slotsRing, 0, thrlab_get_num_chairs());
    sem_init(&chairs->itemsRing, 0, 0);

    chairs->front = chairs->rear = 0;
//    chairs->freeChairs = chairs->max;
    freeChairs = thrlab_get_num_chairs();
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
	pthread_create(&simulator->barberThread[i], 0, barber_work, barber);
	pthread_detach(simulator->barberThread[i]);

//	sem_post(&customer->mutex);
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
static void customer_arrived(struct customer *customer, void *arg)
{
    struct simulator *simulator = arg;
    struct chairs *chairs = &simulator->chairs;

    sem_init(&customer->mutex, 0, 0);

    /* TODO: Accept if there is an available chair */

    //if found
//    if(chairs->freeChairs > 0) {
    if(freeChairs > 0) {
	freeChairs--;
	sem_wait(&chairs->chair);

	sem_wait(&chairs->slotsRing);
        sem_wait(&chairs->mutexRing);

        chairs->customer[(++chairs->rear)%(chairs->max)] = customer;
	thrlab_accept_customer(customer);

        sem_post(&chairs->mutexRing);
        sem_post(&chairs->itemsRing);

	chairs->queueSize++;
//        chairs->freeChairs = (chairs->freeChairs - 1);



//	sem_wait(&chairs->mutex);
//	sem_wait(&chairs->chair);

//	thrlab_accept_customer(customer);
//	chairs->customer[0] = customer;

//	sem_post(&chairs->mutex);


	sem_post(&chairs->barber);
	sem_wait(&customer->mutex);
    } else {
    /* TODO: Reject if there are no available chairs */
	thrlab_reject_customer(customer);
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
