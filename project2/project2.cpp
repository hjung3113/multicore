#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <omp.h>
#include <cstring>
#include <atomic>
// #include <algorithm>

using namespace std;

#define threadh 32
#define MAXLEN 1000
#define CHARMAX 128
#define BILLION  1000000000L

int N; // sorted no

char** words;
char** temp;
int thread_num;

void msd();
void msd(int, int, int);
void insertion_sort(int lo, int hi);

// bool compare(char* a, char* b){
//     if(strcmp(a, b) < 0){
//         return true;
//     }else{
//         return false;
//     }
// }

int main(int argc, char* argv[])
{
    struct timespec start, stop;

    if (argc != 6) {
        printf("Usage: %s <infile> <sort item num> <start index> <print out num> <thread num>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    N = atoi(argv[2]);
    int startidx = atoi(argv[3]);
    int printn = atoi(argv[4]);
    thread_num = atoi(argv[5]);
    char* buf = (char *)malloc(MAXLEN);

    if (startidx + printn > N) printn = N-startidx;

    // if (thread_num > omp_get_num_threads()) thread_num = omp_get_num_threads();
    omp_set_num_threads(thread_num);

    words = (char **)malloc(sizeof(char *)*N);
    temp = (char **)malloc(sizeof(char *)*N);
    FILE* fin = fopen(argv[1], "r");

    for (int i=0; i<N; i++)
    {
        fscanf(fin, "%s ", buf);
        words[i] = (char*)malloc(sizeof(char)*(strlen(buf)+1));
        strcpy(words[i], buf);
    }
    free(buf);

    clock_gettime( CLOCK_REALTIME, &start);

    ///////////////////////////////////////////////////////
    //#pragma omp parallel
    //insertion_sort(0, N);
    msd();
    //sort(words, words+N, compare);
    
    ///////////////////////////////////////////////////////

    clock_gettime( CLOCK_REALTIME, &stop);

    for(int i = startidx; i < startidx+printn; i++){
        printf("%s\n", words[i]);
    }
    // print results
    cout << "Elapsed time: " << (stop.tv_sec - start.tv_sec) + ((double) (stop.tv_nsec - start.tv_nsec))/BILLION << " sec" << endl;
    // clean up and return
    for(int i=0;i<N;i++){
        free(words[i]);
    }
    free(words);
    free(temp);
    fclose(fin);

    return (EXIT_SUCCESS);
}

void msd() {
    msd(0, N, 0);
}

struct padding_int{
    atomic<int> val;
    char padding[60];
};

void insertion_sort(int lo, int hi)
{
    for(int i=lo+1; i<hi; i++){
        char* key = words[i];
        int j;
        for(j=i-1; j>=lo && strcmp(words[j], key)>0; j--){
            words[j+1] = words[j];
        }
        words[j+1] = key;
    }
}

void msd(int lo, int hi, int d) {
    padding_int count[CHARMAX];
    // int pcount[CHARMAX];
    
    if (hi <= lo) return;
    if (hi - lo < threadh) {
        insertion_sort(lo, hi);
        return;
    }

    memset((void*)&count, 0, sizeof(padding_int)*CHARMAX);
    
    #pragma omp parallel for
    for (int i = lo; i < hi; i++)
        count[words[i][d]].val++;

    // #pragma omp parallel shared(count, words, lo, hi) private(pcount)
    // {
    //     memset((void*)&pcount, 0, sizeof(int)*CHARMAX);
    //     #pragma omp parallel for schedule(static, (hi-lo)/thread_num)
    //     for (int i = lo; i < hi; i++)
    //         pcount[words[i][d]]++;
        
    //     #pragma omp critical
    //     {
    //         for (int i=0; i<CHARMAX; i++){
    //             count[i] += pcount[i];
    //         }
    //     }
    // }

    int tmp = 0;
    for (int k = 0; k < CHARMAX; k++){
        int tmp2 = tmp;
        tmp = count[k].val.load();
        count[k].val.store(tmp2);
        tmp += tmp2;
    }
    
    #pragma omp parallel for
    for (int i = lo; i < hi; i++)
        temp[lo + count[words[i][d]].val++] = words[i];

    #pragma omp parallel for
    for (int i = lo; i < hi; i++)
        words[i] = temp[i];

    #pragma omp parallel
    {
        #pragma omp for nowait schedule(dynamic)
        for (int i = 1; i < CHARMAX-1; i++){ // 0 -> string end
            if(count[i+1].val.load()-count[i].val.load() == 1) continue;
            msd(lo + count[i].val.load(), lo + count[i+1].val.load(), d+1);
        }
    }
}
