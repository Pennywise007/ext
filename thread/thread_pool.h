#pragma once

/*
 * Thread pool implementation, allow to add task for execution on already existed threads
 * Example:

#include <ext/thread/thread_pool.h>

std::set<ext::task::TaskId, ext::task::TaskIdComparer> taskList;
ext::thread_pool threadPool([&taskList, &listMutex](const ext::task::TaskId& taskId)
{
	taskList.erase(taskId);
});

const auto maxThreads = std::thread::hardware_concurrency();
for (auto i = maxThreads; i != 0; --i)
{
	taskList.emplace(threadPool.add_task([]()
	{
		...
	}));
}
threadPool.wait_for_tasks();

 */
#include <algorithm>
#include <future>
#include <mutex>
#include <stdint.h>
#include <thread>
#include <type_traits>
#include <queue>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/core/noncopyable.h>

#include <ext/thread/event.h>
#include <ext/thread/thread.h>

namespace ext {
namespace task {

// task identifier
typedef GUID TaskId;

struct TaskIdComparer
{
    // @see also IsEqualGUID
    bool operator()(const TaskId& Left, const TaskId& Right) const { return memcmp(&Left, &Right, sizeof(Right)) < 0;     }
};

} // namespace task

// thread pool for execution tasks
class thread_pool : ext::NonCopyable
{
public:
    /**
     * \param onTaskDone callback on execution task by id
     * \param threadsCount working threads count
     */
    thread_pool(std::function<void(const task::TaskId&)>&& onTaskDone = nullptr,
                std::uint_fast32_t threadsCount = std::thread::hardware_concurrency());

    // interrupt and join all existing threads
    ~thread_pool();

    // wait until task queue not empty
    void wait_for_tasks();

    // detach and clear all working threads list, after calling this function class will be in inconsistent state
    void detach_all();

    // task execution priority
    enum class TaskPriority
    {
        eHigh,
        eNormal
    };

    EXT_NODISCARD static thread_pool& GlobalInstance();

    /**
     * \brief add task function to queue
     * \param task execution function
     * \param priority  task execution priority
     * \return created task identifier
     */
    task::TaskId add_task(std::function<void()>&& task, const TaskPriority priority = TaskPriority::eNormal);

    /**
     * \brief Add task function to queue
     * \tparam Result function result
     * \tparam FunctionArgs list of function arguments
     * \tparam Args list of arguments passed to function
     * \param task execution function
     * \param args list of arguments passed to function
     * \param priority task execution priority
     * \return taskId of created task and future with task result
     */
    template <typename Result, typename... FunctionArgs, typename... Args>
    EXT_NODISCARD std::pair<task::TaskId, std::future<Result>> add_task(Result(task)(FunctionArgs...),
                                                                        const TaskPriority priority = TaskPriority::eNormal,
                                                                        Args&&... args);

    // remove task from queue by id
    void erase_task(const task::TaskId& taskId);

private:
    // main thread for workers
    void worker();

private:
    // struct with task information
    struct TaskInfo;
    typedef std::unique_ptr<TaskInfo> TaskInfoPtr;

    // synchronization of m_queueTasks
    std::mutex m_mutex;
    std::condition_variable m_notifier;
    // queue of tasks ordered by TaskPriority
    std::list<TaskInfoPtr> m_queueTasks;

    // event about task done
    ext::Event m_taskDoneEvent;
    // count executing tasks at that moment threads
    std::atomic_uint m_countExecutingTasksThread = 0;
    // callback to callers about task done
    const std::function<void(const task::TaskId&)> m_onTaskDone;

    // list of worker threads
    std::list<ext::thread> m_threads;
};

// struct with task information
struct thread_pool::TaskInfo : ext::NonCopyable
{
    TaskInfo(std::function<void()>&& _task, const TaskPriority _priority)
        : task(std::move(_task)), priority(_priority)
    {
        EXT_EXPECT(SUCCEEDED(CoCreateGuid(&taskId))) << "Failed to create GUID";
    }

    std::function<void()> task;
    TaskPriority priority;
    task::TaskId taskId;
};

inline thread_pool& thread_pool::GlobalInstance()
{
    static thread_pool globalThreadPool;
    return globalThreadPool;
}

inline task::TaskId thread_pool::add_task(std::function<void()>&& task, const TaskPriority priority)
{
    task::TaskId newTaskId;
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        const auto priorityIt = std::find_if(m_queueTasks.cbegin(), m_queueTasks.cend(),
            [priority](const TaskInfoPtr& task)
            {
                return task->priority > priority;
            });

        newTaskId = m_queueTasks.emplace(priorityIt, std::make_unique<TaskInfo>(std::move(task), priority))->get()->taskId;
    }
    m_notifier.notify_one();
    return newTaskId;
}

template <typename Result, typename ... FunctionArgs, typename ... Args>
std::pair<task::TaskId, std::future<Result>> thread_pool::add_task(Result(task)(FunctionArgs...),
                                                                   const TaskPriority priority /*= TaskPriority::eNormal*/,
                                                                   Args&&... args)
{
    std::shared_ptr<std::promise<Result>> promise(new std::promise<Result>);
    std::future<Result> resultFuture = promise->get_future();
    const auto taskId = add_task([task, args..., promise]()
    {
        try
        {
            promise->set_value(task(args...));
        }
        catch (...)
        {
            try
            {
                promise->set_exception(std::current_exception());
            }
            catch (...)
            {
            }
        }
    }, priority);

    return std::make_pair(std::move(taskId), resultFuture);
}

inline void thread_pool::erase_task(const task::TaskId& taskId)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queueTasks.erase(std::find_if(m_queueTasks.begin(), m_queueTasks.end(), [&taskId](const TaskInfoPtr& task)
        {
            return task->taskId == taskId;
        }), m_queueTasks.end());
    }
    m_taskDoneEvent.Set();
}

inline thread_pool::thread_pool(std::function<void(const task::TaskId&)>&& onTaskDone, std::uint_fast32_t threadsCount)
    : m_onTaskDone(std::move(onTaskDone))
{
    m_taskDoneEvent.Create();

    EXT_EXPECT(threadsCount > 0) << "Zero thread count";
    m_threads.resize(threadsCount);
    for (auto& thread : m_threads)
        thread.run(&thread_pool::worker, this);
}

inline thread_pool::~thread_pool()
{
    std::for_each(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::interrupt));
    m_notifier.notify_all();
    std::for_each(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::join));
}

inline void thread_pool::wait_for_tasks()
{
    for (;;)
    {
        {
            std::unique_lock lock(m_mutex);
            if (m_queueTasks.empty())
            {
                lock.unlock();

                if (m_countExecutingTasksThread == 0)
                    return;
            }
        }

        m_taskDoneEvent.Wait();
    }
}

inline void thread_pool::detach_all()
{
    std::for_each(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::interrupt));
    m_notifier.notify_all();
    std::for_each(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::detach));
    m_threads.clear();
}

inline void thread_pool::worker()
{
    while (!ext::this_thread::interruption_requested())
    {
        thread_pool::TaskInfoPtr taskToExecute;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (m_queueTasks.empty())
            {
                m_notifier.wait(lock);

                // notification call can be connected with interrupting
                if (ext::this_thread::interruption_requested())
                    return;
            }

            taskToExecute = std::move(m_queueTasks.front());
            m_queueTasks.pop_front();
        }

        ++m_countExecutingTasksThread;
        taskToExecute->task();
        if (m_onTaskDone)
            m_onTaskDone(taskToExecute->taskId);

        --m_countExecutingTasksThread;
        m_taskDoneEvent.Set();
    }
}

} // namespace ext