#pragma once

/*
    Task scheduller for executing task by period or at specific time

Example:
#include <ext/thread/scheduller.h>

    bool executed = false;
    const auto taskIdAtTime = ext::Scheduler::GlobalInstance().SubscribeTaskAtTime(
        [&executed]()
        {
            executed = true;
        },
        std::chrono::system_clock::now());
    EXPECT_EQ(taskIdAtTime, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_TRUE(executed);
*/

#include <chrono>
#include <map>
#include <mutex>
#include <optional>

#include <ext/core/check.h>
#include <ext/core/noncopyable.h>
#include <ext/error/dump_writer.h>

namespace ext {

typedef size_t TaskId;
constexpr TaskId kInvalidId = -1;

// Task schedule, allow to set the execution schedule for task or execute it at specific time
// You can use global schedule instance for fast task, or use own copy for long executing tasks
class Scheduler : ext::NonCopyable
{
public:
    explicit Scheduler() noexcept;
    virtual ~Scheduler();

    // Getting global instance of scheduller
    [[nodiscard]] static Scheduler& GlobalInstance() noexcept;

    // Setting task call period by task id, next call will be now() + callingPeriod
    TaskId SubscribeTaskByPeriod(std::function<void()>&& task,
                                 std::chrono::high_resolution_clock::duration callingPeriod,
                                 TaskId taskId = kInvalidId);

    // Setting next task call time by task id, if time < now() execute it immediately
    TaskId SubscribeTaskAtTime(std::function<void()>&& task,
                               std::chrono::system_clock::time_point time,
                               TaskId taskId = kInvalidId);

    // Checking is task exist
    [[nodiscard]] bool IsTaskExists(TaskId taskId) noexcept;

    // Removing task by id
    void RemoveTask(TaskId taskId);

private:
    // Task execution thread
    void MainThread();

private:
    struct TaskInfo;

    std::map<TaskId, TaskInfo> m_tasks;

    std::mutex m_mutexTasks;
    std::condition_variable m_cvTasks;

    std::atomic_bool m_interrupted = false;
    std::thread m_thread;
};;

struct Scheduler::TaskInfo
{
    std::function<void()> task;

    std::chrono::system_clock::time_point nextCallTime;
    std::optional<std::chrono::high_resolution_clock::duration> callingPeriod;

    explicit TaskInfo(std::function<void()>&& function, std::chrono::high_resolution_clock::duration&& period) noexcept
        : task(function)
        , nextCallTime(std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(period))
        , callingPeriod(std::move(period))
    {}

    explicit TaskInfo(std::function<void()>&& function, std::chrono::system_clock::time_point&& callTime) noexcept
        : task(function)
        , nextCallTime(std::move(callTime))
    {
        EXT_ASSERT(nextCallTime > std::chrono::system_clock::now());
    }
};

inline Scheduler::Scheduler() noexcept : m_thread(&Scheduler::MainThread, this)
{}

inline Scheduler::~Scheduler()
{
    EXT_ASSERT(m_thread.joinable());
    m_interrupted = true;
    m_cvTasks.notify_all();
    m_thread.join();
}

inline Scheduler& Scheduler::GlobalInstance() noexcept
{
    static Scheduler globalScheduler;
    return globalScheduler;
}

inline TaskId Scheduler::SubscribeTaskByPeriod(std::function<void()>&& task,
                                               std::chrono::high_resolution_clock::duration callingPeriod,
                                               TaskId taskId)
{
    {
        std::lock_guard<std::mutex> lock(m_mutexTasks);

        if (taskId == kInvalidId || m_tasks.find(taskId) != m_tasks.end())
            taskId = m_tasks.empty() ? 0 : (m_tasks.rbegin()->first + 1);

        EXT_DUMP_IF(!m_tasks.try_emplace(taskId, std::move(task), std::move(callingPeriod)).second);
    }
    m_cvTasks.notify_one();
    return taskId;
}

inline TaskId Scheduler::SubscribeTaskAtTime(std::function<void()>&& task,
                                             std::chrono::system_clock::time_point time,
                                             TaskId taskId)
{
    {
        std::lock_guard<std::mutex> lock(m_mutexTasks);

        if (taskId == kInvalidId || m_tasks.find(taskId) != m_tasks.end())
            taskId = m_tasks.empty() ? 0 : (m_tasks.rbegin()->first + 1);

        EXT_DUMP_IF(!m_tasks.try_emplace(taskId, std::move(task), std::move(time)).second);
    }
    m_cvTasks.notify_one();
    return taskId;
}

inline bool Scheduler::IsTaskExists(TaskId taskId) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutexTasks);
    return m_tasks.find(taskId) != m_tasks.end();
}

inline void Scheduler::RemoveTask(TaskId taskId)
{
    EXT_EXPECT(taskId != kInvalidId);
    {
        std::lock_guard<std::mutex> lock(m_mutexTasks);
        const auto it = m_tasks.find(taskId);
        if (it == m_tasks.end())
            return;

        m_tasks.erase(it);
    }

    m_cvTasks.notify_one();
}

inline void Scheduler::MainThread()
{
    while (!m_interrupted)
    {
        std::function<void()> callBack = nullptr;
        {
            std::unique_lock<std::mutex> lk(m_mutexTasks);

            // wait for scheduled tasks
            while (m_tasks.empty())
            {
                m_cvTasks.wait(lk);

                // notification call can be connected with interrupting
                if (m_interrupted)
                    return;
            }

            const auto it = std::min_element(m_tasks.begin(), m_tasks.end(),
                                             [](const auto &a, const auto &b)
                                             {
                                                 return a.second.nextCallTime < b.second.nextCallTime;
                                             });

            const auto nextCallTime = it->second.nextCallTime;
            if (nextCallTime <= std::chrono::system_clock::now() ||
                m_cvTasks.wait_until(lk, nextCallTime) == std::cv_status::timeout)
            {
                if (!it->second.callingPeriod.has_value())
                {
                    callBack = std::move(it->second.task);
                    m_tasks.erase(it);
                }
                else
                {
                    it->second.nextCallTime += std::chrono::duration_cast<std::chrono::system_clock::duration>(*it->second.callingPeriod);
                    callBack = it->second.task;
                }
            }
        }

        // executing task
        if (callBack)
            callBack();
    }
}

} // namespace ext