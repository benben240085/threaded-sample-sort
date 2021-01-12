/*typedef struct worker {
    float* data;
    long size;
    long* sizes;
    int P;
    floats* samps;
    barrier* bb;
} worker;*/

#include <stdlib.h>

#include "worker.h"

worker* make_worker()
{
    worker* ww = malloc(sizeof(worker));
    return ww;
}

void free_worker(worker* ww)
{
    free(ww);
}
