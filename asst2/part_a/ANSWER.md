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

## optimized implA
ImplA uses static dispatch strategy which may lead to loading unbalanced. So in this implementation I try to dynamically dispatch workload.

~~~c++
class TaskDescription
{
public:
    int assign_from;
    int assign_to;
    IRunnable* runnable;
    int num_total_tasks;
    TaskDescription() : assign_from(0), assign_to(0), runnable(nullptr), num_total_tasks(0) {}
    TaskDescription(int from, int to, IRunnable* r, int total): 
            assign_from(from), assign_to(to), runnable(r), num_total_tasks(total) {}
};

class WorkQueue
{
public:
    WorkQueue() = default;
    void InQueueTask(TaskDescription& taskdes)
    {
        std::lock_guard<std::mutex> lockGuard(m_lock);
        m_queue.push(taskdes);
    }

    bool DeQueueTask(TaskDescription& out)
    {
        std::lock_guard<std::mutex> lockGuard(m_lock);
        if (m_queue.empty()) return false;
        out = m_queue.front();
        m_queue.pop();
        return true;
    }

    TaskDescription& Front()
    {
        std::lock_guard<std::mutex> lockGuard(m_lock);
        return m_queue.front();
    }

    bool IsEmpty() 
    {
        std::lock_guard<std::mutex> lockGuard(m_lock);
        return m_queue.empty();
    }

private:
    std::queue<TaskDescription> m_queue;
    std::mutex m_lock;
};

class TaskState
{
public:
    std::atomic<bool> m_isStartProcess;
    std::atomic<bool> m_killed;
    std::atomic<int> m_RemainingTask;
};

class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        TaskState m_taskState;
        std::vector<WorkQueue> m_workQuque;
    private:
        std::vector<std::thread> m_threads; // m_threads.size() + 1 = m_num_threads;
        int m_num_threads;
        
};

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

void DoWork(int index, TaskSystemParallelThreadPoolSpinning* taskSystem)
{
    TaskDescription taskDes;
    bool doSelf = taskSystem->m_workQuque[index].DeQueueTask(taskDes);
    if (!doSelf)
    {
        // steal from others
        for (int stolenIdx = (index + 1) % (taskSystem->m_workQuque.size()); 
            stolenIdx != index; 
            stolenIdx = (stolenIdx + 1) % (taskSystem->m_workQuque.size()))
        {
            if (taskSystem->m_workQuque[stolenIdx].DeQueueTask(taskDes))
            {
                // printf("thread idx %d steal from %d [%d %d)\n", index, stolenIdx, taskDes.assign_from, taskDes.assign_to);
                break;
            }
        }
    }

    if (taskDes.runnable)
    {
        for (int i = taskDes.assign_from; i < taskDes.assign_to; i++)
        {
            // printf("thread idx %d task [%d, %d) starttorun\n", index, taskDes.assign_from, taskDes.assign_to);
            taskDes.runnable->runTask(i, taskDes.num_total_tasks);
        }
        int doneNum = taskDes.assign_to - taskDes.assign_from;
        taskSystem->m_taskState.m_RemainingTask.fetch_sub(doneNum, std::memory_order_seq_cst);
        // printf("thread idx %d task [%d, %d) completed\n", index, taskDes.assign_from, taskDes.assign_to);
    }

}

void TaskSystemParallelThreadPoolSpinningWorker(int index, TaskSystemParallelThreadPoolSpinning* taskSystem)
{
    // printf("threadidx %d enter SpiningWorker\n", threadidx);
    while (true)
    {
        if (taskSystem->m_taskState.m_killed.load(std::memory_order_acquire))
        {
            break;
        } 

        if (!taskSystem->m_taskState.m_isStartProcess.load(std::memory_order_acquire))
        {
            // printf("thread idx %d, nothing to do continue iteration\n", index);
            continue;
        }

        DoWork(index, taskSystem);
    }
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): 
    ITaskSystem(num_threads), 
    m_taskState(),
    m_workQuque(num_threads),
    m_threads(num_threads - 1), 
    m_num_threads(num_threads)
{
    // printf("parallel thread pool spining start init\n");
    m_taskState.m_killed.store(false, std::memory_order_release);
    m_taskState.m_isStartProcess.store(false, std::memory_order_release);
    m_taskState.m_RemainingTask.store(0, std::memory_order_release);
    for (size_t i = 0; i < m_threads.size(); i++)
    {
        m_threads[i] = std::thread(TaskSystemParallelThreadPoolSpinningWorker, static_cast<int>(i), this);
    }
    // printf("new tasksysem done\n");
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    // printf("teminate spining pool threads start\n");
    m_taskState.m_killed.store(true, std::memory_order_release);
    for (int i = 0; i < m_num_threads - 1; i++) {
        m_threads[i].join();
    }
    // printf("teminate spining pool threads done\n");
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) 
{

    // printf("num_total_tasks %d \n",num_total_tasks);
    int multipler = 8;
    int num_slice = m_num_threads * multipler;
    int num_tasks_each_slice = num_total_tasks / (num_slice);
    if (num_tasks_each_slice <= 0)
    {
        num_tasks_each_slice = 1;
    }

    m_taskState.m_RemainingTask.store(num_total_tasks, std::memory_order_release);
    int assignedIdx = 0; 
    for (int i = 0; i < num_total_tasks; i += num_tasks_each_slice)
    {
        TaskDescription taskDes{i, i + num_tasks_each_slice, runnable, num_total_tasks};
        if (i + num_tasks_each_slice > num_total_tasks)
        {
            taskDes.assign_to = num_total_tasks;
        }
        // printf("assign task[%d-%d) to thread idx %d\n",taskDes.assign_from, taskDes.assign_to, assignedIdx);
        m_workQuque[assignedIdx].InQueueTask(taskDes);
        assignedIdx = (assignedIdx + 1) % m_workQuque.size();
    }

    // printf("main thread dispatched all tasks\n");

    m_taskState.m_isStartProcess.store(true, std::memory_order_release);

    while (m_taskState.m_RemainingTask.load(std::memory_order_acquire) > 0)
    {
        DoWork(m_num_threads - 1, this);
    }
    // printf("all tasks done return run()\n");
    m_taskState.m_isStartProcess.store(false, std::memory_order_release);
    
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}
~~~

huge accelleration for "Parallel + Thread Pool + Spin", but I dont konw why here is a fallback in "Parallel + Always Spawn".

~~~md

➜  part_a git:(1e2f94e) ✗ python3  ../tests/run_test_harness.py
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
[Serial]                                5.006     5.187       0.97  (OK)
[Parallel + Always Spawn]               97.306    102.463     0.95  (OK)
[Parallel + Thread Pool + Spin]         8.002     21.53       0.37  (OK)
[Parallel + Thread Pool + Sleep]        6.213     40.86       0.15  (OK)
================================================================================
Executing test: super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                71.845    69.565      1.03  (OK)
[Parallel + Always Spawn]               112.596   111.178     1.01  (OK)
[Parallel + Thread Pool + Spin]         18.531    27.292      0.68  (OK)
[Parallel + Thread Pool + Sleep]        73.138    43.498      1.68  (NOT OK)
================================================================================
Executing test: ping_pong_equal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_equal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1195.748  1151.752    1.04  (OK)
[Parallel + Always Spawn]               384.008   296.967     1.29  (NOT OK)
[Parallel + Thread Pool + Spin]         249.469   274.006     0.91  (OK)
[Parallel + Thread Pool + Sleep]        1173.379  271.646     4.32  (NOT OK)
================================================================================
Executing test: ping_pong_unequal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1762.749  1657.21     1.06  (OK)
[Parallel + Always Spawn]               484.854   359.198     1.35  (NOT OK)
[Parallel + Thread Pool + Spin]         335.401   363.92      0.92  (OK)
[Parallel + Thread Pool + Sleep]        1755.34   349.31      5.03  (NOT OK)
================================================================================
Executing test: recursive_fibonacci...
Reference binary: ./runtasks_ref_linux
Results for: recursive_fibonacci
                                        STUDENT   REFERENCE   PERF?
[Serial]                                876.068   1366.185    0.64  (OK)
[Parallel + Always Spawn]               220.921   230.853     0.96  (OK)
[Parallel + Thread Pool + Spin]         183.332   262.989     0.70  (OK)
[Parallel + Thread Pool + Sleep]        881.119   233.009     3.78  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop
                                        STUDENT   REFERENCE   PERF?
[Serial]                                543.16    515.815     1.05  (OK)
[Parallel + Always Spawn]               585.138   517.632     1.13  (OK)
[Parallel + Thread Pool + Spin]         142.286   166.244     0.86  (OK)
[Parallel + Thread Pool + Sleep]        550.716   266.873     2.06  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fewer_tasks...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fewer_tasks
                                        STUDENT   REFERENCE   PERF?
[Serial]                                537.283   513.444     1.05  (OK)
[Parallel + Always Spawn]               561.696   526.554     1.07  (OK)
[Parallel + Thread Pool + Spin]         143.185   165.471     0.87  (OK)
[Parallel + Thread Pool + Sleep]        549.112   291.888     1.88  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fan_in...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fan_in
                                        STUDENT   REFERENCE   PERF?
[Serial]                                284.777   263.85      1.08  (OK)
[Parallel + Always Spawn]               117.062   97.337      1.20  (NOT OK)
[Parallel + Thread Pool + Spin]         55.904    65.64       0.85  (OK)
[Parallel + Thread Pool + Sleep]        282.384   74.166      3.81  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_reduction_tree...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_reduction_tree
                                        STUDENT   REFERENCE   PERF?
[Serial]                                286.771   269.761     1.06  (OK)
[Parallel + Always Spawn]               76.015    61.822      1.23  (NOT OK)
[Parallel + Thread Pool + Spin]         53.262    56.815      0.94  (OK)
[Parallel + Thread Pool + Sleep]        286.082   55.429      5.16  (NOT OK)
================================================================================
Executing test: spin_between_run_calls...
Reference binary: ./runtasks_ref_linux
Results for: spin_between_run_calls
                                        STUDENT   REFERENCE   PERF?
[Serial]                                316.52    490.804     0.64  (OK)
[Parallel + Always Spawn]               165.203   260.956     0.63  (OK)
[Parallel + Thread Pool + Spin]         199.325   307.426     0.65  (OK)
[Parallel + Thread Pool + Sleep]        316.432   250.606     1.26  (NOT OK)
================================================================================
Executing test: mandelbrot_chunked...
Reference binary: ./runtasks_ref_linux
Results for: mandelbrot_chunked
                                        STUDENT   REFERENCE   PERF?
[Serial]                                464.575   430.473     1.08  (OK)
[Parallel + Always Spawn]               64.052    58.283      1.10  (OK)
[Parallel + Thread Pool + Spin]         62.949    66.267      0.95  (OK)
[Parallel + Thread Pool + Sleep]        464.138   58.403      7.95  (NOT OK)
================================================================================
Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : Perf did not pass all tests
[Parallel + Thread Pool + Spin]         : All passed Perf
[Parallel + Thread Pool + Sleep]        : Perf did not pass all tests
➜  part_a git:(1e2f94e) ✗ python3  ../tests/run_test_harness.py
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
[Serial]                                4.986     5.123       0.97  (OK)
[Parallel + Always Spawn]               93.816    98.213      0.96  (OK)
[Parallel + Thread Pool + Spin]         7.78      20.462      0.38  (OK)
[Parallel + Thread Pool + Sleep]        6.044     39.762      0.15  (OK)
================================================================================
Executing test: super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                68.975    65.886      1.05  (OK)
[Parallel + Always Spawn]               109.861   106.235     1.03  (OK)
[Parallel + Thread Pool + Spin]         16.959    26.088      0.65  (OK)
[Parallel + Thread Pool + Sleep]        68.604    42.398      1.62  (NOT OK)
================================================================================
Executing test: ping_pong_equal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_equal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1141.604  1108.157    1.03  (OK)
[Parallel + Always Spawn]               378.104   279.022     1.36  (NOT OK)
[Parallel + Thread Pool + Spin]         224.997   250.466     0.90  (OK)
[Parallel + Thread Pool + Sleep]        1159.741  256.131     4.53  (NOT OK)
================================================================================
Executing test: ping_pong_unequal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1729.797  1616.58     1.07  (OK)
[Parallel + Always Spawn]               483.397   361.304     1.34  (NOT OK)
[Parallel + Thread Pool + Spin]         327.356   352.805     0.93  (OK)
[Parallel + Thread Pool + Sleep]        1726.935  339.274     5.09  (NOT OK)
================================================================================
Executing test: recursive_fibonacci...
Reference binary: ./runtasks_ref_linux
Results for: recursive_fibonacci
                                        STUDENT   REFERENCE   PERF?
[Serial]                                891.962   1375.227    0.65  (OK)
[Parallel + Always Spawn]               222.234   229.73      0.97  (OK)
[Parallel + Thread Pool + Spin]         179.452   262.585     0.68  (OK)
[Parallel + Thread Pool + Sleep]        901.417   235.411     3.83  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop
                                        STUDENT   REFERENCE   PERF?
[Serial]                                558.719   529.049     1.06  (OK)
[Parallel + Always Spawn]               603.208   546.041     1.10  (OK)
[Parallel + Thread Pool + Spin]         151.725   167.224     0.91  (OK)
[Parallel + Thread Pool + Sleep]        557.125   272.796     2.04  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fewer_tasks...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fewer_tasks
                                        STUDENT   REFERENCE   PERF?
[Serial]                                565.391   533.472     1.06  (OK)
[Parallel + Always Spawn]               610.517   549.01      1.11  (OK)
[Parallel + Thread Pool + Spin]         156.346   192.209     0.81  (OK)
[Parallel + Thread Pool + Sleep]        565.097   308.024     1.83  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fan_in...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fan_in
                                        STUDENT   REFERENCE   PERF?
[Serial]                                289.396   277.662     1.04  (OK)
[Parallel + Always Spawn]               120.45    102.784     1.17  (OK)
[Parallel + Thread Pool + Spin]         63.955    71.695      0.89  (OK)
[Parallel + Thread Pool + Sleep]        291.937   78.453      3.72  (NOT OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_reduction_tree...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_reduction_tree
                                        STUDENT   REFERENCE   PERF?
[Serial]                                290.736   273.463     1.06  (OK)
[Parallel + Always Spawn]               78.409    66.076      1.19  (OK)
[Parallel + Thread Pool + Spin]         55.09     58.044      0.95  (OK)
[Parallel + Thread Pool + Sleep]        287.468   60.912      4.72  (NOT OK)
================================================================================
Executing test: spin_between_run_calls...
Reference binary: ./runtasks_ref_linux
Results for: spin_between_run_calls
                                        STUDENT   REFERENCE   PERF?
[Serial]                                318.365   494.725     0.64  (OK)
[Parallel + Always Spawn]               166.311   251.32      0.66  (OK)
[Parallel + Thread Pool + Spin]         216.17    304.106     0.71  (OK)
[Parallel + Thread Pool + Sleep]        319.964   250.433     1.28  (NOT OK)
================================================================================
Executing test: mandelbrot_chunked...
Reference binary: ./runtasks_ref_linux
Results for: mandelbrot_chunked
                                        STUDENT   REFERENCE   PERF?
[Serial]                                466.277   430.347     1.08  (OK)
[Parallel + Always Spawn]               64.173    59.33       1.08  (OK)
[Parallel + Thread Pool + Spin]         63.238    65.429      0.97  (OK)
[Parallel + Thread Pool + Sleep]        467.005   58.774      7.95  (NOT OK)
================================================================================
Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : Perf did not pass all tests
[Parallel + Thread Pool + Spin]         : All passed Perf
[Parallel + Thread Pool + Sleep]        : Perf did not pass all tests
~~~

# step3 
almost same with the spinning thread system, but notice the sleep&notify condition.
test results :
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
[Serial]                                4.741     5.181       0.92  (OK)
[Parallel + Always Spawn]               96.521    100.973     0.96  (OK)
[Parallel + Thread Pool + Spin]         9.903     21.298      0.46  (OK)
[Parallel + Thread Pool + Sleep]        12.803    40.706      0.31  (OK)
================================================================================
Executing test: super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                57.068    71.566      0.80  (OK)
[Parallel + Always Spawn]               103.961   115.745     0.90  (OK)
[Parallel + Thread Pool + Spin]         16.147    32.148      0.50  (OK)
[Parallel + Thread Pool + Sleep]        38.281    46.759      0.82  (OK)
================================================================================
Executing test: ping_pong_equal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_equal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                920.71    1144.672    0.80  (OK)
[Parallel + Always Spawn]               301.553   303.343     0.99  (OK)
[Parallel + Thread Pool + Spin]         176.233   286.603     0.61  (OK)
[Parallel + Thread Pool + Sleep]        217.495   278.346     0.78  (OK)
================================================================================
Executing test: ping_pong_unequal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1624.733  1648.29     0.99  (OK)
[Parallel + Always Spawn]               447.375   372.955     1.20  (OK)
[Parallel + Thread Pool + Spin]         309.572   352.861     0.88  (OK)
[Parallel + Thread Pool + Sleep]        329.168   347.518     0.95  (OK)
================================================================================
Executing test: recursive_fibonacci...
Reference binary: ./runtasks_ref_linux
Results for: recursive_fibonacci
                                        STUDENT   REFERENCE   PERF?
[Serial]                                836.679   1398.359    0.60  (OK)
[Parallel + Always Spawn]               206.66    243.529     0.85  (OK)
[Parallel + Thread Pool + Spin]         170.252   277.111     0.61  (OK)
[Parallel + Thread Pool + Sleep]        169.015   240.802     0.70  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop
                                        STUDENT   REFERENCE   PERF?
[Serial]                                521.631   532.443     0.98  (OK)
[Parallel + Always Spawn]               563.455   565.042     1.00  (OK)
[Parallel + Thread Pool + Spin]         135.124   184.551     0.73  (OK)
[Parallel + Thread Pool + Sleep]        318.094   277.388     1.15  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fewer_tasks...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fewer_tasks
                                        STUDENT   REFERENCE   PERF?
[Serial]                                512.513   536.055     0.96  (OK)
[Parallel + Always Spawn]               538.643   556.911     0.97  (OK)
[Parallel + Thread Pool + Spin]         145.882   182.343     0.80  (OK)
[Parallel + Thread Pool + Sleep]        330.761   303.919     1.09  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fan_in...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fan_in
                                        STUDENT   REFERENCE   PERF?
[Serial]                                266.169   272.67      0.98  (OK)
[Parallel + Always Spawn]               111.719   99.671      1.12  (OK)
[Parallel + Thread Pool + Spin]         55.819    68.822      0.81  (OK)
[Parallel + Thread Pool + Sleep]        77.386    74.641      1.04  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_reduction_tree...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_reduction_tree
                                        STUDENT   REFERENCE   PERF?
[Serial]                                266.028   272.301     0.98  (OK)
[Parallel + Always Spawn]               70.984    64.92       1.09  (OK)
[Parallel + Thread Pool + Spin]         48.673    60.246      0.81  (OK)
[Parallel + Thread Pool + Sleep]        52.923    58.824      0.90  (OK)
================================================================================
Executing test: spin_between_run_calls...
Reference binary: ./runtasks_ref_linux
Results for: spin_between_run_calls
                                        STUDENT   REFERENCE   PERF?
[Serial]                                290.88    497.455     0.58  (OK)
[Parallel + Always Spawn]               147.536   253.107     0.58  (OK)
[Parallel + Thread Pool + Spin]         196.596   303.8       0.65  (OK)
[Parallel + Thread Pool + Sleep]        151.046   253.046     0.60  (OK)
================================================================================
Executing test: mandelbrot_chunked...
Reference binary: ./runtasks_ref_linux
Results for: mandelbrot_chunked
                                        STUDENT   REFERENCE   PERF?
[Serial]                                423.278   428.494     0.99  (OK)
[Parallel + Always Spawn]               57.689    58.587      0.98  (OK)
[Parallel + Thread Pool + Spin]         57.328    66.761      0.86  (OK)
[Parallel + Thread Pool + Sleep]        57.154    58.292      0.98  (OK)
================================================================================
Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : All passed Perf
[Parallel + Thread Pool + Spin]         : All passed Perf
[Parallel + Thread Pool + Sleep]        : All passed Perf
~~~
