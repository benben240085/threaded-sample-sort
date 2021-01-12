#ifndef WORKER_H
#define WORKER_h

#include "barrier.h"
#include "float_vec.h"

typedef struct worker {
    int fd;
    int pnum;
    float* data;
    long size;
    long* sizes;
    int P;
    floats* samps;
    barrier* bb;
} worker;

worker* make_worker();
void free_worker(worker* ww);


#endif

