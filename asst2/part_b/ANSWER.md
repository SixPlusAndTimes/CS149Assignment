
original correct implementation: PERF result is following, some sync tests has failed to pass.
~~~md
➜  part_b git:(main) ✗ python3 ../tests/run_test_harness.py -a
runtasks_ref
Linux x86_64
================================================================================
Running task system grading harness... (22 total tests)
  - Detected CPU with 8 execution contexts
  - Task system configured to use at most 8 threads
================================================================================
================================================================================
Executing test: super_super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                4.59      5.176       0.89  (OK)
[Parallel + Always Spawn]               5.513     120.967     0.05  (OK)
[Parallel + Thread Pool + Spin]         5.387     23.058      0.23  (OK)
[Parallel + Thread Pool + Sleep]        47.596    39.051      1.22  (OK)
================================================================================
Executing test: super_super_light_async...
Reference binary: ./runtasks_ref_linux
Results for: super_super_light_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                4.623     5.193       0.89  (OK)
[Parallel + Always Spawn]               5.351     122.087     0.04  (OK)
[Parallel + Thread Pool + Spin]         5.314     18.023      0.29  (OK)
[Parallel + Thread Pool + Sleep]        11.853    24.684      0.48  (OK)
================================================================================
Executing test: super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                63.879    66.577      0.96  (OK)
[Parallel + Always Spawn]               63.511    129.195     0.49  (OK)
[Parallel + Thread Pool + Spin]         62.504    30.364      2.06  (NOT OK)
[Parallel + Thread Pool + Sleep]        67.457    41.781      1.61  (NOT OK)
================================================================================
Executing test: super_light_async...
Reference binary: ./runtasks_ref_linux
Results for: super_light_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                63.088    67.028      0.94  (OK)
[Parallel + Always Spawn]               62.572    127.858     0.49  (OK)
[Parallel + Thread Pool + Spin]         62.213    25.487      2.44  (NOT OK)
[Parallel + Thread Pool + Sleep]        29.413    34.094      0.86  (OK)
================================================================================
Executing test: ping_pong_equal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_equal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1045.914  1104.507    0.95  (OK)
[Parallel + Always Spawn]               1044.569  312.782     3.34  (NOT OK)
[Parallel + Thread Pool + Spin]         1040.416  245.787     4.23  (NOT OK)
[Parallel + Thread Pool + Sleep]        253.139   255.036     0.99  (OK)
================================================================================
Executing test: ping_pong_equal_async...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_equal_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1034.08   1110.039    0.93  (OK)
[Parallel + Always Spawn]               1033.909  316.404     3.27  (NOT OK)
[Parallel + Thread Pool + Spin]         1025.556  255.766     4.01  (NOT OK)
[Parallel + Thread Pool + Sleep]        217.495   242.501     0.90  (OK)
================================================================================
Executing test: ping_pong_unequal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1555.376  1565.179    0.99  (OK)
[Parallel + Always Spawn]               1559.839  393.022     3.97  (NOT OK)
[Parallel + Thread Pool + Spin]         1550.855  331.264     4.68  (NOT OK)
[Parallel + Thread Pool + Sleep]        334.977   333.978     1.00  (OK)
================================================================================
Executing test: ping_pong_unequal_async...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1556.645  1567.986    0.99  (OK)
[Parallel + Always Spawn]               1556.532  397.567     3.92  (NOT OK)
[Parallel + Thread Pool + Spin]         1551.312  331.157     4.68  (NOT OK)
[Parallel + Thread Pool + Sleep]        297.209   301.373     0.99  (OK)
================================================================================
Executing test: recursive_fibonacci...
Reference binary: ./runtasks_ref_linux
Results for: recursive_fibonacci
                                        STUDENT   REFERENCE   PERF?
[Serial]                                782.421   1311.051    0.60  (OK)
[Parallel + Always Spawn]               780.507   240.921     3.24  (NOT OK)
[Parallel + Thread Pool + Spin]         780.502   250.949     3.11  (NOT OK)
[Parallel + Thread Pool + Sleep]        158.599   227.56      0.70  (OK)
================================================================================
Executing test: recursive_fibonacci_async...
Reference binary: ./runtasks_ref_linux
Results for: recursive_fibonacci_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                785.134   1308.337    0.60  (OK)
[Parallel + Always Spawn]               782.655   238.588     3.28  (NOT OK)
[Parallel + Thread Pool + Spin]         779.706   234.047     3.33  (NOT OK)
[Parallel + Thread Pool + Sleep]        149.887   220.149     0.68  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop
                                        STUDENT   REFERENCE   PERF?
[Serial]                                492.855   518.46      0.95  (OK)
[Parallel + Always Spawn]               499.716   514.302     0.97  (OK)
[Parallel + Thread Pool + Spin]         502.062   166.016     3.02  (NOT OK)
[Parallel + Thread Pool + Sleep]        387.519   260.237     1.49  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_async...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                500.134   506.514     0.99  (OK)
[Parallel + Always Spawn]               502.658   498.004     1.01  (OK)
[Parallel + Thread Pool + Spin]         497.772   155.128     3.21  (NOT OK)
[Parallel + Thread Pool + Sleep]        250.697   216.439     1.16  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fewer_tasks...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fewer_tasks
                                        STUDENT   REFERENCE   PERF?
[Serial]                                508.337   514.144     0.99  (OK)
[Parallel + Always Spawn]               511.246   484.791     1.05  (OK)
[Parallel + Thread Pool + Spin]         505.98    174.051     2.91  (NOT OK)
[Parallel + Thread Pool + Sleep]        416.984   288.946     1.44  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fewer_tasks_async...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fewer_tasks_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                509.615   523.233     0.97  (OK)
[Parallel + Always Spawn]               503.328   488.903     1.03  (OK)
[Parallel + Thread Pool + Spin]         507.326   99.029      5.12  (NOT OK)
[Parallel + Thread Pool + Sleep]        90.798    91.546      0.99  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fan_in...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fan_in
                                        STUDENT   REFERENCE   PERF?
[Serial]                                264.016   268.041     0.98  (OK)
[Parallel + Always Spawn]               262.338   94.796      2.77  (NOT OK)
[Parallel + Thread Pool + Spin]         263.522   66.412      3.97  (NOT OK)
[Parallel + Thread Pool + Sleep]        85.942    73.893      1.16  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fan_in_async...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fan_in_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                263.967   269.124     0.98  (OK)
[Parallel + Always Spawn]               261.748   93.892      2.79  (NOT OK)
[Parallel + Thread Pool + Spin]         260.641   52.182      4.99  (NOT OK)
[Parallel + Thread Pool + Sleep]        47.044    49.104      0.96  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_reduction_tree...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_reduction_tree
                                        STUDENT   REFERENCE   PERF?
[Serial]                                262.318   267.759     0.98  (OK)
[Parallel + Always Spawn]               258.712   60.455      4.28  (NOT OK)
[Parallel + Thread Pool + Spin]         258.588   56.979      4.54  (NOT OK)
[Parallel + Thread Pool + Sleep]        54.306    56.539      0.96  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_reduction_tree_async...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_reduction_tree_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                258.274   262.23      0.98  (OK)
[Parallel + Always Spawn]               257.322   59.635      4.31  (NOT OK)
[Parallel + Thread Pool + Spin]         256.878   49.653      5.17  (NOT OK)
[Parallel + Thread Pool + Sleep]        44.615    45.9        0.97  (OK)
================================================================================
Executing test: spin_between_run_calls...
Reference binary: ./runtasks_ref_linux
Results for: spin_between_run_calls
                                        STUDENT   REFERENCE   PERF?
[Serial]                                277.426   480.404     0.58  (OK)
[Parallel + Always Spawn]               279.996   244.731     1.14  (OK)
[Parallel + Thread Pool + Spin]         280.246   283.766     0.99  (OK)
[Parallel + Thread Pool + Sleep]        178.431   246.618     0.72  (OK)
================================================================================
Executing test: spin_between_run_calls_async...
Reference binary: ./runtasks_ref_linux
Results for: spin_between_run_calls_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                279.213   476.81      0.59  (OK)
[Parallel + Always Spawn]               279.879   244.729     1.14  (OK)
[Parallel + Thread Pool + Spin]         281.76    279.576     1.01  (OK)
[Parallel + Thread Pool + Sleep]        180.042   245.616     0.73  (OK)
================================================================================
Executing test: mandelbrot_chunked...
Reference binary: ./runtasks_ref_linux
Results for: mandelbrot_chunked
                                        STUDENT   REFERENCE   PERF?
[Serial]                                418.071   414.412     1.01  (OK)
[Parallel + Always Spawn]               420.103   57.948      7.25  (NOT OK)
[Parallel + Thread Pool + Spin]         415.18    63.548      6.53  (NOT OK)
[Parallel + Thread Pool + Sleep]        57.813    58.356      0.99  (OK)
================================================================================
Executing test: mandelbrot_chunked_async...
Reference binary: ./runtasks_ref_linux
Results for: mandelbrot_chunked_async
                                        STUDENT   REFERENCE   PERF?
[Serial]                                414.91    417.726     0.99  (OK)
[Parallel + Always Spawn]               416.311   57.788      7.20  (NOT OK)
[Parallel + Thread Pool + Spin]         419.439   61.685      6.80  (NOT OK)
[Parallel + Thread Pool + Sleep]        57.561    58.114      0.99  (OK)
================================================================================
Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : Perf did not pass all tests
[Parallel + Thread Pool + Spin]         : Perf did not pass all tests
[Parallel + Thread Pool + Sleep]        : Perf did not pass all tests
~~~
