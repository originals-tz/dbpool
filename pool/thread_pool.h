#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <atomic>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

namespace threadpool
{
struct Task
{
public:
    virtual void Run() = 0;

    void SetTaskType(const std::string& type) { m_task_type = type; }

    std::string GetTaskType() { return m_task_type; }

private:
    std::string m_task_type;
};

class State
{
public:
    size_t Add()
    {
        m_id++;
        m_state[m_id];
        return m_id;
    }

    void Set(size_t id, const std::string& key) { m_state[id] = key; }

    std::string Get()
    {
        std::stringstream ss;
        for (auto& state : m_state)
        {
            ss << state.first << " : " << (state.second.empty() ? "idle" : state.second) << std::endl;
            ;
        }
        return ss.str();
    }

private:
    size_t m_id = 0;
    std::map<size_t, std::string> m_state;
};

class Context
{
public:
    Context()
        : m_is_stop(false)
    {}

    void Add(const std::shared_ptr<Task>& task)
    {
        std::lock_guard<std::mutex> lk(m_mut);
        m_task_list.emplace_back(task);
        m_cond.notify_one();
    }

    std::shared_ptr<Task> Get()
    {
        {
            std::lock_guard<std::mutex> lk(m_mut);
            if (!m_task_list.empty())
            {
                auto task = m_task_list.front();
                m_task_list.pop_front();
                return task;
            }
        }

        std::unique_lock<std::mutex> lk(m_mut);
        m_cond.wait(lk, [&] { return !m_task_list.empty() || m_is_stop; });
        if (m_is_stop)
        {
            return nullptr;
        }
        auto task = m_task_list.front();
        m_task_list.pop_front();
        return task;
    }

    void Stop()
    {
        m_is_stop = true;
        m_cond.notify_all();
    }

    size_t GetID()
    {
        std::lock_guard<std::mutex> lk(m_mut);
        return m_state.Add();
    }

    void SetState(size_t id, const std::string& key = "")
    {
        std::lock_guard<std::mutex> lk(m_mut);
        m_state.Set(id, key);
    }

    std::string GetState()
    {
        std::lock_guard<std::mutex> lk(m_mut);
        return m_state.Get();
    }

    bool IsStop() { return m_is_stop; }

private:
    std::atomic<bool> m_is_stop;
    std::mutex m_mut;
    std::condition_variable m_cond;
    std::list<std::shared_ptr<Task>> m_task_list;
    State m_state;
};

class WorkThread
{
public:
    WorkThread(std::shared_ptr<Context> ctx, size_t id)
        : m_ctx(ctx)
        , m_id(id)
    {
        m_thread.reset(new std::thread(&WorkThread::Run, this));
    }

    ~WorkThread()
    {
        if (m_thread->joinable())
        {
            m_thread->join();
        }
    }

    void Run()
    {
        while (!m_ctx->IsStop())
        {
            auto task = m_ctx->Get();
            if (task)
            {
                m_ctx->SetState(m_id, task->GetTaskType());
                task->Run();
            }
        }
    }

private:
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<Context> m_ctx;
    size_t m_id;
};

class ThreadPool
{
public:
    ThreadPool(size_t size)
        : m_size(size)
    {
        m_ctx = std::make_shared<Context>();
        for (size_t i = 0; i < m_size; i++)
        {
            m_worker_list.emplace_back(new WorkThread(m_ctx, m_ctx->GetID()));
        }
    }

    ~ThreadPool() { m_ctx->Stop(); }

    template <typename T>
    void AddTask(const std::shared_ptr<T>& task)
    {
        task->SetTaskType(typeid(T).name());
        m_ctx->Add(task);
    }

    std::string GetState() { return m_ctx->GetState(); }

private:
    size_t m_size;
    std::shared_ptr<Context> m_ctx;
    std::list<std::unique_ptr<WorkThread>> m_worker_list;
};

}  // namespace threadpool
#endif  // !THREAD_POOL_H_
