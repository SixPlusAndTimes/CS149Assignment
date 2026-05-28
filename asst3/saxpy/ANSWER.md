
ISPC results in asst1:
~~~md
[saxpy serial]:         [76.086] ms     [19.585] GB/s   [2.629] GFLOPS
[saxpy ispc]:           [75.910] ms     [19.630] GB/s   [2.635] GFLOPS
[saxpy task ispc]:      [57.970] ms     [25.705] GB/s   [3.450] GFLOPS
~~~

cuda results in asst3:
~~~md
---------------------------------------------------------
Found 1 CUDA devices
Device 0: NVIDIA GeForce RTX 3050 Laptop GPU
   SMs:        16
   Global mem: 4096 MB
   CUDA Cap:   8.6
---------------------------------------------------------
Running 3 timing tests:
Effective BW by CUDA saxpy: 233.917 ms          [4.778 GB/s]
Effective BW of kernel calc: 10.398 ms          [107.480 GB/s]
Effective BW by CUDA saxpy: 173.751 ms          [6.432 GB/s]
Effective BW of kernel calc: 6.959 ms           [160.595 GB/s]
Effective BW by CUDA saxpy: 192.386 ms          [5.809 GB/s]
Effective BW of kernel calc: 7.489 ms           [149.233 GB/s]
~~~

Cuda is fater than ispc in the terms of calculations, but its' mempry copy operation is so time-consuming. The tested bandwidth is close to the theoretical value which is 193GB/s.