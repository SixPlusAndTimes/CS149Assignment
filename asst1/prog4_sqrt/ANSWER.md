# Q1

~~~md
[sqrt serial]:          [778.245] ms
[sqrt ispc]:            [192.859] ms
[sqrt task ispc]:       [25.875] ms
                                (4.04x speedup from ISPC)
                                (30.08x speedup from task ISPC)
~~~
4.04x speedup for single CPU core, and 30.08x speedup for all cores.
4.04x speedup due to SIMD parrallelization and (30.08 / 4.04 =) 7.44x speed up due to multi-core parellelization

# Q2
To maximize the speedup, I should try my best to let all lanes have the same loadbalance and let the loadbalance be large enough to compensate for some computation-unrelated operations such as memory loading and context switch etc.
So a value that infinitely close to 3.f will be the best choice, but limted to the accuracy of float type, I chose `2.999999f`.
~~~c++
for (unsigned int i=0; i<N; i++)
{
    values[i] = 2.999999f;
}
~~~

result is :
~~~md
[sqrt serial]:          [4595.527] ms
[sqrt ispc]:            [701.672] ms
[sqrt task ispc]:       [90.519] ms
                                (6.55x speedup from ISPC)
                                (50.77x speedup from task ISPC)

~~~
i.e. 6.55x speedup for simd and 7.75x speedup for multicore 

# Q3
To minimize the speedup, I should introduce the divergence in one vector operation and I know that smaller computational load(relative to memory access and some other operations) results in worse speedup.
So in every 8 elems' computation,set one elem be `2.999999f` and the other seven be `1.f` will lead to minimal speedup.

~~~c++
for (unsigned int i=0; i<N; i++)
{
    if (i % 8 == 0)
    {
        values[i] = 2.999999f;
    }
    else 
    {
        values[i] = 1.f;
    }
}
~~~

result:
~~~md
[sqrt serial]:          [566.620] ms
[sqrt ispc]:            [675.777] ms
[sqrt task ispc]:       [90.690] ms
                                (0.84x speedup from ISPC)
                                (6.25x speedup from task ISPC)
~~~md
# Q4