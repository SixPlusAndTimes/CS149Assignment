#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include "atomic"
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <set>

#include <memory>
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
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

class TaskDesc
{
    public:
        IRunnable* runnable;
        int num_total_tasks;
        TaskID taskId;
        int task_not_done_num; // decreased from num_total_tasks
        std::vector<TaskID> deps;
        int deped_has_not_been_done_num; // Only when all dependent tasks are completed can this task moved into workqueue
        std::set<TaskID> sufs; // Tasks that depend on this task
        bool has_been_done; // indicate whether this task hasbeen done
                                // may be duplicated in function with task_not_done_num
        std::mutex taskDesLck;
    TaskDesc(): runnable(nullptr), num_total_tasks(0), taskId(-1), task_not_done_num(-1) {}
    TaskDesc(IRunnable* r, int num_total_tasks, TaskID taskId, const std::vector<TaskID>& indeps):
            runnable(r), num_total_tasks(num_total_tasks), taskId(taskId), 
            task_not_done_num(num_total_tasks), deps(indeps), deped_has_not_been_done_num(0), sufs(), has_been_done(false), taskDesLck() {}
};

class TaskSliceDesc
{
    public:
        int assign_from;
        int assign_to;
        IRunnable* runnable;
        int num_total_tasks;
        TaskID taskid_belongs_to;
        TaskSliceDesc() : assign_from(0), assign_to(0), runnable(nullptr), num_total_tasks(0), taskid_belongs_to(-1) {}
        TaskSliceDesc(int from, int to, IRunnable* r, int total, TaskID taskIdBelongsTo): 
                assign_from(from), assign_to(to), runnable(r), num_total_tasks(total), taskid_belongs_to(taskIdBelongsTo) {}
        
};

class WorkQueue
{
public:
    WorkQueue() = default;
    void InQueueTask(TaskSliceDesc& taskdes)
    {
        std::lock_guard<std::mutex> lockGuard(m_lock);
        m_queue.push(taskdes);
    }

    bool DeQueueTask(TaskSliceDesc& out)
    {
        std::lock_guard<std::mutex> lockGuard(m_lock);
        if (m_queue.empty()) return false;
        out = m_queue.front();
        m_queue.pop();
        return true;
    }

    TaskSliceDesc& Front()
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
    std::queue<TaskSliceDesc> m_queue;
    std::mutex m_lock;
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
                std::atomic<int> m_RemainingTask;
                std::atomic<bool> m_killed;

                std::mutex  m_hasTaksLk;
                std::condition_variable m_hasTaksCv;

                std::mutex m_allTasksDoneLk;
                std::condition_variable m_allTasksDoneCv;
        };
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void SliceAndMoveTheTaskToWorkQueue(const TaskDesc& taskDes);
        void sync();

        void TaskSystemParallelThreadPoolSleepingWorker(int index);
        bool GetWorkFromWaitingQueue(int index);

        TaskID m_nextTaskId;

        std::vector<std::thread> m_threads;
        int m_num_threads;

        TaskState m_taskState;
        
        std::vector<WorkQueue>  m_workQuque; // task slices that are ready to exec and are distributed to m_num_threads
        std::vector<TaskDesc*>   m_waitingQueue;
        std::mutex              m_waitingQueueLck;

        std::unordered_map<TaskID, std::unique_ptr<TaskDesc>> m_taskRecords;


};

#endif
