#include "tasksys.h"
#include <assert.h>
#include <iostream>
#include <sstream> 
// #define DEBUG
#ifdef DEBUG
#define debugprint(format, ...) printf(format, ##__VA_ARGS__)
#else
#define debugprint(format, ...)
#endif

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

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

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
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

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
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

bool DoWork(int index, TaskSystemParallelThreadPoolSleeping* taskSystem)
{
    TaskSliceDesc taskSliceDes;
    bool doSelf = taskSystem->m_workQuque[index].DeQueueTask(taskSliceDes);
    if (!doSelf)
    {
        // steal from others
        for (int stolenIdx = (index + 1) % (taskSystem->m_workQuque.size()); 
            stolenIdx != index; 
            stolenIdx = (stolenIdx + 1) % (taskSystem->m_workQuque.size()))
        {
            if (taskSystem->m_workQuque[stolenIdx].DeQueueTask(taskSliceDes))
            {
                debugprint("thread idx %d steal from %d [%d %d)\n", index, stolenIdx, taskSliceDes.assign_from, taskSliceDes.assign_to);
                break;
            }
        }
    }

    if (taskSliceDes.runnable)
    {
        for (int i = taskSliceDes.assign_from; i < taskSliceDes.assign_to; i++)
        {
            taskSliceDes.runnable->runTask(i, taskSliceDes.num_total_tasks);
        }
        int doneNum = taskSliceDes.assign_to - taskSliceDes.assign_from;

        taskSystem->m_taskRecordLck.lock();
        auto& taskDes = taskSystem->m_taskRecords.find(taskSliceDes.taskid_belongs_to)->second;
        taskSystem->m_taskRecordLck.unlock();
        // assert(taskDes.get() != nullptr);

        taskDes->taskDesLck.lock(); 
                    // it seems we do not need the lock operation, but it will lead to dead lock if we do not do the lock operation
                    // But I am not intereted in finding out the reason, just let it go (
        taskDes->task_not_done_num.fetch_sub(doneNum, std::memory_order_release);
        bool done = taskDes->task_not_done_num.load(std::memory_order_acquire) == 0;
        taskDes->taskDesLck.unlock();

        if (done)
        {
            for (auto taskId : taskDes->sufs)
            {
                taskSystem->m_taskRecordLck.lock();
                auto& taskSuf = taskSystem->m_taskRecords.find(taskId)->second;
                taskSystem->m_taskRecordLck.unlock();

                taskSuf->deped_has_not_been_done_num.fetch_sub(1, std::memory_order_release);
            }
            taskSystem->m_taskState.m_RemainingTask.fetch_sub(1, std::memory_order_release);
            if (taskSystem->m_taskState.m_RemainingTask.load(std::memory_order_acquire) == 0)
            {
                std::unique_lock<std::mutex> lc(taskSystem->m_taskState.m_allTasksDoneLk);
                taskSystem->m_taskState.m_allTasksDoneCv.notify_all();
                lc.unlock();
                debugprint("no remaining tasks notify, thread idx %d", index);
            }
            debugprint("threadid %d done 1 task, taskid %d, remainingTaksNum %d\n", index, taskDes->taskId, taskSystem->m_taskState.m_RemainingTask.load());
        }
        // debugprint("thread idx %d task [%d, %d) completed\n", index, taskSliceDes.assign_from, taskSliceDes.assign_to);
        return true;
    }
    return false;

}

void TaskSystemParallelThreadPoolSleeping::SliceAndMoveTheTaskToWorkQueue(const TaskDesc& taskDes)
{
    int multipler = 8;
    int num_slice = m_num_threads * multipler;
    int num_tasks_each_slice = taskDes.num_total_tasks / (num_slice);
    if (num_tasks_each_slice <= 0)
    {
        num_tasks_each_slice = 1;
    }

    int assignedIdx = 0; 
    for (int i = 0; i < taskDes.num_total_tasks; i += num_tasks_each_slice)
    {
        TaskSliceDesc tasksliceDes{i, i + num_tasks_each_slice, taskDes.runnable, taskDes.num_total_tasks, taskDes.taskId};
        if (i + num_tasks_each_slice > taskDes.num_total_tasks)
        {
            tasksliceDes.assign_to = taskDes.num_total_tasks;
        }
        // printf("assign task[%d-%d) to thread idx %d\n",tasksliceDes.assign_from, tasksliceDes.assign_to, assignedIdx);
        m_workQuque[assignedIdx].InQueueTask(tasksliceDes);
        assignedIdx = (assignedIdx + 1) % m_workQuque.size();

    }
}

bool TaskSystemParallelThreadPoolSleeping::GetWorkFromWaitingQueue(int index)
{

    bool isFInd = false;
    m_waitingQueueLck.lock();
    for (auto iter = m_waitingQueue.begin(); iter != m_waitingQueue.end(); ++iter)
    {
        TaskDesc* taskDesPtr = *iter;
        if (taskDesPtr->deped_has_not_been_done_num.load(std::memory_order_acquire) == 0)
        {
            SliceAndMoveTheTaskToWorkQueue(*taskDesPtr);
            isFInd = true;
        }
        if (isFInd) 
        { 
            m_waitingQueue.erase(iter); // notice the erase logical. erase it directly since we break the loop
            debugprint("thread idx %d, Get A workFromWaitingQueue, taskId%d, num_total_tasks %d deped_has_not_been_done_num %d\n", taskDesPtr->taskId, 
                        taskDesPtr->num_total_tasks, taskDesPtr->deped_has_not_been_done_num.load(std::memory_order_acquire), taskDesPtr->task_not_done_num.load());
            std::unique_lock<std::mutex> lc(m_taskState.m_hasTaksLk);
            m_taskState.m_hasTaksCv.notify_all();
            lc.unlock();
            break;
        }
    }
    m_waitingQueueLck.unlock();

    return isFInd;
}

void TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleepingWorker(int index)
{
    while (true)
    {
        if (m_taskState.m_killed.load(std::memory_order_acquire))
        {
            break;
        } 

        if (DoWork(index, this))
        {
            continue;
        }
        else if (GetWorkFromWaitingQueue(index))
        {
            continue;
        }
        else
        {
            std::unique_lock<std::mutex> lc(m_taskState.m_hasTaksLk);
            // avoid sleep/notify condition when destroying the thread system
            if (!m_taskState.m_killed.load(std::memory_order_acquire) && m_taskState.m_RemainingTask.load() == 0)
            {
                debugprint("threadidx %d wait for work\n", index);
                m_taskState.m_hasTaksCv.wait(lc);
            }
        }
    }
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads),
        m_nextTaskId(0), m_threads(num_threads), m_num_threads(num_threads), m_taskState(), m_workQuque(num_threads), m_waitingQueue()
{
    m_taskState.m_killed.store(false, std::memory_order_release);
    m_taskState.m_RemainingTask.store(0, std::memory_order_release);
    for (size_t i = 0; i < m_threads.size(); i++)
    {
        m_threads[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleepingWorker,this, static_cast<int>(i));
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    m_taskState.m_killed.store(true, std::memory_order_release);
    std::unique_lock<std::mutex> lc(m_taskState.m_hasTaksLk);
    m_taskState.m_hasTaksCv.notify_all();
    lc.unlock();
    for (int i = 0; i < m_num_threads; i++) {
        m_threads[i].join();
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) 
{
    TaskID curTaskId = m_nextTaskId;

    std::unique_ptr<TaskDesc> taskDesUptr(new TaskDesc(runnable, num_total_tasks, curTaskId, deps));
    std::stringstream depString;
    depString << "begin to add new task, new taskId " << curTaskId << ", deps[ ";
    for (TaskID taskIdDep : deps)
    {
        depString << taskIdDep << " ";
    }
    depString << "]\n";
    debugprint(depString.str().c_str());
    m_waitingQueueLck.lock();
    m_taskRecordLck.lock();
    for (TaskID taskeId : deps)
    {
        // assert(m_taskRecords.count(taskeId) != 0);
        auto& preTask = m_taskRecords[taskeId];
        preTask->taskDesLck.lock();
        if (preTask->task_not_done_num.load() != 0) 
        {
            taskDesUptr->deped_has_not_been_done_num.fetch_add(1, std::memory_order_release);
            preTask->sufs.insert(taskDesUptr->taskId);
        }
        preTask->taskDesLck.unlock();
    }
    m_taskRecords.emplace(taskDesUptr->taskId, std::move(taskDesUptr));
    m_taskRecordLck.unlock();
    m_waitingQueue.push_back(m_taskRecords.find(curTaskId)->second.get());
    m_waitingQueueLck.unlock();


    m_taskState.m_RemainingTask.fetch_add(1, std::memory_order_release);
    debugprint("remainiTask Num is %d\n", m_taskState.m_RemainingTask.load());
    std::unique_lock<std::mutex> lc(m_taskState.m_hasTaksLk);
    m_taskState.m_hasTaksCv.notify_all();
    lc.unlock();
    return m_nextTaskId++;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    std::unique_lock<std::mutex> lc(m_taskState.m_allTasksDoneLk);
    debugprint("wait for all tasks done, remaing Task Num is %d\n", m_taskState.m_RemainingTask.load());
    m_taskState.m_allTasksDoneCv.wait(lc, [&]() {return m_taskState.m_RemainingTask.load() == 0;});
    debugprint("tasksystem: all tasks done\n");
    return;
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {

    runAsyncWithDeps(runnable, num_total_tasks, {});
    sync();
}