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


# step2 
## implementA
~~~c++
void DoWork(int index, WorkQueue& workQueue,  EndSignal& endSignal)
{

    while (!workQueue.IsEmpty())
    {
        TaskDescription& taskDes = workQueue.Front();
        for (int i = taskDes.assign_from; i < taskDes.assign_to; i++)
        {
            taskDes.runnable->runTask(i, taskDes.num_total_tasks);
        }
        workQueue.DeQueueTask();
    }

    if (workQueue.IsEmpty())
    {
        std::unique_lock<std::mutex> endlock(endSignal.m_endSignalLock);
        endSignal.m_endSignalCV.notify_all();
        // printf("Enter thread idx %d, work done \n",index);
        endlock.unlock();
    }
}

void TaskSystemParallelThreadPoolSpinningWorker(int index, WorkQueue& workQueue, EndSignal& endSignal, bool& killed)
{
    while (true)
    {
        if (killed) break;
        DoWork(index, workQueue, endSignal);
    }
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): 
    ITaskSystem(num_threads), 
    m_threads(num_threads - 1), 
    m_num_threads(num_threads),
    m_workQuque(num_threads),
    m_killed(false)
{
    for (size_t i = 0; i < m_threads.size(); i++)
    {
        m_threads[i] = std::thread(TaskSystemParallelThreadPoolSpinningWorker, static_cast<int>(i), std::ref(m_workQuque[i]), std::ref(m_endSig), std::ref(m_killed));
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    m_killed = true;
    for (int i = 0; i < m_num_threads - 1; i++) {
        m_threads[i].join();
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {

    int multipler = 4;
    int num_slice = m_num_threads * multipler;
    int num_tasks_each_slice = num_total_tasks / (num_slice);
    if (num_tasks_each_slice <= 0)
    {
        num_tasks_each_slice = 1;
    }

    for (int i = 0; i < num_total_tasks; i += num_tasks_each_slice)
    {
        int assignedIdx = rand() % m_num_threads; 
        TaskDescription taskDes{i, i + num_tasks_each_slice, runnable, num_total_tasks};
        m_workQuque[assignedIdx].InQueueTask(taskDes);
        // printf("assign task %d to threadid %d\n",i, assignedIdx );
    }


    DoWork(m_num_threads - 1, m_workQuque[m_num_threads - 1], m_endSig);

    std::unique_lock<std::mutex> endLock(m_endSig.m_endSignalLock);
    m_endSig.m_endSignalCV.wait(endLock, [this](){
        return std::all_of(m_workQuque.begin(), m_workQuque.end(),[](WorkQueue& queue){
            return queue.IsEmpty();
        } );
    });
    endLock.unlock();
}
~~~

Test result is following, almost all test failed. Maybe because the large amount of lock operaion in WorkingQueue?

~~~md
 python3  ../tests/run_test_harness.py
runtasks_ref
Linux x86_64
================================================================================
Running task system grading harness... (11 total tests)
  - Detected CPU with 8 execution contexts
  - Task system configured to use at most 8 threads
================================================================================
================================================================================
Executing test: super_super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                4.626     5.195       0.89  (OK)
[Parallel + Always Spawn]               88.62     98.789      0.90  (OK)
[Parallel + Thread Pool + Spin]         21.941    20.077      1.09  (OK)
[Parallel + Thread Pool + Sleep]        5.666     39.366      0.14  (OK)
================================================================================
Executing test: super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                55.161    69.942      0.79  (OK)
[Parallel + Always Spawn]               102.636   110.667     0.93  (OK)
[Parallel + Thread Pool + Spin]         30.482    27.627      1.10  (OK)
[Parallel + Thread Pool + Sleep]        55.769    44.315      1.26  (NOT OK)
================================================================================
Executing test: ping_pong_equal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_equal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                911.461   1155.122    0.79  (OK)
[Parallel + Always Spawn]               288.446   294.777     0.98  (OK)
[Parallel + Thread Pool + Spin]         316.979   254.542     1.25  (NOT OK)
[Parallel + Thread Pool + Sleep]        925.038   268.558     3.44  (NOT OK)
================================================================================
Executing test: ping_pong_unequal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1687.013  1688.007    1.00  (OK)
[Parallel + Always Spawn]               459.726   389.333     1.18  (OK)
[Parallel + Thread Pool + Spin]         573.094   387.13      1.48  (NOT OK)
[Parallel + Thread Pool + Sleep]        1696.864  365.723     4.64  (NOT OK)
================================================================================
Executing test: recursive_fibonacci...
Reference binary: ./runtasks_ref_linux
Results for: recursive_fibonacci
                                        STUDENT   REFERENCE   PERF?
[Serial]                                845.118   1400.835    0.60  (OK)
[Parallel + Always Spawn]               210.901   239.04      0.88  (OK)
[Parallel + Thread Pool + Spin]         275.266   273.623     1.01  (OK)
[Parallel + Thread Pool + Sleep]        839.822   244.852     3.43  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop
                                        STUDENT   REFERENCE   PERF?
[Serial]                                526.0     534.113     0.98  (OK)
[Parallel + Always Spawn]               576.317   567.272     1.02  (OK)
[Parallel + Thread Pool + Spin]         258.701   183.512     1.41  (NOT OK)
[Parallel + Thread Pool + Sleep]        526.22    280.824     1.87  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fewer_tasks...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fewer_tasks
                                        STUDENT   REFERENCE   PERF?
[Serial]                                522.954   538.104     0.97  (OK)
[Parallel + Always Spawn]               547.11    549.521     1.00  (OK)
[Parallel + Thread Pool + Spin]         291.882   196.669     1.48  (NOT OK)
[Parallel + Thread Pool + Sleep]        521.979   303.663     1.72  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fan_in...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fan_in
                                        STUDENT   REFERENCE   PERF?
[Serial]                                271.081   272.815     0.99  (OK)
[Parallel + Always Spawn]               108.73    100.251     1.08  (OK)
[Parallel + Thread Pool + Spin]         94.163    68.38       1.38  (NOT OK)
[Parallel + Thread Pool + Sleep]        268.428   75.861      3.54  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_reduction_tree...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_reduction_tree
                                        STUDENT   REFERENCE   PERF?
[Serial]                                267.119   274.54      0.97  (OK)
[Parallel + Always Spawn]               71.238    63.902      1.11  (OK)
[Parallel + Thread Pool + Spin]         82.526    59.14       1.40  (NOT OK)
[Parallel + Thread Pool + Sleep]        266.837   58.191      4.59  (NOT OK)
================================================================================
Executing test: spin_between_run_calls...
Reference binary: ./runtasks_ref_linux
Results for: spin_between_run_calls
                                        STUDENT   REFERENCE   PERF?
[Serial]                                293.447   495.226     0.59  (OK)
[Parallel + Always Spawn]               152.086   251.578     0.60  (OK)
[Parallel + Thread Pool + Spin]         200.675   301.885     0.66  (OK)
[Parallel + Thread Pool + Sleep]        295.279   254.779     1.16  (OK)
================================================================================
Executing test: mandelbrot_chunked...
Reference binary: ./runtasks_ref_linux
Results for: mandelbrot_chunked
                                        STUDENT   REFERENCE   PERF?
[Serial]                                428.6     428.666     1.00  (OK)
[Parallel + Always Spawn]               58.29     58.407      1.00  (OK)
[Parallel + Thread Pool + Spin]         87.026    62.225      1.40  (NOT OK)
[Parallel + Thread Pool + Sleep]        425.884   58.913      7.23  (NOT OK)
================================================================================
Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : All passed Perf
[Parallel + Thread Pool + Spin]         : Perf did not pass all tests
[Parallel + Thread Pool + Sleep]        : Perf did not pass all tests
~~~

refering to this implementation : https://github.com/PKUFlyingPig/asst2/blob/master/part_a/tasksys.cpp
it is actually better than my implementation. Why? Explore it later.

## implementB
inspired by this repo https://github.com/PKUFlyingPig/asst2/blob/master/part_a/tasksys.cpp
implementation :
~~~c++
void TaskSystemParallelThreadPoolSpinningWorker(int threadidx, TaskSystemParallelThreadPoolSpinning* taskSysSpiningPool)
{
    TaskState& state = taskSysSpiningPool->m_taskState;
    // printf("threadidx %d enter SpiningWorker\n", threadidx);
    while (true)
    {
        if (taskSysSpiningPool->m_killed)
        {
            break;
        } 
        state.m_lockWorking.lock(); 
        if (state.m_currentIdx != -1 && state.m_currentIdx < state.m_total_num)
        {
            int runID = state.m_currentIdx++;
            int total = state.m_total_num;
            IRunnable* runable = state.m_runable;
            state.m_lockWorking.unlock(); 
                // printf("threaidx %d should run taskid %d total %d\n",threadidx, runID, total);
                runable->runTask(runID, total);


            state.m_lockFinished.lock(); 
            state.m_left_num--;
            if (state.m_left_num == 0)
            {
                // printf("threaidx %d run taskid %d done, notify main thread\n",threadidx, runID);
                state.m_cv_finished.notify_all();
            }
            state.m_lockFinished.unlock(); 
        }
        else 
        {
            state.m_lockWorking.unlock();
            continue;
        }

    }
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): 
    ITaskSystem(num_threads), 
    m_killed(false),
    m_taskState(),
    m_threads(num_threads - 1), 
    m_num_threads(num_threads)
{
    m_taskState.m_runable = nullptr;
    m_taskState.m_total_num = 0;
    m_taskState.m_left_num = 0;
    m_taskState.m_currentIdx = -1;
    for (size_t i = 0; i < m_threads.size(); i++)
    {
        m_threads[i] = std::thread(TaskSystemParallelThreadPoolSpinningWorker, i, this);
    }
    // printf("new tasksysem done\n");
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() 
{
    m_killed = true;
    for (int i = 0; i < m_num_threads - 1; i++) {
        m_threads[i].join();
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) 
{
    // printf("num_total_tasks %d\n", num_total_tasks);
    std::unique_lock<std::mutex> finish(m_taskState.m_lockFinished);
    // std::unique_lock<std::mutex> working(m_taskState.m_lockWorking);
    m_taskState.m_lockWorking.lock();
    m_taskState.m_total_num = num_total_tasks;
    m_taskState.m_left_num = num_total_tasks;
    m_taskState.m_currentIdx = 0;
    m_taskState.m_runable = runnable;
    m_taskState.m_lockWorking.unlock();
    // printf("main thread wait for done \n");
    m_taskState.m_cv_finished.wait(finish);

    m_taskState.m_lockWorking.lock();
    m_taskState.m_currentIdx = -1;
    m_taskState.m_lockWorking.unlock();
    // printf("all tasks done\n");
}
~~~

test result:
~~~md
➜  part_a git:(main) ✗ python3  ../tests/run_test_harness.py
runtasks_ref
Linux x86_64
================================================================================
Running task system grading harness... (11 total tests)
  - Detected CPU with 8 execution contexts
  - Task system configured to use at most 8 threads
================================================================================
================================================================================
Executing test: super_super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                5.15      5.205       0.99  (OK)
[Parallel + Always Spawn]               91.204    108.639     0.84  (OK)
[Parallel + Thread Pool + Spin]         22.379    22.544      0.99  (OK)
[Parallel + Thread Pool + Sleep]        6.142     42.11       0.15  (OK)
================================================================================
Executing test: super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                64.969    69.286      0.94  (OK)
[Parallel + Always Spawn]               114.468   108.206     1.06  (OK)
[Parallel + Thread Pool + Spin]         27.487    30.369      0.91  (OK)
[Parallel + Thread Pool + Sleep]        64.077    46.071      1.39  (NOT OK)
================================================================================
Executing test: ping_pong_equal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_equal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1022.126  1164.13     0.88  (OK)
[Parallel + Always Spawn]               317.68    321.439     0.99  (OK)
[Parallel + Thread Pool + Spin]         244.478   289.63      0.84  (OK)
[Parallel + Thread Pool + Sleep]        1022.964  284.932     3.59  (NOT OK)
================================================================================
Executing test: ping_pong_unequal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1699.613  1693.239    1.00  (OK)
[Parallel + Always Spawn]               468.578   402.931     1.16  (OK)
[Parallel + Thread Pool + Spin]         377.967   390.681     0.97  (OK)
[Parallel + Thread Pool + Sleep]        1749.709  369.828     4.73  (NOT OK)
================================================================================
Executing test: recursive_fibonacci...
Reference binary: ./runtasks_ref_linux
Results for: recursive_fibonacci
                                        STUDENT   REFERENCE   PERF?
[Serial]                                839.509   1386.833    0.61  (OK)
[Parallel + Always Spawn]               211.598   242.379     0.87  (OK)
[Parallel + Thread Pool + Spin]         175.467   271.435     0.65  (OK)
[Parallel + Thread Pool + Sleep]        858.346   244.796     3.51  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop
                                        STUDENT   REFERENCE   PERF?
[Serial]                                553.58    536.73      1.03  (OK)
[Parallel + Always Spawn]               592.998   564.173     1.05  (OK)
[Parallel + Thread Pool + Spin]         201.631   177.516     1.14  (OK)
[Parallel + Thread Pool + Sleep]        531.042   273.021     1.95  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fewer_tasks...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fewer_tasks
                                        STUDENT   REFERENCE   PERF?
[Serial]                                539.167   525.56      1.03  (OK)
[Parallel + Always Spawn]               568.09    553.881     1.03  (OK)
[Parallel + Thread Pool + Spin]         204.021   185.943     1.10  (OK)
[Parallel + Thread Pool + Sleep]        530.833   298.679     1.78  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fan_in...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fan_in
                                        STUDENT   REFERENCE   PERF?
[Serial]                                270.24    270.708     1.00  (OK)
[Parallel + Always Spawn]               108.806   98.089      1.11  (OK)
[Parallel + Thread Pool + Spin]         66.077    65.163      1.01  (OK)
[Parallel + Thread Pool + Sleep]        284.076   73.073      3.89  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_reduction_tree...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_reduction_tree
                                        STUDENT   REFERENCE   PERF?
[Serial]                                283.468   272.352     1.04  (OK)
[Parallel + Always Spawn]               76.055    65.805      1.16  (OK)
[Parallel + Thread Pool + Spin]         57.008    58.831      0.97  (OK)
[Parallel + Thread Pool + Sleep]        267.219   60.653      4.41  (NOT OK)
================================================================================
Executing test: spin_between_run_calls...
Reference binary: ./runtasks_ref_linux
Results for: spin_between_run_calls
                                        STUDENT   REFERENCE   PERF?
[Serial]                                293.056   493.734     0.59  (OK)
[Parallel + Always Spawn]               151.546   254.727     0.59  (OK)
[Parallel + Thread Pool + Spin]         204.947   307.484     0.67  (OK)
[Parallel + Thread Pool + Sleep]        305.546   248.785     1.23  (NOT OK)
================================================================================
Executing test: mandelbrot_chunked...
Reference binary: ./runtasks_ref_linux
Results for: mandelbrot_chunked
                                        STUDENT   REFERENCE   PERF?
[Serial]                                429.222   430.328     1.00  (OK)
[Parallel + Always Spawn]               62.807    58.165      1.08  (OK)
[Parallel + Thread Pool + Spin]         71.214    63.041      1.13  (OK)
[Parallel + Thread Pool + Sleep]        428.413   58.695      7.30  (NOT OK)
================================================================================
Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : All passed Perf
[Parallel + Thread Pool + Spin]         : All passed Perf
[Parallel + Thread Pool + Sleep]        : Perf did not pass all tests
~~~

Obviously, implementB is faster than implementA. But why is that since implementA has less lock operation?