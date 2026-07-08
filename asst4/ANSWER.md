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
~~~