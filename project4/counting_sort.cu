#include <cuda.h>
#include <stdio.h>

#define nthread 256

__global__ void count_hist(int arr[], int hist[], int size)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if(i < size) atomicAdd(&hist[arr[i]], 1);
}

__global__ void arr_sort(int arr[], int hist[], int max_val)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	int j = threadIdx.x;
	__shared__ int s_idx[nthread];

	if(i < max_val){
		s_idx[j] = hist[i];
		
		__syncthreads();

		int s, cnt;
		if(i==0 && j==0) s=0;
		else if(i!=0 && j==0) s=hist[i-1];
		else s=s_idx[j-1];

		cnt = s_idx[j] - s;

		for(int idx = s; idx < s+cnt; idx++){
			arr[idx] = i;
		}
	}

}

__host__ void counting_sort(int arr[], int size, int max_val)
{
   // fill
	int* histogram;
	int* cuda_hist, *cuda_arr;
	histogram = (int *)calloc(sizeof(int), max_val);

	cudaMalloc((void**)&cuda_hist, max_val*sizeof(int));
	cudaMalloc((void**)&cuda_arr, size*sizeof(int));

	cudaMemcpy(cuda_hist, histogram, max_val*sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(cuda_arr, arr, size*sizeof(int), cudaMemcpyHostToDevice);

	count_hist <<< ceil((double)size / nthread), nthread >>> (cuda_arr, cuda_hist, size);


	cudaMemcpy(histogram, cuda_hist, max_val*sizeof(int), cudaMemcpyDeviceToHost);

//	cudaFree(cuda_hist);
//	cudaFree(cuda_arr);

	int sum = 0;
	for (int i=0; i<max_val; i++)
	{
		sum += histogram[i];
		histogram[i] = sum;
	}

	cudaMemcpy(cuda_hist, histogram, max_val*sizeof(int), cudaMemcpyHostToDevice);

	arr_sort <<< ceil((double)max_val / nthread), nthread  >>> (cuda_arr, cuda_hist, max_val);

	cudaMemcpy(arr, cuda_arr, size*sizeof(int), cudaMemcpyDeviceToHost);

	cudaFree(cuda_hist);
	cudaFree(cuda_arr);
	free(histogram);
}

