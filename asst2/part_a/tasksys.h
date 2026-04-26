#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <thread>
#include <queue>
#include <condition_variable>
#include <shared_mutex>
#include <atomic>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */


class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
    private:
        std::vector<std::thread> m_threads; // m_threads.size() + 1 = m_num_threads;
        int m_num_threads;

};


/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
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


class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        class TaskState
        {
            public:
            std::atomic<bool> m_killed;
            std::atomic<int> m_RemainingTask;
        };
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

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        class TaskState
        {
            public:
            std::atomic<bool> m_killed;
            std::atomic<int> m_RemainingTask;
            std::mutex  m_hasTaksLk;
            std::condition_variable m_hasTaksCv;
        };
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
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

#endif
