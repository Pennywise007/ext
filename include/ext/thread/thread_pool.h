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
#include <deque>
#include <type_traits>

#include <ext/core/check.h>
#include <ext/core/noncopyable.h>

#include <ext/reflection/enum.h>

#include <ext/thread/event.h>
#include <ext/thread/thread.h>

#include <ext/types/uuid.h>

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

    [[nodiscard]] static thread_pool& GlobalInstance();

    // interrupt and join all existing threads
    ~thread_pool();

    // wait until task queue not empty
    void wait_for_tasks() const;

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

    // remove task from queue by id and interrupt if it is executing
    // return true if task was removed or interrupted, false if task not found
    bool stop_and_remove_task(const TaskId& taskId);

    [[nodiscard]] std::size_t running_tasks_count() const noexcept;

    // interrupt and remove all tasks from queue
    void interrupt_and_remove_all_tasks();

private:
    // main thread for workers
    void worker(ext::thread& workerThread);

    // task execution priority
    enum class TaskPriority
    {
        eHigh,
        eNormal,
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
    [[nodiscard]] std::pair<TaskId,
              std::future<std::invoke_result_t<Function, Args...>>>
        add_task_with_priority(TaskPriority priority, Function&& function, Args&&... args);

private:
    // struct with task information
    struct TaskInfo;
    typedef std::unique_ptr<TaskInfo> TaskInfoPtr;

    // synchronization of m_queueTasks
    mutable std::mutex m_taskQueueMutex;
    std::condition_variable m_taskQueueChangedNotifier;
    mutable std::condition_variable m_allTasksDoneCv;

    // queue of tasks ordered by TaskPriority
    std::deque<TaskInfoPtr> m_queueTasks;

    // callback to callers about task done
    const std::function<void(const TaskId&)> m_onTaskDone;

    // list of worker threads
    std::vector<ext::thread> m_threads;
    std::unordered_map<TaskId, ext::thread*> m_runningTasks;
    std::atomic_bool m_threadPoolWorks = true;
};

// struct with task information
struct thread_pool::TaskInfo : ext::NonCopyable
{
    explicit TaskInfo(TaskId id, const TaskPriority _priority, std::function<void()>&& _task) noexcept
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

        std::lock_guard<std::mutex> lock(m_taskQueueMutex);
        switch (priority)
        {
        case TaskPriority::eHigh:
        {
            const auto priorityIt = std::find_if(m_queueTasks.cbegin(), m_queueTasks.cend(),
                [priority](const TaskInfoPtr& task)
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
            static_assert(ext::reflection::get_enum_size<TaskPriority>() == 2, 
                "Task priority has unsupported value, extent this enum");
            EXT_UNREACHABLE();
        }
        m_taskQueueChangedNotifier.notify_one();
    }

    EXT_ASSERT(std::any_of(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::thread_works)))
        << "Threads interrupted or stopped";

    return std::make_pair(std::move(taskId), std::move(resultFuture));
}

inline bool thread_pool::stop_and_remove_task(const TaskId& taskId)
{
    ext::thread* threadToInterrupt = nullptr;

    {
        std::lock_guard lock(m_taskQueueMutex);

        auto it = std::find_if(m_queueTasks.begin(), m_queueTasks.end(),
            [&](const TaskInfoPtr& task) {
                return task->taskId == taskId;
            });
        if (it != m_queueTasks.end())
        {
            m_queueTasks.erase(it);
            return true;
        }

        auto runningIt = m_runningTasks.find(taskId);
        if (runningIt == m_runningTasks.end())
            return false;

        threadToInterrupt = runningIt->second;
    }

    threadToInterrupt->interrupt();

    std::unique_lock lock(m_taskQueueMutex);
    m_allTasksDoneCv.wait(lock, [&] {
        return m_runningTasks.find(taskId) == m_runningTasks.end();
    });

    return true;
}

[[nodiscard]] inline std::size_t thread_pool::running_tasks_count() const noexcept
{
    std::lock_guard lock(m_taskQueueMutex);
    return m_runningTasks.size();
}

inline void thread_pool::interrupt_and_remove_all_tasks()
{
    {
        std::lock_guard<std::mutex> lock(m_taskQueueMutex);
        m_queueTasks.clear();
        std::for_each(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::interrupt));
    }
    m_allTasksDoneCv.notify_all();

    wait_for_tasks();
}

inline thread_pool::thread_pool(std::function<void(const TaskId&)>&& onTaskDone, std::uint_fast32_t threadsCount)
    : m_onTaskDone(std::move(onTaskDone))
    , m_threads(threadsCount, {})
{
    for (auto& thread : m_threads)
        thread.run(&thread_pool::worker, this, std::ref(thread));
}

inline thread_pool::thread_pool(std::uint_fast32_t threadsCount)
    : thread_pool(nullptr, threadsCount)
{}

inline thread_pool::~thread_pool()
{
    m_threadPoolWorks = false;

    {
        std::lock_guard<std::mutex> lock(m_taskQueueMutex);
        std::for_each(m_threads.begin(), m_threads.end(), std::mem_fn(&ext::thread::interrupt));
        m_queueTasks.clear();
        m_taskQueueChangedNotifier.notify_all();
    }

    std::for_each(m_threads.begin(), m_threads.end(), [](ext::thread& thread) {
        thread.join();
    });

    m_allTasksDoneCv.notify_all();
}

inline void thread_pool::wait_for_tasks() const
{
    std::unique_lock lock(m_taskQueueMutex);

    m_allTasksDoneCv.wait(lock, [&] {
        return m_queueTasks.empty() && m_runningTasks.empty();
    });
}

inline void thread_pool::worker(ext::thread& workerThread)
{
    while (m_threadPoolWorks)
    {
        thread_pool::TaskInfoPtr taskToExecute;
        {
            std::unique_lock<std::mutex> lock(m_taskQueueMutex);

            m_taskQueueChangedNotifier.wait(lock, [&]() { return !m_queueTasks.empty() || !m_threadPoolWorks; });
            if (!m_threadPoolWorks && m_queueTasks.empty())
                return;

            taskToExecute = std::move(m_queueTasks.front());
            m_queueTasks.pop_front();

            m_runningTasks.emplace(taskToExecute->taskId, &workerThread);
        }

        taskToExecute->task();

        if (m_onTaskDone)
            m_onTaskDone(taskToExecute->taskId);

        {
            std::lock_guard lock(m_taskQueueMutex);
            m_runningTasks.erase(taskToExecute->taskId);
        }

        // If thread was interrupted during task execution, we should restore it to be able to execute next tasks
        if (workerThread.interrupted())
            workerThread.restore_interrupted();

        m_allTasksDoneCv.notify_all();
    }
}

} // namespace ext