#include "d_task_thread.h"

#include <atomic>

#include "d_timer.h"


#ifdef DAL_MULTITHREADING

//TaskManager :: TaskQueue
namespace dal {

    void TaskManager::TaskQueue::push(std::unique_ptr<dal::ITask> t) {
        std::unique_lock<std::mutex> lck{ m_mut, std::defer_lock };

        m_q.push(std::move(t));
    }

    std::unique_ptr<dal::ITask> TaskManager::TaskQueue::pop(void) {
        std::unique_lock<std::mutex> lck{ m_mut, std::defer_lock };

        if ( m_q.empty() ) {
            return nullptr;
        }
        else {
            auto v = std::move(m_q.front());
            m_q.pop();
            return v;
        }
    }

    size_t TaskManager::TaskQueue::size(void) {
        std::unique_lock<std::mutex> lck{ this->m_mut, std::defer_lock };

        return m_q.size();
    }

}


// TaskManager :: TaskRegistry
namespace dal {

    void TaskManager::TaskRegistry::registerTask(const dal::ITask* const task, dal::ITaskListener* const listener) {
        const auto ptr = reinterpret_cast<const void*>(task);

        if (nullptr != listener) {
            this->m_listers.emplace(ptr, listener);
        }
        else {
            this->m_orphan_tasks.emplace(ptr);
        }
    }

    dal::ITaskListener* TaskManager::TaskRegistry::unregister(const dal::ITask* const task) {
        const auto ptr = reinterpret_cast<const void*>(task);

        {
            auto iter = this->m_listers.find(ptr);
            if (iter != this->m_listers.end()) {
                const auto listener = iter->second;
                this->m_listers.erase(iter);
                return listener;
            }
        }

        {
            auto iter = this->m_orphan_tasks.find(ptr);
            if (iter != this->m_orphan_tasks.end()) {
                this->m_orphan_tasks.erase(iter);
                return nullptr;
            }
        }
    }

}



namespace dal {

    class TaskManager::Worker {

    private:
        size_t m_id;
        TaskQueue* m_wait_queue = nullptr;
        TaskQueue* m_done_queue = nullptr;
        std::atomic_bool m_flag_exit;

    public:
        Worker(const Worker&) = delete;
        Worker& operator=(const Worker&) = delete;

    public:
        Worker(const size_t id, TaskQueue& inQ, TaskQueue& outQ)
            : m_id(id)
            , m_wait_queue(&inQ)
            , m_done_queue(&outQ)
            , m_flag_exit(ATOMIC_VAR_INIT(false))
        {

        }

        Worker(Worker&& other) noexcept {
            this->m_id = other.m_id;
            this->m_wait_queue = other.m_wait_queue;
            this->m_done_queue = other.m_done_queue;
            this->m_flag_exit.store(other.m_flag_exit.load());

            other.m_wait_queue = nullptr;
            other.m_done_queue = nullptr;
        }

        Worker& operator=(Worker&& other) noexcept {
            this->m_id = other.m_id;
            this->m_wait_queue = other.m_wait_queue;
            this->m_done_queue = other.m_done_queue;
            this->m_flag_exit.store(other.m_flag_exit.load());

            other.m_wait_queue = nullptr;
            other.m_done_queue = nullptr;

            return *this;
        }

        void operator()() {
            while (true) {
                if (this->m_flag_exit) {
                    return;
                }

                auto task = this->m_wait_queue->pop();
                if (nullptr == task) {
                    dal::sleep_for(0.1);
                    continue;
                }

                task->run();
                this->m_done_queue->push(std::move(task));

                dal::sleep_for(0.1);
            }
        }

        void order_to_get_terminated(void) {
            this->m_flag_exit = true;
        }

    };

}

#endif


namespace dal {

    TaskManager::TaskManager() = default;

    TaskManager::~TaskManager(void) {
        this->destroy();
    }

    void TaskManager::init(const size_t thread_count) {
        this->destroy();

#ifdef DAL_MULTITHREADING
        this->m_workers.reserve(thread_count);
        this->m_threads.reserve(thread_count);

        for (size_t i = 0; i < thread_count; ++i) {
            this->m_workers.push_back(Worker{ i, this->m_wait_queue, this->m_done_queue });
        }

        for (auto& worker : this->m_workers) {
            this->m_threads.emplace_back(std::ref(worker));
        }
#endif

    }

    void TaskManager::update(void) {

#ifdef DAL_MULTITHREADING
        auto task = this->m_done_queue.pop();
        if (nullptr == task) {
            return;
        }

        auto listener = this->m_registry.unregister(task.get());
        if (nullptr != listener) {
            listener->notify_task_done(std::move(task));
            return;
        }
#endif

    }

    void TaskManager::destroy() {

#ifdef DAL_MULTITHREADING
        for (auto& worker : this->m_workers) {
            worker.order_to_get_terminated();
        }

        for (auto& thread : this->m_threads) {
            if (thread.joinable())
                thread.join();
        }
#endif

    }

    void TaskManager::order_task(std::unique_ptr<ITask>&& task, ITaskListener* const client) {

#ifdef DAL_MULTITHREADING
        this->m_registry.registerTask(task.get(), client);
        this->m_wait_queue.push(std::move(task));
#else
        task->run();
        if (nullptr != client) {
            client->notify_task_done(std::move(task));
        }
#endif

    }

}
