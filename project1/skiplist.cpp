/*
 * main.cpp
 *
 * Serial version
 *
 * Compile with -O2
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include "skiplist.h"

using namespace std;

// aggregate variables
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
bool done = false;

struct task_node
{
    char cmd;
    long num;
};

#define QSIZE 10000

struct
{
    task_node q[QSIZE];
    int put, get;
    pthread_mutex_t qlock;   
}task_queue;

int task_req, task_sol;

skiplist<int, int> list(0,1000000);

pthread_cond_t nonempty = PTHREAD_COND_INITIALIZER;
pthread_cond_t nonfull = PTHREAD_COND_INITIALIZER;
pthread_cond_t ptask_done = PTHREAD_COND_INITIALIZER;
pthread_spinlock_t splock;

bool all_task_done = false;
bool print_done = true;

pthread_barrier_t pbarrier;

// function prototypes
bool isfull();
bool isempty();
void push(task_node a);
void wait_for_queue_empty_print();
task_node pop();

bool isfull(){
    return ((task_queue.put+1)%QSIZE == task_queue.get);
}

bool isempty(){
    return (task_queue.put == task_queue.get);
}

bool task_done(){
    return (task_req == task_sol);
}

void push(task_node a)
{
    pthread_mutex_lock(&task_queue.qlock);
    while(isfull()) pthread_cond_wait(&nonfull, &task_queue.qlock);
    task_queue.q[task_queue.put] = a;
    task_queue.put = (task_queue.put + 1)%QSIZE;
    pthread_cond_signal(&nonempty);
    pthread_mutex_unlock(&task_queue.qlock);
}

task_node pop()
{
    task_node a;
    pthread_mutex_lock(&task_queue.qlock);
    while(isempty() && !all_task_done) pthread_cond_wait(&nonempty, &task_queue.qlock);
    if(all_task_done){
        a.cmd = 'e';
        pthread_mutex_unlock(&task_queue.qlock);
        return a;
    }
    a.cmd=task_queue.q[task_queue.get].cmd;
    a.num=task_queue.q[task_queue.get].num;
    task_queue.get = (task_queue.get+1)%QSIZE;
    pthread_cond_signal(&nonfull);
    pthread_mutex_unlock(&task_queue.qlock);
    return a;
}

void wait_for_queue_empty_print()
{
    pthread_mutex_lock(&task_queue.qlock);
    while(!task_done()) pthread_cond_wait(&ptask_done, &task_queue.qlock);
    cout << list.printList() << endl;
    pthread_mutex_unlock(&task_queue.qlock);
}

void* thread_main(void* arg)
{
    pthread_t tid = *(pthread_t*)arg;
    while(1)
    {
        task_node tmp = pop();
        char action = tmp.cmd;
        int num = tmp.num;

        if (action == 'i') {            // insert
            list.insert(num,num);
            // update aggregate variables
            sum += num;
            if (num % 2 == 1) {
                odd++;
            }
        } else if (action == 'q') {      // qeury
            if(list.find(num)!=num)
		    std::cout << "ERROR: Not Found: " << num << endl;
        } else if (action == 'w') {     // wait
            usleep(num);
        } else if (action == 'e') {
            return NULL;
        } else {
            printf("ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }
        // printf("thread %d done job : action : %c, num :%d\n", tid%10, action, num);

        pthread_spin_lock(&splock);
        task_sol++;
        if(task_done()) pthread_cond_signal(&ptask_done);
        pthread_spin_unlock(&splock);
    }
}

int main(int argc, char* argv[])
{
    struct timespec start, stop;

    // check and parse command line options
    if (argc != 3) {
        printf("Usage: %s <infile> <nthread>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *fn = argv[1];
    int nthread = atoi(argv[2]);
    pthread_t tid[nthread];

    clock_gettime( CLOCK_REALTIME, &start);

    // load input file
    FILE* fin = fopen(fn, "r");
    char action;
    long num;

    pthread_mutex_init(&task_queue.qlock, NULL);
    pthread_spin_init(&splock, PTHREAD_PROCESS_SHARED);

    pthread_barrier_init(&pbarrier, NULL, nthread+1);

    for(int i=0; i<nthread; i++)
    {
        if(pthread_create(&tid[i], NULL, thread_main, (void*)(&tid[i]))){
            fprintf(stderr, "fail : pthread_create, errno : %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }

    while (fscanf(fin, "%c %ld\n", &action, &num) > 0) {
        // printf("enqueue! action : %c, num : %ld\n", action, num);
        if (action == 'p') {     // wait
            wait_for_queue_empty_print();
            continue;
        }
        task_req++;
        task_node tmp;
        tmp.cmd = action;
        tmp.num = num;     
        push(tmp);
    }

    pthread_mutex_lock(&task_queue.qlock);
    // pthread_cond_broadcast(&nonempty);
    while(!isempty()) pthread_cond_wait(&ptask_done, &task_queue.qlock);
    
    all_task_done = true;
    pthread_cond_broadcast(&nonempty);

    pthread_mutex_unlock(&task_queue.qlock);

    // if(all_task_done) for(int i=0; i<nthread; i++) pthread_cancel(tid[i]);

    for(int i=0; i<nthread; i++) pthread_join(tid[i], NULL);

    pthread_spin_destroy(&splock);
    pthread_mutex_destroy(&task_queue.qlock);
    pthread_barrier_destroy(&pbarrier);
    fclose(fin);
    clock_gettime( CLOCK_REALTIME, &stop);
    printf("sum : %ld\n", sum);

    // print results
    cout << "Elapsed time: " << (stop.tv_sec - start.tv_sec) + ((double) (stop.tv_nsec - start.tv_nsec))/BILLION << " sec" << endl;

    // clean up and return
    return (EXIT_SUCCESS);

}
