#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#include "float_vec.h"
#include "barrier.h"
#include "worker.h"
#include "utils.h"

int
diff(const void *a, const void *b)
{
    //float fa = *(const float*) a;
    //float fb = *(const float*) b;
    float epsilon = 0.00001;
    float diff = ((*(float*)a - *(float*)b));
    if (diff > 0 && diff < epsilon) {
	return 0;
    } else if (diff > 0){
	return 1;
    } else {
	return -1;
    }
}

void
qsort_floats(floats* xs)
{ 
    qsort(xs->data, xs->size, sizeof(float), diff);
}

floats*
sample(float* data, long size, int P)
{
    floats* xs = make_floats(0);
    int corP = 3*(P-1);
    for (int ii = 0; ii < corP; ii++) {
	int r = rand() % size;
	floats_push(xs, data[r]);
    }
    qsort_floats(xs);
    
    floats* ys = make_floats(0);
    floats_push(ys, 0);
    for (int jj = 1; jj < xs->size; jj+= 3) {
	floats_push(ys, xs->data[jj]);
    }
    floats_push(ys, FLT_MAX);
    free_floats(xs);
    return ys;
}

void
sort_worker(worker* ww)
{
    floats* xs = make_floats(0);
    // TODO: select the floats to be sorted by this worker

    float lower = ww->samps->data[ww->pnum];
    float upper = ww->samps->data[ww->pnum + 1];

    for (int ii = 0; ii < ww->size; ii++) {
	if (ww->data[ii] >= lower && ww->data[ii] < upper) {
	    floats_push(xs, ww->data[ii]);
	}
    }
    printf("%d: start %.04f, count %ld\n",ww->pnum, ww->samps->data[ww->pnum], xs->size);
   
    qsort_floats(xs);
    ww->sizes[ww->pnum] = xs->size;

    barrier_wait(ww->bb);
    
    long start = 0;
    if ((ww->pnum-1) == -1) {
	start = 0;
    } else {
	for (int i = 0; i <= ww->pnum-1; i++) {
	    start += ww->sizes[i];
	}
    }
    long end = start + ww->sizes[ww->pnum];
    int yy = 0;
    for (int xx = start; xx < end; xx++) {
	ww->data[xx] = xs->data[yy];
	yy++;
    }
    free_floats(xs);
}

void*
thread_main(void* thread_arg)
{
    worker* ww = ((worker*)thread_arg);
    sort_worker(ww);
    return thread_arg;
}

void
run_sort_workers(float* data, long size, long* sizes, int P, floats* samps, barrier* bb, int fd)
{
    pthread_t threads[P];
    int rv;
    for (int pp = 0; pp < P; ++pp) {
        worker* ww = make_worker();
        ww->fd = fd;
        ww->data = data;
        ww->size = size;
        ww->sizes = sizes;
        ww->P = P;
        ww->samps = samps;
        ww->bb = bb;
	ww->pnum = pp;
	rv = pthread_create(&(threads[pp]), 0, thread_main, (void*)(ww));
	check_rv(rv);
    }

    for (int ii = 0; ii < P; ++ii) {
	void* ret;
	rv = pthread_join(threads[ii], &ret);
	check_rv(rv);
	free_worker((worker*)(ret));
    }
}

void
sample_sort(long size, int P, long* sizes, barrier* bb, int fd, float* data)
{
    floats* samps = sample(data, size, P);
    run_sort_workers(data, size, sizes, P, samps, bb, fd);
    free_floats(samps);
}

int
main(int argc, char* argv[])
{
    if (argc != 4) {
        return 1;
    }

    const int P = atoi(argv[1]);
    if (P == 0) {
	exit(1);
    }
    const char* fname = argv[2];
    const char* fname_out = argv[3];

    seed_rng();

    int rv;
    struct stat st;
    rv = stat(fname, &st);
    check_rv(rv);

    const int fsize = st.st_size;
    if (fsize < 8) {
        printf("File too small.\n");
        return 1;
    }

    int fd = open(fname, O_RDONLY);
    check_rv(fd);
    long count = 0;
    read(fd, &count, 8);
    floats* xs = make_floats(count);
    read(fd, xs->data, (count*sizeof(float)));

    long* sizes = malloc(P*sizeof(long));

    barrier* bb = make_barrier(P);
    int fd_out = open(fname_out, O_CREAT | O_RDWR | O_TRUNC, 0666);
    check_rv(fd_out);
    sample_sort(count, P, sizes, bb, fd_out, xs->data);

    lseek(fd_out, 0, SEEK_SET);
    write(fd_out, &count, sizeof(long));

    lseek(fd_out, sizeof(long), SEEK_CUR);
    write(fd_out, xs->data, count * sizeof(float));
     
    rv = close(fd);
    assert(rv == 0);
    rv = close(fd_out);
    assert(rv == 0);
    free_barrier(bb);
    free_floats(xs);
    free(sizes);

    return 0;
}

