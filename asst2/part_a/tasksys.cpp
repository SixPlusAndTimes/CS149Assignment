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
