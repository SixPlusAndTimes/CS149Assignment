#include "tasksys.h"
#include <stdio.h>
#include <random>
#include <functional>
#include <algorithm>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() { }

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {
}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}



TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): 
    ITaskSystem(num_threads), 
    m_threads(num_threads - 1), 
    m_num_threads(num_threads)
{
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {

}

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
    int multipler = 8;
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

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

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

void TaskSystemParallelThreadPoolSpinningWorker(int index, WorkQueue& workQueue, EndSignal& endSignal, bool& killed, StartRunSignal& startRun)
{
    while (true)
    {
        if (killed)
        {
            break;
        } 

        {
            startRun.m_startRunSignalLock.lock();
            // std::lock_guard<std::mutex> lockGuard(startRun.m_startRunSignalLock);
            if (startRun.m_startRun == false)
            {
                startRun.m_startRunSignalLock.unlock();
                continue;
            }
            // printf("start to run threadid %d \n ", index);
            startRun.m_startRunSignalLock.unlock();
        }

        // if (index == 1)
        // {
            // printf("start to run threadid %d \n", index);
        // }
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
        m_threads[i] = std::thread(TaskSystemParallelThreadPoolSpinningWorker, static_cast<int>(i), std::ref(m_workQuque[i]), std::ref(m_endSig), std::ref(m_killed), std::ref(m_startRunSignal));
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    m_killed = true;
    for (int i = 0; i < m_num_threads - 1; i++) {
        m_threads[i].join();
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {

    int multipler = 16;
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

    // printf("main thread dispatched all tasks\n");

        m_startRunSignal.m_startRunSignalLock.lock();
        // std::lock_guard<std::mutex> lockGuard(startRun.m_startRunSignalLock);
        m_startRunSignal.m_startRun = true;
        m_startRunSignal.m_startRunSignalLock.unlock();
        // printf("all thread start to run\b");

    DoWork(m_num_threads - 1, m_workQuque[m_num_threads - 1], m_endSig);

    std::unique_lock<std::mutex> endLock(m_endSig.m_endSignalLock);
    m_endSig.m_endSignalCV.wait(endLock, [this](){
        return std::all_of(m_workQuque.begin(), m_workQuque.end(),[](WorkQueue& queue){
            return queue.IsEmpty();
        } );
    });
    endLock.unlock();

    {
        m_startRunSignal.m_startRunSignalLock.lock();
        m_startRunSignal.m_startRun = false;
        m_startRunSignal.m_startRunSignalLock.unlock();
    }
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

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
