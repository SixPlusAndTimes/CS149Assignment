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
    int multipler = 16;
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

    while (m_taskState.m_RemainingTask.load(std::memory_order_acquire) > 0)
    {
        DoWork(m_num_threads - 1, this);
    }
    // printf("all tasks done return run()\n");
    
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
