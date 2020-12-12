#include <stdlib.h>
#include <stdio.h>
#include <cuda_runtime.h>
#include <algorithm>
#include <cstring>
#include <time.h>

#define N 100000
#define MAX_VAL 10000000

using namespace std;

extern void counting_sort(int arr[], int, int);

int main()
{
	clock_t start, end;
  int *array = (int *)malloc(sizeof(int)*N);
  int *array2 = (int *)malloc(sizeof(int)*N);

  for(int i=0;i<N;i++){
      array[i] = rand()%MAX_VAL;
  }
  memcpy(array2, array, sizeof(int)*N);
	start = clock();
  counting_sort(array, N, MAX_VAL);
  end = clock();
  printf("parallel : %f\n", (double)(end-start));
	start = clock();
  sort(array2, array2+N);
  end = clock();
  printf("qsort : %f\n", (double)(end-start));

  for(int i=0;i<N;i++){
	  if(array[i] != array2[i]) {
		  printf("not eq!\n");
		  break;
	  }
  }

  for(int i=0;i<N-1;i++){
      if( array[i] > array[i+1]){
          printf("Not sorted\n");
		  free(array);
		  free(array2);
          exit(1);
      }
  }
  printf("Sorted\n");
  free(array);
  free(array2);
}
