# step1
## implementA
~~~c++
void TaskSystemParallelSpawnThreadWorker2ndEdition(int threadidx, IRunnable* runable, int num_total_tasks, int form, int to)
{

    for (int i = form; i < to; i++)
    {
        runable->runTask(i, num_total_tasks);
    }
}
void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {

    int step = num_total_tasks / m_num_threads;    
    for (int i = 0; i < m_num_threads - 1; i+=1) {
        m_threads[i] = std::thread(TaskSystemParallelSpawnThreadWorker2ndEdition,i, runnable, num_total_tasks, i * step, (i + 1) * step);
    }

    TaskSystemParallelSpawnThreadWorker2ndEdition(m_num_threads - 1, runnable, num_total_tasks, (m_num_threads - 1) * step, num_total_tasks);

    for (int i = 0; i < m_num_threads - 1; i++) {
        m_threads[i].join();
    }
}
~~~

Run the test and we can see that there are two tasks not OK :
~~~md

Executing test: ping_pong_unequal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1621.976  1621.016    1.00  (OK)
[Parallel + Always Spawn]               471.117   357.591     1.32  (NOT OK)
[Parallel + Thread Pool + Spin]         1623.466  345.167     4.70  (NOT OK)
[Parallel + Thread Pool + Sleep]        1637.149  339.5       4.82  (NOT OK)
================================================================================

Executing test: spin_between_run_calls...
Reference binary: ./runtasks_ref_linux
Results for: spin_between_run_calls
                                        STUDENT   REFERENCE   PERF?
[Serial]                                288.777   476.151     0.61  (OK)
[Parallel + Always Spawn]               292.367   241.963     1.21  (NOT OK)
[Parallel + Thread Pool + Spin]         291.891   280.62      1.04  (OK)
[Parallel + Thread Pool + Sleep]        287.525   243.818     1.18  (OK)
================================================================================

Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : Perf did not pass all tests 
[Parallel + Thread Pool + Spin]         : Perf did not pass all tests
[Parallel + Thread Pool + Sleep]        : Perf did not pass all tests
~~~

Above two tasks's work loading is not balanced. So we should slice the whole task into several pieces, and asign each piece to threads interleavely, hoping that can let each thread get the same work loading on average.

## implement B
~~~c++
void TaskSystemParallelSpawnThreadWorker(int thread_index, IRunnable* runnable, int num_total_tasks, int assignedfrom, int num_tasks_each_slice, int step)
{
    for (int i = assignedfrom; i < num_total_tasks; i += step)
    {
        for (int j = 0; j < num_tasks_each_slice; j++)
        {
            runnable->runTask(i + j, num_total_tasks);
        }
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {

    int multipler = 4;
    int num_slice = m_num_threads * multipler;
    int num_tasks_each_slice = num_total_tasks / (num_slice);
    if (num_tasks_each_slice <= 0)
    {
        num_tasks_each_slice = 1;
    }

    // int step = num_total_tasks / m_num_threads;    
    for (int i = 0; i < m_num_threads - 1; i++) {
        m_threads[i] = std::thread(TaskSystemParallelSpawnThreadWorker,i, runnable, num_total_tasks, i * num_tasks_each_slice, num_tasks_each_slice, m_num_threads * num_tasks_each_slice);
    }

    TaskSystemParallelSpawnThreadWorker(m_num_threads - 1, runnable, num_total_tasks, (m_num_threads - 1) * num_tasks_each_slice, num_tasks_each_slice, m_num_threads * num_tasks_each_slice);
    if (num_slice * num_tasks_each_slice < num_total_tasks)
    {
        for (int i = num_slice * num_tasks_each_slice; i < num_total_tasks; i++)
        {
            runnable->runTask(i, num_total_tasks);
        }
    }

    for (int i = 0; i < m_num_threads - 1; i++) {
        m_threads[i].join();
    }
}
~~~

perf test:
~~~md
Executing test: ping_pong_unequal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1614.725  1601.918    1.01  (OK)
[Parallel + Always Spawn]               446.718   359.554     1.24  (NOT OK)
[Parallel + Thread Pool + Spin]         1645.188  343.985     4.78  (NOT OK)
[Parallel + Thread Pool + Sleep]        1630.85   331.177     4.92  (NOT OK)
/// ......
Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : Perf did not pass all tests
[Parallel + Thread Pool + Spin]         : Perf did not pass all tests
[Parallel + Thread Pool + Sleep]        : Perf did not pass all tests
~~~

Still one task is not equal, so I try to slice the tasks to more pieces:
~~~c++
    int multipler = 8;
    int num_slice = m_num_threads * multipler;
    int num_tasks_each_slice = num_total_tasks / (num_slice);
~~~
And it worked :

~~~md
// ......
Executing test: ping_pong_unequal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1630.528  1609.641    1.01  (OK)
[Parallel + Always Spawn]               403.927   360.977     1.12  (OK)
[Parallel + Thread Pool + Spin]         1519.358  346.356     4.39  (NOT OK)
[Parallel + Thread Pool + Sleep]        1607.045  342.001     4.70  (NOT OK)
// ......
Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : All passed Perf
[Parallel + Thread Pool + Spin]         : Perf did not pass all tests
[Parallel + Thread Pool + Sleep]        : Perf did not pass all tests
~~~