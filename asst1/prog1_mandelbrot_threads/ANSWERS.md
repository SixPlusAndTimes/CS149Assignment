## My Environment Settings
My cpu : 11th Gen Intel(R) Core(TM) i5-11400H @ 2.70GHz

My env: wls2 limited to 8 (virtual)cores, 12GB memory. You can refer to [how to modify cpu cores in wls2](https://learn.microsoft.com/en-us/answers/questions/1296124/how-to-increase-memory-and-cpu-limits-for-wsl2-win)

## prog1: Mandelbrot Multi-threading
### Implementation Summary
This assignment implements parallelization of Mandelbrot set computation using **spatial decomposition**:

#### implementation 1
1. **Threads divide work by image regions**: Each thread computes a block of the image
2. **Dynamic row distribution**: Last thread handles remaining rows when height not evenly divisible

1. Speedup Results (1-8 Threads, VIEW 1)

VIEW 1:
Threads | Serial Time | Thread Time | Speedup
--------|-------------|-------------|--------
   1 |      443.88 |      441.42 |    1.01
   2 |      429.90 |      217.44 |    1.98
   3 |      431.69 |      273.03 |    1.58
   4 |      447.57 |      184.53 |    2.43
   5 |      470.56 |      188.15 |    2.50
   6 |      441.93 |      136.66 |    3.23
   7 |      445.93 |      131.33 |    3.40
   8 |      444.61 |      110.16 |    4.04
  16 |      444.29 |       77.78 |    5.71

VIEW 2:
Threads | Serial Time | Thread Time | Speedup
--------|-------------|-------------|--------
   1 |      273.06 |      265.42 |    1.03
   2 |      263.66 |      155.34 |    1.70
   3 |      275.75 |      124.29 |    2.22
   4 |      280.07 |      110.14 |    2.54
   5 |      273.51 |       90.61 |    3.02
   6 |      274.46 |       79.32 |    3.46
   7 |      271.91 |       71.36 |    3.81
   8 |      273.92 |       63.18 |    4.34
  16 |      273.22 |       40.52 |    6.74

2. Is Speedup Linear?
No, because the overloadig of threads are not balanced

3 threads:
~~~
Thread 0 completed in 79.631 ms
Thread 2 completed in 80.574 ms
Thread 1 completed in 242.251 ms
~~~
Obviously thread 1 have the most significant overloading. 

8 threads:
~~~
Thread 0 completed in 7.596 ms
Thread 7 completed in 7.823 ms
Thread 1 completed in 38.047 ms
Thread 6 completed in 38.263 ms
Thread 2 completed in 74.456 ms
Thread 5 completed in 76.675 ms
Thread 3 completed in 110.445 ms
Thread 4 completed in 114.860 ms
~~~
The overloading is still not balanced.


### implementation 2
Interleaved row assignment for better load balancing. Thread i processes rows i, i+numThreads, i+2*numThreads, etc.
Now we can achieve 7-8× speedup
VIEW 1 :
Threads | Serial Time | Thread Time | Speedup
--------|-------------|-------------|--------
   1 |      441.05 |      441.50 |    1.00
   2 |      434.64 |      221.71 |    1.96
   3 |      442.01 |      150.47 |    2.94
   4 |      443.59 |      114.21 |    3.88
   5 |      442.21 |       91.33 |    4.84
   6 |      442.64 |       78.35 |    5.65
   7 |      435.25 |       67.65 |    6.43
   8 |      441.04 |       60.35 |    7.31
  16 |      441.06 |       62.60 |    7.05

VIEW 2 :
Threads | Serial Time | Thread Time | Speedup
--------|-------------|-------------|--------
   1 |      268.30 |      273.04 |    0.98
   2 |      271.73 |      138.59 |    1.96
   3 |      272.25 |       93.52 |    2.91
   4 |      271.67 |       70.59 |    3.85
   5 |      273.56 |       58.18 |    4.70
   6 |      273.52 |       49.04 |    5.58
   7 |      273.04 |       42.22 |    6.47
   8 |      265.13 |       39.06 |    6.79
  16 |      267.31 |       40.03 |    6.68


**overloading of each threads are now balanced:**
~~~
   Thread 2 completed in 148.589 ms
   Thread 1 completed in 148.907 ms
   Thread 0 completed in 149.465 ms
~~~

~~~
   Thread 4 completed in 60.767 ms
   Thread 7 completed in 60.628 ms
   Thread 3 completed in 60.906 ms
   Thread 6 completed in 61.281 ms
   Thread 0 completed in 61.786 ms
   Thread 2 completed in 62.665 ms
   Thread 1 completed in 63.057 ms
   Thread 5 completed in 62.864 ms
~~~

Performance when runing with sixteen threads is NOT noticably greater than eight threads? Because we only have 4 cores in VM, and each core can exec 2 threads simultaneously.