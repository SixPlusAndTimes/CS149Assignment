#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>

#include <driver_functions.h>

#include <thrust/scan.h>
#include <thrust/device_ptr.h>
#include <thrust/device_malloc.h>
#include <thrust/device_free.h>

#include "CycleTimer.h"

#define THREADS_PER_BLOCK 256
#define CHECK_CUDA(call) \
do { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
        fprintf(stderr, "CUDA 错误: %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

// 内核启动检查（必须加！）
#define CHECK_KERNEL() \
do { \
    cudaError_t err = cudaGetLastError(); \
    if (err != cudaSuccess) { \
        fprintf(stderr, "KERNEL 错误: %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
        exit(EXIT_FAILURE); \
    } \
    err = cudaDeviceSynchronize(); \
    if (err != cudaSuccess) { \
        fprintf(stderr, "KERNEL 崩溃: %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

// helper function to round an integer up to the next power of 2
static inline int nextPow2(int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}
__global__ void
upsweep_kernel(int N, int* inputArray, int stride)
{
    
    // int idx = (blockIdx.x * blockDim.x + threadIdx.x + 1) * stride - 1;
    long long idx = (blockIdx.x * blockDim.x + threadIdx.x + 1) * stride - 1; // avoid integer overflow
    inputArray[idx] += inputArray[idx - (stride >> 1)];
}

__global__ void
downsweep_kernel(int N, int* inputArray, int stride)
{
    // int idx = ( blockIdx.x * blockDim.x + threadIdx.x + 1) * stride - 1;
    long long idx = ( blockIdx.x * blockDim.x + threadIdx.x + 1) * stride - 1; // avoid integer overflow
    int tmp = inputArray[idx - (stride >> 1)];
    inputArray[idx - (stride >> 1)] = inputArray[idx];
    inputArray[idx] += tmp;
}
// exclusive_scan --
//
// Implementation of an exclusive scan on global memory array `input`,
// with results placed in global memory `result`.
//
// N is the logical size of the input and output arrays, however
// students can assume that both the start and result arrays we
// allocated with next power-of-two sizes as described by the comments
// in cudaScan().  This is helpful, since your parallel scan
// will likely write to memory locations beyond N, but of course not
// greater than N rounded up to the next power of 2.
//
// Also, as per the comments in cudaScan(), you can implement an
// "in-place" scan, since the timing harness makes a copy of input and
// places it in result
void exclusive_scan(int* input, int roundedLength, int* result)
{
    // printf("max int = %d\n", __INT_MAX__);
    // int roundedLength = nextPow2(N);
    // printf("N = %d, roudedLength = %d\n", N, roundedLength);
    // int * temp = new int[roundedLength];
    // cudaMemcpy(temp, input, roundedLength * sizeof(int), cudaMemcpyDeviceToHost);
    // printf("exclusive_scan begin:\n");
    // for (int i = 0; i < roundedLength; ++i)
    // {
    //     printf("%2d ", temp[i]);
    // }
    // printf("\n");

    // upsweep phase
    for (int two_d = 1; two_d < roundedLength/2; two_d*=2) {
        int num_threads = roundedLength / (2 * two_d);
        int blocks = (num_threads - 1 + THREADS_PER_BLOCK) / THREADS_PER_BLOCK;
        if (num_threads <= THREADS_PER_BLOCK)
        {
            upsweep_kernel<<<1, num_threads>>>(roundedLength, input, 2 * two_d);
        }
        else 
        {
            upsweep_kernel<<<blocks, THREADS_PER_BLOCK>>>(roundedLength, input, 2 * two_d);
        }
        CHECK_KERNEL();
        // cudaDeviceSynchronize(); 

        // cudaMemcpy(temp, input, roundedLength * sizeof(int), cudaMemcpyDeviceToHost);
        // printf("upsweep phase, two_d %d two_dplus1 %d, num_threads %d blocks %d\n", two_d, two_dplus1, num_threads, blocks);
        // for (int i = 0; i < roundedLength; ++i)
        // {
        //     printf("%2d ", temp[i]);
        // }
        // printf("\n");

    }
    // printf("upsweep phase done\n");
    cudaMemset(input + roundedLength - 1, 0, sizeof(int));
    // downsweep phase
    for (int two_d = roundedLength/2, thread_num = 1; two_d >= 1; two_d /= 2, thread_num *= 2) {
        int blocks = (thread_num - 1 + THREADS_PER_BLOCK) / THREADS_PER_BLOCK;
        // printf("downsweep phase, two_d %d, two_dplus1 %d, numthread %d, blocks %d\n", two_d, two_dplus1, thread_num, blocks);
        if (thread_num <= THREADS_PER_BLOCK)
        {
            downsweep_kernel<<<1, thread_num>>>(roundedLength, input, 2 * two_d);
        }
        else 
        {
            downsweep_kernel<<<blocks, THREADS_PER_BLOCK>>>(roundedLength, input, 2 * two_d);
        }
        CHECK_KERNEL();
        // cudaDeviceSynchronize(); 

        // cudaMemcpy(temp, input, roundedLength * sizeof(int), cudaMemcpyDeviceToHost);
        // printf("downsweep phase, two_d %d, two_dplus1 %d, numthread %d, blocks %d\n", two_d, two_dplus1, thread_num, blocks);
        // for (int i = 0; i < roundedLength; ++i)
        // {
        //     printf("%2d ", temp[i]);
        // }
        // printf("\n");
    }

}


//
// cudaScan --
//
// This function is a timing wrapper around the student's
// implementation of scan - it copies the input to the GPU
// and times the invocation of the exclusive_scan() function
// above. Students should not modify it.
double cudaScan(int* inarray, int* end, int* resultarray)
{
    int* device_result;
    int* device_input;
    int N = end - inarray;  

    // This code rounds the arrays provided to exclusive_scan up
    // to a power of 2, but elements after the end of the original
    // input are left uninitialized and not checked for correctness.
    //
    // Student implementations of exclusive_scan may assume an array's
    // allocated length is a power of 2 for simplicity. This will
    // result in extra work on non-power-of-2 inputs, but it's worth
    // the simplicity of a power of two only solution.

    int rounded_length = nextPow2(end - inarray);
    
    // cudaMalloc((void **)&device_result, sizeof(int) * rounded_length);
    cudaMalloc((void **)&device_input, sizeof(int) * rounded_length);

    // For convenience, both the input and output vectors on the
    // device are initialized to the input values. This means that
    // students are free to implement an in-place scan on the result
    // vector if desired.  If you do this, you will need to keep this
    // in mind when calling exclusive_scan from find_repeats.
    cudaMemcpy(device_input, inarray, (end - inarray) * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemset(device_input + N, 0 , (rounded_length - N) * sizeof(int));
    // cudaMemcpy(device_result, inarray, (end - inarray) * sizeof(int), cudaMemcpyHostToDevice);

    double startTime = CycleTimer::currentSeconds();

    exclusive_scan(device_input, rounded_length, nullptr);

    // Wait for completion
    cudaDeviceSynchronize();
    double endTime = CycleTimer::currentSeconds();
       
    cudaMemcpy(resultarray, device_input, (end - inarray) * sizeof(int), cudaMemcpyDeviceToHost);

    double overallDuration = endTime - startTime;
    return overallDuration; 
}


// cudaScanThrust --
//
// Wrapper around the Thrust library's exclusive scan function
// As above in cudaScan(), this function copies the input to the GPU
// and times only the execution of the scan itself.
//
// Students are not expected to produce implementations that achieve
// performance that is competition to the Thrust version, but it is fun to try.
double cudaScanThrust(int* inarray, int* end, int* resultarray) {

    int length = end - inarray;
    thrust::device_ptr<int> d_input = thrust::device_malloc<int>(length);
    thrust::device_ptr<int> d_output = thrust::device_malloc<int>(length);
    
    cudaMemcpy(d_input.get(), inarray, length * sizeof(int), cudaMemcpyHostToDevice);

    double startTime = CycleTimer::currentSeconds();

    thrust::exclusive_scan(d_input, d_input + length, d_output);

    cudaDeviceSynchronize();
    double endTime = CycleTimer::currentSeconds();
   
    cudaMemcpy(resultarray, d_output.get(), length * sizeof(int), cudaMemcpyDeviceToHost);

    thrust::device_free(d_input);
    thrust::device_free(d_output);

    double overallDuration = endTime - startTime;
    return overallDuration; 
}

__global__ void 
fill_repeat_flags(int* input, int* outflags)
{
    long long idx = ( blockIdx.x * blockDim.x + threadIdx.x ); // avoid integer overflow
    outflags[idx] = (input[idx] == input[idx + 1]);
}

__global__ void 
scatter_flags(int* scan, int* flags, int* output)
{
    long long idx = ( blockIdx.x * blockDim.x + threadIdx.x ); // avoid integer overflow
    if (flags[idx])
    {
        output[scan[idx]] = idx;
    }
}

// find_repeats --
//
// Given an array of integers `device_input`, returns an array of all
// indices `i` for which `device_input[i] == device_input[i+1]`.
//
// Returns the total number of pairs found
int find_repeats(int* device_input, int length, int* device_output) {

    // CS149 TODO:
    //
    // Implement this function. You will probably want to
    // make use of one or more calls to exclusive_scan(), as well as
    // additional CUDA kernel launches.
    //    
    // Note: As in the scan code, the calling code ensures that
    // allocated arrays are a power of 2 in size, so you can use your
    // exclusive_scan function with them. However, your implementation
    // must ensure that the results of find_repeats are correct given
    // the actual array length.

    int rounded_length = nextPow2(length);

    // int* tmp = new int[rounded_length];
    // cudaMemcpy(tmp, device_input, rounded_length * sizeof(int), cudaMemcpyDeviceToHost);
    // printf("find repeats start\n");
    // for (int i = 0; i < rounded_length; ++i)
    // {
    //     printf("%2d ", tmp[i]);
    // }
    // printf("\n");

    int *flags;
    cudaMalloc((void **)&flags, rounded_length * sizeof(int));
    int thread_num = rounded_length - 1;
    int blocks = (thread_num - 1 + THREADS_PER_BLOCK) / THREADS_PER_BLOCK;
    if (thread_num < THREADS_PER_BLOCK) 
    {
        fill_repeat_flags<<<1, thread_num>>>(device_input, flags);
    }
    else 
    {
        fill_repeat_flags<<<blocks, THREADS_PER_BLOCK>>>(device_input, flags);
    }
    CHECK_KERNEL();
    cudaMemset(flags + rounded_length - 1, 0, sizeof(int));

    {
        // cudaMemcpy(tmp, flags, rounded_length * sizeof(int), cudaMemcpyDeviceToHost);
        // printf("fill_repeat_flags done\n");
        // for (int i = 0; i < rounded_length; ++i)
        // {
        //     printf("%2d ", tmp[i]);
        // }
        // printf("\n");
    }
    
    int* scans;
    cudaMalloc((void **)&scans, rounded_length * sizeof(int));
    cudaMemcpy(scans, flags, rounded_length * sizeof(int), cudaMemcpyDeviceToDevice);
    exclusive_scan(scans, rounded_length, nullptr);

    {
        // cudaMemcpy(tmp, scans, rounded_length * sizeof(int), cudaMemcpyDeviceToHost);
        // printf("exclusive_scan done\n");
        // for (int i = 0; i < rounded_length; ++i)
        // {
        //     printf("%2d ", tmp[i]);
        // }
        // printf("\n");
    }

    
    if (thread_num < THREADS_PER_BLOCK) 
    {
        scatter_flags<<<1, thread_num>>>(scans, flags, device_output);
    }
    else 
    {
        scatter_flags<<<blocks, THREADS_PER_BLOCK>>>(scans, flags, device_output);
    }

    {
        // cudaMemcpy(tmp, device_output, rounded_length * sizeof(int), cudaMemcpyDeviceToHost);
        // printf("scatter_flags done\n");
        // for (int i = 0; i < rounded_length; ++i)
        // {
        //     printf("%2d ", tmp[i]);
        // }
        // printf("\n");
    }
    int num_repeats;
    cudaMemcpy(&num_repeats, &scans[length - 1], sizeof(int), cudaMemcpyDeviceToHost);
    // printf("repeat num is %d\n", num_repeats);
    return num_repeats; 
}


//
// cudaFindRepeats --
//
// Timing wrapper around find_repeats. You should not modify this function.
double cudaFindRepeats(int *input, int length, int *output, int *output_length) {

    int *device_input;
    int *device_output;
    int rounded_length = nextPow2(length);
    
    cudaMalloc((void **)&device_input, rounded_length * sizeof(int));
    cudaMalloc((void **)&device_output, rounded_length * sizeof(int));
    cudaMemcpy(device_input, input, length * sizeof(int), cudaMemcpyHostToDevice);

    cudaDeviceSynchronize();
    double startTime = CycleTimer::currentSeconds();
    
    int result = find_repeats(device_input, length, device_output);

    cudaDeviceSynchronize();
    double endTime = CycleTimer::currentSeconds();

    // set output count and results array
    *output_length = result;
    cudaMemcpy(output, device_output, length * sizeof(int), cudaMemcpyDeviceToHost);

    cudaFree(device_input);
    cudaFree(device_output);

    float duration = endTime - startTime; 
    return duration;
}



void printCudaInfo()
{
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);

    printf("---------------------------------------------------------\n");
    printf("Found %d CUDA devices\n", deviceCount);

    for (int i=0; i<deviceCount; i++)
    {
        cudaDeviceProp deviceProps;
        cudaGetDeviceProperties(&deviceProps, i);
        printf("Device %d: %s\n", i, deviceProps.name);
        printf("   SMs:        %d\n", deviceProps.multiProcessorCount);
        printf("   Global mem: %.0f MB\n",
               static_cast<float>(deviceProps.totalGlobalMem) / (1024 * 1024));
        printf("   CUDA Cap:   %d.%d\n", deviceProps.major, deviceProps.minor);
    }
    printf("---------------------------------------------------------\n"); 
}
