#pragma once

/*
 * Thread pool implementation, allow to add task for execution on already existed threads
 * Example:

#include <ext/thread/thread_pool.h>

std::set<ext::TaskId, ext::TaskIdHelper> taskList;
ext::thread_pool threadPool([&taskList, &listMutex](const ext::TaskId& taskId)
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

#include <ext/utils/uuid.h>

namespace ext {

// thread pool for execution tasks
class thread_pool : ext::NonCopyable
{
public:
    using TaskId = ext::uuid;

    /**
     * \param onTaskDone callback on execution task by id
     * \param threadsCount working threads count
     */
    explicit thread_pool(std::function<void(const TaskId&)>&& onTaskDone,
                         std::uint_fast32_t threadsCount = std::thread::hardware_concurrency());
    thread_pool(std::uint_fast32_t threadsCount = std::thread::hardware_concurrency());

    EXT_NODISCARD static thread_pool& GlobalInstance();

    // interrupt and join all existing threads
    ~thread_pool();

    // wait until task queue not empty
    void wait_for_tasks();

    // detach and clear all working threads list, after calling this function class will be in inconsistent state
    void detach_all();

    /**
     * \brief Add task function to queue
     * \tparam Function to invoke
     * \tparam Args list of arguments passed to function
     * \param function execution function
     * \param args list of arguments passed to function
     * \return pair of a taskId(created task identifier) and a future(with task result)
     */
    template <typename Function, typename... Args>
    std::pair<TaskId,
              std::future<
                std::invoke_result_t<Function, Args...>
              >>
        add_task(Function&& function, Args&&... args);
    
    /**
     * \brief Add task function to queue with high priority
     * \tparam Function to invoke
     * \tparam Args list of arguments passed to function
     * \param function execution function
     * \param args list of arguments passed to function
     * \return pair of a taskId(created task identifier) and a future(with task result)
     */
    template <typename Function, typename... Args>
    std::pair<TaskId,
              std::future<
                std::invoke_result_t<Function, Args...>
              >>
        add_high_priority_task(Function&& function, Args&&... args);

    // remove task from queue by id
    void erase_task(const TaskId& taskId);

    EXT_NODISCARD uint32_t running_tasks_count() const EXT_NOEXCEPT;

    // interrupt and remove all tasks from queue, after this action thread pool is in inconsistent state
    void interrupt_and_remove_all_tasks();

private:
    // main thread for workers
    void worker();

    // task execution priority
    enum class TaskPriority
    {
        eHigh,
        eNormal
    };

    /**
     * \brief Add function to queue with high priority
     * \tparam Function to invoke
     * \tparam Args list of arguments passed to function
     * \param priority task execution priority
     * \param function execution function
     * \param args list of arguments passed to function
     * \return pair of a taskId(created task identifier) and a future(with task result)
     */    
    template <typename Function, typename... Args>
    EXT_NODISCARD std::pair<TaskId,
              std::future<std::invoke_result_t<Function, Args...>>>
        add_task_with_priority(TaskPriority priority, Function&& function, Args&&... args);

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
    const std::function<void(const TaskId&)> m_onTaskDone;

    // list of worker threads
    std::list<ext::thread> m_threads;
    std::atomic_bool m_threadPoolWorks = true;
};

// struct with task information
struct thread_pool::TaskInfo : ext::NonCopyable
{
    explicit TaskInfo(TaskId id, const TaskPriority _priority, std::function<void()>&& _task) EXT_NOEXCEPT
        : task(std::move(_task)), priority(_priority), taskId(std::move(id))
    {}

    TaskInfo(const TaskInfo&) = delete;
    TaskInfo(TaskInfo&&) = delete;

    const std::function<void()> task;
    const TaskPriority priority;
    const TaskId taskId;
};

inline thread_pool& thread_pool::GlobalInstance()
{
    static thread_pool globalThreadPool;
    return globalThreadPool;
}

template <typename Function, typename... Args>
std::pair<thread_pool::TaskId, 
          std::future<
            std::invoke_result_t<Function, Args...>
            >>
    thread_pool::add_task(Function&& function, Args&&... args)
{
    return add_task_with_priority(TaskPriority::eNormal, std::forward<Function>(function), std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
std::pair<thread_pool::TaskId, 
          std::future<
            std::invoke_result_t<Function, Args...>
            >>
    thread_pool::add_high_priority_task(Function&& function, Args&&... args)
{
    return add_task_with_priority(TaskPriority::eHigh, std::forward<Function>(function), std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
std::pair<thread_pool::TaskId, 
          std::future<
            std::invoke_result_t<Function, Args...>
            >>
    thread_pool::add_task_with_priority(TaskPriority priority, Function&& function, Args&&... args)
{
    using _Result = std::invoke_result_t<Function, Args...>;
    using PackagedTaskPtr = std::unique_ptr<std::packaged_task<_Result()>>;

    // packaging task with params
    PackagedTaskPtr packagedTask = std::make_unique<std::packaged_task<_Result()>>(
            ext::ThreadInvoker<Function, Args...>(
                std::forward<Function>(function), std::forward<Args>(args)...)
            );
    std::future<_Result> resultFuture = packagedTask->get_future();

    // adding task to a queue
    TaskId taskId;
    {
        auto taskInfo = std::make_unique<TaskInfo>(taskId, priority,
            [taskPointer = packagedTask.release()]()
            {
                PackagedTaskPtr taskPtr(taskPointer);
                taskPtr->operator()();
            });

        std::lock_guard<std::mutex> lock(m_mutex);
        switch (priority)
        {
        case TaskPriority::eHigh:
        {
            const auto priorityIt = std::find_if(m_queueTasks.cbegin(), m_queueTasks.cend(),
                [&priority](const TaskInfoPtr& task)
                {
                    return task->priority > priority;
                });   
            m_queueTasks.emplace(priorityIt, std::move(taskInfo));
            break;
        }
        case TaskPriority::eNormal:
            m_queueTasks.emplace_back(std::move(taskInfo));
            break;
        default:
            EXT_UNREACHABLE();
        }
    }
    m_notifier.notify_one();

    EXT_ASSERT(std::any_of(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::thread_works)))
        << "Threads interrupted or stopped";

    return std::make_pair(std::move(taskId), std::move(resultFuture));
}

inline void thread_pool::erase_task(const TaskId& taskId)
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

EXT_NODISCARD inline uint32_t thread_pool::running_tasks_count() const EXT_NOEXCEPT
{
    return m_countExecutingTasksThread;
}

inline void thread_pool::interrupt_and_remove_all_tasks()
{
    EXT_ASSERT(std::all_of(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::thread_works)))
        << "Threads already interrupted or stopped";
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queueTasks.clear();
    }
    std::for_each(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::interrupt));
    m_taskDoneEvent.Set();
    wait_for_tasks();
    std::for_each(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::restore_interrupted));
}

inline thread_pool::thread_pool(std::function<void(const TaskId&)>&& onTaskDone, std::uint_fast32_t threadsCount)
    : m_onTaskDone(std::move(onTaskDone))
{
    EXT_EXPECT(threadsCount > 0) << "Zero thread count";
    m_threads.resize(threadsCount);
    for (auto& thread : m_threads)
        thread.run(&thread_pool::worker, this);
}

inline thread_pool::thread_pool(std::uint_fast32_t threadsCount)
    : thread_pool(nullptr, threadsCount)
{}

inline thread_pool::~thread_pool()
{
    m_threadPoolWorks = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queueTasks.clear();
    }
    std::for_each(m_threads.begin(), m_threads.end(), [](ext::thread& thread) {
        thread.interrupt();
    });
    m_notifier.notify_all();
    std::for_each(m_threads.begin(), m_threads.end(), [](ext::thread& thread) {
        thread.join();
    });
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
        m_taskDoneEvent.Reset();
    }
}

inline void thread_pool::detach_all()
{
    std::for_each(m_threads.begin(), m_threads.end(), [](ext::thread& thread) {
        thread.interrupt();
    });
    m_notifier.notify_all();
    std::for_each(m_threads.begin(), m_threads.end(), [](ext::thread& thread) {
        thread.detach();
    });
    m_threads.clear();
}

inline void thread_pool::worker()
{
    while (m_threadPoolWorks)
    {
        thread_pool::TaskInfoPtr taskToExecute;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (m_queueTasks.empty())
            {
                m_notifier.wait(lock);

                // notification call can be connected with interrupting
                if (!m_threadPoolWorks)
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