// Author: Nat Tuck
// CS3650 starter code

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "barrier.h"

barrier*
make_barrier(int nn)
{
    barrier* bb = malloc(sizeof(barrier));
    assert(bb != 0);
    pthread_mutex_init(&(bb->lock), 0);
    pthread_cond_init(&(bb->cond), 0);
    bb->count = nn; 
    bb->seen  = 0;
    return bb;
}

void
barrier_wait(barrier* bb)
{
    while (1) {
        sleep(1);
	pthread_mutex_lock(&(bb->lock));
	bb->seen += 1;
	if(bb->seen >= bb->count) {
	    pthread_cond_broadcast(&(bb->cond));
	}

	while (bb->seen < bb->count) {
	    pthread_cond_wait(&(bb->cond), &(bb->lock));
	}
   	pthread_mutex_unlock(&(bb->lock));
	break;
    }
}

void
free_barrier(barrier* bb)
{
    free(bb);
}

