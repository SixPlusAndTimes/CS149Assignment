result :
~~~md
[saxpy ispc]:           [11.750] ms     [25.363] GB/s   [3.404] GFLOPS
[saxpy task ispc]:      [9.065] ms      [32.875] GB/s   [4.412] GFLOPS
                                (1.30x speedup from use of tasks)
~~~
Only 1.30x speed up can achieve from ispc_tasks.Because the computation is I/O-bound.

Theoretical computation:
Again My cpu is: `11th Gen Intel(R) Core(TM) i5-11400H @ 2.70GHz` and wsl limit corenum to 4.
 Theoretically, maximum GFLOPS = CoresNums * Clock Speed (GHz) * FLOPS per Cycle = 4 * 2.7 * 8(may be even larger) = 86.4 GFPLOPS. To meet the maximum GFPLOPS, the required memory bandwidth should be `86.4 GFPLOPS / 1FPLOPS / 4BYTES(32FP) = 345.6GB/s`
 But theoretically memmory bandwidth is [51.2GB / s](https://www.intel.cn/content/www/cn/zh/products/sku/213805/intel-core-i511400h-processor-12m-cache-up-to-4-50-ghz/specifications.html#:~:text=2-,%E6%9C%80%E5%A4%A7%E5%86%85%E5%AD%98%E5%B8%A6%E5%AE%BD,-51.2%20GB/s). So modern cpu's compute power is not the bottleneck but the memorybandwith is.