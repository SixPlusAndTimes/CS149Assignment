# Part2
~~~c++
  for (int b = 0; b < B; ++b)
    {
        for (int h = 0; h < H; ++h)
        {
            for (int i_block = 0; i_block < N; i_block += BLOCK_SIZE)
            {
                for (int j_block = 0; j_block < N; j_block += BLOCK_SIZE)
                {
                    for (int k_block = 0; k_block < d; k_block += BLOCK_SIZE)
                    {

                        for (int i = 0; i < BLOCK_SIZE; ++i)
                        {
                            if (i + i_block >= N) break;
                            for (int j = 0; j < BLOCK_SIZE; ++j)
                            {
                                if (j + j_block >= N) break;
                                float block_sum = 0.f;
                                for (int k = 0; k < BLOCK_SIZE; ++k)
                                {
                                    if (k + k_block >= d) break;
                                    block_sum += fourDimRead(Q, b, h, i + i_block, k + k_block, H, N, d) * fourDimRead(K, b, h, j + j_block, k + k_block, H, N, d);
                                }
                                float origin = twoDimRead(QK_t, i + i_block, j + j_block, N);
                                twoDimWrite(QK_t, i + i_block, j + j_block, N, block_sum + origin);
                            }
                        }
                    }
                }
            }

            for (int i = 0; i < N; ++i) 
            {
                float sum = 0.0;
                for (int j = 0; j < N; ++j) 
                {
                    sum += std::exp(twoDimRead(QK_t, i, j, N));
                }

                for (int j = 0; j < N; ++j) 
                {
                    float val = std::exp(twoDimRead(QK_t, i, j, N)) / sum;
                    twoDimWrite(QK_t, i, j, N, val);
                }
            }

            for (int i_block = 0; i_block < N; i_block += BLOCK_SIZE)
            {
                for (int j_block = 0; j_block < d; j_block += BLOCK_SIZE)
                {
                    for (int k_block = 0; k_block < N; k_block += BLOCK_SIZE)
                    {

                        for (int i = 0; i < BLOCK_SIZE; ++i)
                        {
                            if (i + i_block >= N) break;
                            for (int j = 0; j < BLOCK_SIZE; ++j)
                            {
                                if (j + j_block >= d) break;
                                float block_sum = 0.f;
                                for (int k = 0; k < BLOCK_SIZE; ++k)
                                {
                                    if (k + k_block >= N) break;
                                    block_sum += twoDimRead(QK_t, i + i_block, k + k_block, N) * fourDimRead(V, b, h, k + k_block, j + j_block, H, N, d);
                                }
                                float origin = fourDimRead(O, b, h, i + i_block, j + j_block, H, N, d);
                                fourDimWrite(O, b, h, i + i_block, j + j_block, H, N, d, block_sum + origin);
                            }
                        }
                    }
                }
            }

        }
    }
~~~

test result is following: 
~~~md
blocks : 64
Running Part 2 Test: Unfused Attention with Blocked Matmul

-----RUNNING REFERENCE IMPLEMENTATION-----

STAGE:2026-07-08 23:04:14 43930:43930 ActivityProfilerController.cpp:312] Completed Stage: Warm Up
STAGE:2026-07-08 23:04:14 43930:43930 ActivityProfilerController.cpp:318] Completed Stage: Collection
STAGE:2026-07-08 23:04:14 43930:43930 ActivityProfilerController.cpp:322] Completed Stage: Post Processing
manual attention == pytorch attention True
Manual Execution Time:  0.16151022911071777 

------------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                            Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg       CPU Mem  Self CPU Mem    # of Calls  
------------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                     aten::empty         0.05%      78.000us         0.05%      78.000us      26.000us       5.00 Mb       5.00 Mb             3  
    REFERENCE - BLOCKED MATMUL + UNFUSED SOFTMAX        91.51%     147.871ms        99.93%     161.473ms     161.473ms       4.50 Mb      -1.00 Mb             1  
                                     aten::zeros         0.03%      52.000us         5.34%       8.627ms       4.314ms       4.50 Mb           0 b             2  
                                     aten::clone         0.05%      86.000us         2.97%       4.791ms       2.396ms       1.00 Mb           0 b             2  
                                 model_inference         0.07%     110.000us       100.00%     161.583ms     161.583ms     512.00 Kb      -4.00 Mb             1  
                                   aten::flatten         0.09%     146.000us         2.91%       4.695ms     939.000us     512.00 Kb           0 b             5  
                                aten::empty_like         0.02%      25.000us         0.03%      44.000us      44.000us     512.00 Kb           0 b             1  
                             aten::empty_strided         0.03%      55.000us         0.03%      55.000us      55.000us     512.00 Kb     512.00 Kb             1  
                                     aten::zero_         0.04%      71.000us         5.27%       8.516ms       4.258ms           0 b           0 b             2  
                                     aten::fill_         5.23%       8.445ms         5.23%       8.445ms       4.223ms           0 b           0 b             2  
------------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
Self CPU time total: 161.583ms

REFERENCE - BLOCKED MATMUL + UNFUSED SOFTMAX statistics
cpu time:  161.473ms
mem usage:  4718592 bytes
-----RUNNING STUDENT IMPLEMENTATION-----

STAGE:2026-07-08 23:04:20 43930:43930 ActivityProfilerController.cpp:312] Completed Stage: Warm Up
STAGE:2026-07-08 23:04:20 43930:43930 ActivityProfilerController.cpp:318] Completed Stage: Collection
STAGE:2026-07-08 23:04:20 43930:43930 ActivityProfilerController.cpp:322] Completed Stage: Post Processing
manual attention == pytorch attention True
Manual Execution Time:  0.2207016944885254 

----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                          Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg       CPU Mem  Self CPU Mem    # of Calls  
----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                   aten::empty         0.01%      30.000us         0.01%      30.000us      10.000us       5.00 Mb       5.00 Mb             3  
    STUDENT - BLOCKED MATMUL + UNFUSED SOFTMAX        99.52%     219.670ms        99.97%     220.665ms     220.665ms       4.50 Mb      -1.00 Mb             1  
                                   aten::zeros         0.01%      17.000us         0.19%     424.000us     212.000us       4.50 Mb           0 b             2  
                                   aten::clone         0.02%      37.000us         0.22%     484.000us     242.000us       1.00 Mb           0 b             2  
                               model_inference         0.03%      64.000us       100.00%     220.729ms     220.729ms     512.00 Kb      -4.00 Mb             1  
                                 aten::flatten         0.03%      63.000us         0.15%     325.000us      65.000us     512.00 Kb           0 b             5  
                              aten::empty_like         0.00%       7.000us         0.00%       9.000us       9.000us     512.00 Kb           0 b             1  
                           aten::empty_strided         0.00%      11.000us         0.00%      11.000us      11.000us     512.00 Kb     512.00 Kb             1  
                                   aten::zero_         0.01%      13.000us         0.17%     379.000us     189.500us           0 b           0 b             2  
                                   aten::fill_         0.17%     366.000us         0.17%     366.000us     183.000us           0 b           0 b             2  
----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
Self CPU time total: 220.729ms

STUDENT - BLOCKED MATMUL + UNFUSED SOFTMAX statistics
cpu time:  220.665ms
mem usage:  4718592 bytes

blocksize 16:
STAGE:2026-07-08 23:28:06 57685:57685 ActivityProfilerController.cpp:312] Completed Stage: Warm Up
STAGE:2026-07-08 23:28:06 57685:57685 ActivityProfilerController.cpp:318] Completed Stage: Collection
STAGE:2026-07-08 23:28:06 57685:57685 ActivityProfilerController.cpp:322] Completed Stage: Post Processing
manual attention == pytorch attention True
Manual Execution Time:  0.19634008407592773 

----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                          Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg       CPU Mem  Self CPU Mem    # of Calls  
----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                   aten::empty         0.03%      64.000us         0.03%      64.000us      21.333us       5.00 Mb       5.00 Mb             3  
    STUDENT - BLOCKED MATMUL + UNFUSED SOFTMAX        99.27%     194.943ms        99.96%     196.303ms     196.303ms       4.50 Mb      -1.00 Mb             1  
                                   aten::zeros         0.02%      32.000us         0.33%     647.000us     323.500us       4.50 Mb           0 b             2  
                                   aten::clone         0.05%      90.000us         0.28%     545.000us     272.500us       1.00 Mb           0 b             2  
                               model_inference         0.04%      75.000us       100.00%     196.378ms     196.378ms     512.00 Kb      -4.00 Mb             1  
                                 aten::flatten         0.04%      87.000us         0.24%     470.000us      94.000us     512.00 Kb           0 b             5  
                              aten::empty_like         0.01%      20.000us         0.02%      32.000us      32.000us     512.00 Kb           0 b             1  
                           aten::empty_strided         0.01%      11.000us         0.01%      11.000us      11.000us     512.00 Kb     512.00 Kb             1  
                                   aten::zero_         0.01%      22.000us         0.29%     563.000us     281.500us           0 b           0 b             2  
                                   aten::fill_         0.28%     541.000us         0.28%     541.000us     270.500us           0 b           0 b             2  
----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
Self CPU time total: 196.378ms

STUDENT - BLOCKED MATMUL + UNFUSED SOFTMAX statistics
cpu time:  196.303ms
mem usage:  4718592 bytes

blocksize 8 :
-----RUNNING STUDENT IMPLEMENTATION-----

STAGE:2026-07-08 23:30:38 58912:58912 ActivityProfilerController.cpp:312] Completed Stage: Warm Up
STAGE:2026-07-08 23:30:38 58912:58912 ActivityProfilerController.cpp:318] Completed Stage: Collection
STAGE:2026-07-08 23:30:38 58912:58912 ActivityProfilerController.cpp:322] Completed Stage: Post Processing
manual attention == pytorch attention True
Manual Execution Time:  0.1853632926940918 

----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                          Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg       CPU Mem  Self CPU Mem    # of Calls  
----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                   aten::empty         0.03%      47.000us         0.03%      47.000us      15.667us       5.00 Mb       5.00 Mb             3  
    STUDENT - BLOCKED MATMUL + UNFUSED SOFTMAX        99.41%     184.302ms        99.97%     185.333ms     185.333ms       4.50 Mb      -1.00 Mb             1  
                                   aten::zeros         0.01%      26.000us         0.33%     620.000us     310.000us       4.50 Mb           0 b             2  
                                   aten::clone         0.02%      38.000us         0.19%     345.000us     172.500us       1.00 Mb           0 b             2  
                               model_inference         0.03%      63.000us       100.00%     185.396ms     185.396ms     512.00 Kb      -4.00 Mb             1  
                                 aten::flatten         0.02%      37.000us         0.10%     177.000us      35.400us     512.00 Kb           0 b             5  
                              aten::empty_like         0.00%       9.000us         0.01%      13.000us      13.000us     512.00 Kb           0 b             1  
                           aten::empty_strided         0.01%      11.000us         0.01%      11.000us      11.000us     512.00 Kb     512.00 Kb             1  
                                   aten::zero_         0.01%      21.000us         0.30%     551.000us     275.500us           0 b           0 b             2  
                                   aten::fill_         0.29%     530.000us         0.29%     530.000us     265.000us           0 b           0 b             2  
----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
Self CPU time total: 185.396ms

STUDENT - BLOCKED MATMUL + UNFUSED SOFTMAX statistics
cpu time:  185.333ms
mem usage:  4718592 bytes
~~~


delete the if branch:

~~~c++

    for (int b = 0; b < B; ++b)
    {
        for (int h = 0; h < H; ++h)
        {
            for (int i_block = 0; i_block < N; i_block += BLOCK_SIZE)
            {
                for (int j_block = 0; j_block < N; j_block += BLOCK_SIZE)
                {
                    for (int k_block = 0; k_block < d; k_block += BLOCK_SIZE)
                    {

                        int i_limit = std::min(N - i_block, BLOCK_SIZE);
                        int j_limit = std::min(N - j_block, BLOCK_SIZE);
                        int k_limit = std::min(d - k_block, BLOCK_SIZE);
                        for (int i = 0; i < i_limit; ++i)
                        {
                            for (int j = 0; j < j_limit; ++j)
                            {
                                float block_sum = 0.f;
                                for (int k = 0; k < k_limit; ++k)
                                {
                                    block_sum += fourDimRead(Q, b, h, i + i_block, k + k_block, H, N, d) * fourDimRead(K, b, h, j + j_block, k + k_block, H, N, d);
                                }
                                float origin = twoDimRead(QK_t, i + i_block, j + j_block, N);
                                twoDimWrite(QK_t, i + i_block, j + j_block, N, block_sum + origin);
                            }
                        }
                    }
                }
            }

            for (int i = 0; i < N; ++i) 
            {
                float sum = 0.0;
                for (int j = 0; j < N; ++j) 
                {
                    sum += std::exp(twoDimRead(QK_t, i, j, N));
                }

                for (int j = 0; j < N; ++j) 
                {
                    float val = std::exp(twoDimRead(QK_t, i, j, N)) / sum;
                    twoDimWrite(QK_t, i, j, N, val);
                }
            }

            for (int i_block = 0; i_block < N; i_block += BLOCK_SIZE)
            {
                for (int j_block = 0; j_block < d; j_block += BLOCK_SIZE)
                {
                    for (int k_block = 0; k_block < N; k_block += BLOCK_SIZE)
                    {

                        int i_limit = std::min(N - i_block, BLOCK_SIZE);
                        int j_limit = std::min(d - j_block, BLOCK_SIZE);
                        int k_limit = std::min(N - k_block, BLOCK_SIZE);
                        for (int i = 0; i < i_limit; ++i)
                        {
                            // if (i + i_block >= N) break;
                            for (int j = 0; j < j_limit; ++j)
                            {
                                // if (j + j_block >= d) break;
                                float block_sum = 0.f;
                                for (int k = 0; k < k_limit; ++k)
                                {
                                    // if (k + k_block >= N) break;
                                    block_sum += twoDimRead(QK_t, i + i_block, k + k_block, N) * fourDimRead(V, b, h, k + k_block, j + j_block, H, N, d);
                                }
                                float origin = fourDimRead(O, b, h, i + i_block, j + j_block, H, N, d);
                                fourDimWrite(O, b, h, i + i_block, j + j_block, H, N, d, block_sum  + origin);
                            }
                        }
                    }
                }
            }

        }
    }

~~~

result: 
~~~md
Compiling code into a PyTorch module...


Running Part 2 Test: Unfused Attention with Blocked Matmul

-----RUNNING REFERENCE IMPLEMENTATION-----

STAGE:2026-07-08 23:52:28 67889:67889 ActivityProfilerController.cpp:312] Completed Stage: Warm Up
STAGE:2026-07-08 23:52:28 67889:67889 ActivityProfilerController.cpp:318] Completed Stage: Collection
STAGE:2026-07-08 23:52:28 67889:67889 ActivityProfilerController.cpp:322] Completed Stage: Post Processing
manual attention == pytorch attention True
Manual Execution Time:  0.1644890308380127 

------------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                            Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg       CPU Mem  Self CPU Mem    # of Calls  
------------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                     aten::empty         0.06%      93.000us         0.06%      93.000us      31.000us       5.00 Mb       5.00 Mb             3  
    REFERENCE - BLOCKED MATMUL + UNFUSED SOFTMAX        99.06%     162.984ms        99.96%     164.460ms     164.460ms       4.50 Mb      -1.00 Mb             1  
                                     aten::zeros         0.03%      48.000us         0.57%     935.000us     467.500us       4.50 Mb           0 b             2  
                                     aten::clone         0.04%      59.000us         0.28%     461.000us     230.500us       1.00 Mb           0 b             2  
                                 model_inference         0.04%      67.000us       100.00%     164.527ms     164.527ms     512.00 Kb      -4.00 Mb             1  
                                   aten::flatten         0.04%      61.000us         0.15%     240.000us      48.000us     512.00 Kb           0 b             5  
                                aten::empty_like         0.00%       8.000us         0.01%      16.000us      16.000us     512.00 Kb           0 b             1  
                             aten::empty_strided         0.03%      55.000us         0.03%      55.000us      55.000us     512.00 Kb     512.00 Kb             1  
                                     aten::zero_         0.03%      56.000us         0.49%     802.000us     401.000us           0 b           0 b             2  
                                     aten::fill_         0.45%     746.000us         0.45%     746.000us     373.000us           0 b           0 b             2  
------------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
Self CPU time total: 164.527ms

REFERENCE - BLOCKED MATMUL + UNFUSED SOFTMAX statistics
cpu time:  164.46ms
mem usage:  4718592 bytes
-----RUNNING STUDENT IMPLEMENTATION-----

STAGE:2026-07-08 23:52:34 67889:67889 ActivityProfilerController.cpp:312] Completed Stage: Warm Up
STAGE:2026-07-08 23:52:34 67889:67889 ActivityProfilerController.cpp:318] Completed Stage: Collection
STAGE:2026-07-08 23:52:34 67889:67889 ActivityProfilerController.cpp:322] Completed Stage: Post Processing
manual attention == pytorch attention True
Manual Execution Time:  0.13139843940734863 

----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                          Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg       CPU Mem  Self CPU Mem    # of Calls  
----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                                   aten::empty         0.03%      37.000us         0.03%      37.000us      12.333us       5.00 Mb       5.00 Mb             3  
    STUDENT - BLOCKED MATMUL + UNFUSED SOFTMAX        99.38%     130.606ms        99.96%     131.369ms     131.369ms       4.50 Mb      -1.00 Mb             1  
                                   aten::zeros         0.01%      19.000us         0.30%     396.000us     198.000us       4.50 Mb           0 b             2  
                                   aten::clone         0.03%      39.000us         0.25%     331.000us     165.500us       1.00 Mb           0 b             2  
                               model_inference         0.04%      58.000us       100.00%     131.427ms     131.427ms     512.00 Kb      -4.00 Mb             1  
                                 aten::flatten         0.02%      23.000us         0.11%     148.000us      29.600us     512.00 Kb           0 b             5  
                              aten::empty_like         0.01%       7.000us         0.01%       9.000us       9.000us     512.00 Kb           0 b             1  
                           aten::empty_strided         0.01%      15.000us         0.01%      15.000us      15.000us     512.00 Kb     512.00 Kb             1  
                                   aten::zero_         0.01%      12.000us         0.26%     342.000us     171.000us           0 b           0 b             2  
                                   aten::fill_         0.25%     330.000us         0.25%     330.000us     165.000us           0 b           0 b             2  
----------------------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
Self CPU time total: 131.427ms

STUDENT - BLOCKED MATMUL + UNFUSED SOFTMAX statistics
cpu time:  131.369ms
mem usage:  4718592 bytes
~~~

# part3

result:

~~~md
Running Part 3 Test: Fused Attention

-----RUNNING REFERENCE IMPLEMENTATION-----

STAGE:2026-07-11 18:24:50 206531:206531 ActivityProfilerController.cpp:312] Completed Stage: Warm Up
STAGE:2026-07-11 18:24:50 206531:206531 ActivityProfilerController.cpp:318] Completed Stage: Collection
STAGE:2026-07-11 18:24:50 206531:206531 ActivityProfilerController.cpp:322] Completed Stage: Post Processing
manual attention == pytorch attention True
Manual Execution Time:  0.037329912185668945 

-------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                           Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg       CPU Mem  Self CPU Mem    # of Calls  
-------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                    aten::empty         0.06%      24.000us         0.06%      24.000us       8.000us       1.03 Mb       1.03 Mb             3  
                    aten::clone         0.10%      36.000us         0.78%     292.000us     146.000us       1.00 Mb           0 b             2  
    REFERENCE - FUSED ATTENTION        91.64%      34.244ms        99.84%      37.305ms      37.305ms     544.00 Kb      -1.00 Mb             1  
                    aten::zeros         0.11%      40.000us         0.82%     307.000us     153.500us     544.00 Kb           0 b             2  
                model_inference         0.16%      61.000us       100.00%      37.366ms      37.366ms     512.00 Kb     -32.00 Kb             1  
                  aten::flatten         2.35%     879.000us         3.13%       1.171ms       2.269us     512.00 Kb           0 b           516  
               aten::empty_like         0.10%      38.000us         0.13%      49.000us      49.000us     512.00 Kb           0 b             1  
            aten::empty_strided         0.06%      24.000us         0.06%      24.000us      24.000us     512.00 Kb     512.00 Kb             1  
                    aten::zero_         0.24%      90.000us         0.68%     254.000us     127.000us           0 b           0 b             2  
                    aten::fill_         0.44%     164.000us         0.44%     164.000us     164.000us           0 b           0 b             1  
-------------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
Self CPU time total: 37.366ms

REFERENCE - FUSED ATTENTION statistics
cpu time:  37.305ms
mem usage:  557056 bytes
-----RUNNING STUDENT IMPLEMENTATION-----

STAGE:2026-07-11 18:24:56 206531:206531 ActivityProfilerController.cpp:312] Completed Stage: Warm Up
STAGE:2026-07-11 18:24:56 206531:206531 ActivityProfilerController.cpp:318] Completed Stage: Collection
STAGE:2026-07-11 18:24:56 206531:206531 ActivityProfilerController.cpp:322] Completed Stage: Post Processing
manual attention == pytorch attention True
Manual Execution Time:  0.0475614070892334 

-----------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                         Name    Self CPU %      Self CPU   CPU total %     CPU total  CPU time avg       CPU Mem  Self CPU Mem    # of Calls  
-----------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
                  aten::empty         0.07%      33.000us         0.07%      33.000us       8.250us       1.04 Mb       1.04 Mb             4  
                  aten::clone         0.06%      27.000us         0.58%     275.000us     137.500us       1.00 Mb           0 b             2  
                  aten::zeros         0.04%      19.000us         0.25%     119.000us      39.667us     548.00 Kb           0 b             3  
    STUDENT - FUSED ATTENTION        93.52%      44.508ms        99.87%      47.530ms      47.530ms     544.00 Kb      -1.00 Mb             1  
              model_inference         0.13%      60.000us       100.00%      47.590ms      47.590ms     512.00 Kb     -32.00 Kb             1  
                aten::flatten         1.81%     861.000us         2.65%       1.260ms       2.437us     512.00 Kb           0 b           517  
             aten::empty_like         0.01%       6.000us         0.04%      17.000us      17.000us     512.00 Kb           0 b             1  
          aten::empty_strided         0.04%      21.000us         0.04%      21.000us      21.000us     512.00 Kb     512.00 Kb             1  
                  aten::zero_         0.02%      10.000us         0.16%      78.000us      26.000us           0 b           0 b             3  
                  aten::fill_         0.14%      68.000us         0.14%      68.000us      68.000us           0 b           0 b             1  
-----------------------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  ------------  
Self CPU time total: 47.590ms

STUDENT - FUSED ATTENTION statistics
cpu time:  47.53ms
~~~