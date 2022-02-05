#include "d_task_thread.h"

#include <atomic>

#include <daltools/util.h>


// IPriorityTask
namespace dal {

    IPriorityTask::IPriorityTask(const PriorityClass priority) noexcept
        : m_delayed_count(0)
        , m_priority(priority)
    {

    }

    void IPriorityTask::set_priority_class(const PriorityClass priority) noexcept {
        this->m_priority = priority;
    }

    void IPriorityTask::on_delay() {
        ++this->m_delayed_count;
    }

    float IPriorityTask::evaluate_priority() const {
        const auto priority_score = static_cast<double>(static_cast<int>(PriorityClass::least_wanted) - static_cast<int>(this->m_priority));
        const auto delay_score = static_cast<double>(this->m_delayed_count) / 3.0;
        return static_cast<float>(priority_score + delay_score);
    }

}


//TaskManager :: TaskQueue
namespace dal {

    void TaskManager::TaskQueue::push(HTask& t) {
#if DAL_MULTITHREADING
        std::unique_lock<std::mutex> lck{ this->m_mut };
#endif

        m_q.push(t);
    }

    HTask TaskManager::TaskQueue::pop() {
#if DAL_MULTITHREADING
        std::unique_lock<std::mutex> lck{ this->m_mut };
#endif

        if (m_q.empty()) {
            return nullptr;
        }
        else {
            const auto v = this->m_q.top();
            this->m_q.pop();
            return v;
        }
    }

    HTask TaskManager::TaskQueue::pick_higher_priority(HTask& t) {
#if DAL_MULTITHREADING
        std::unique_lock<std::mutex> lck{ this->m_mut };
#endif

        if (this->m_q.empty()) {
            return t;
        }

        if (!t) {
            const auto output = this->m_q.top();
            this->m_q.pop();
            return output;
        }

        if (t->evaluate_priority() < this->m_q.top()->evaluate_priority()) {
            t->on_delay();
            auto output = this->m_q.top();
            this->m_q.push(t);
            return output;
        }
        else {
            return t;
        }
    }

    size_t TaskManager::TaskQueue::size() {
#if DAL_MULTITHREADING
        std::unique_lock<std::mutex> lck{ this->m_mut };
#endif

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

        if (auto iter = this->m_listers.find(ptr); iter != this->m_listers.end()) {
            const auto listener = iter->second;
            this->m_listers.erase(iter);
            return listener;
        }

        if (auto iter = this->m_orphan_tasks.find(ptr); iter != this->m_orphan_tasks.end()) {
            this->m_orphan_tasks.erase(iter);
            return nullptr;
        }

        return nullptr;
    }

}


#if DAL_MULTITHREADING

// TaskManager :: Worker
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
            HTask current_task{ nullptr };

            while (true) {
                if (this->m_flag_exit)
                    return;

                current_task = this->m_wait_queue->pick_higher_priority(current_task);
                if (!current_task) {
                    dal::sleep_for(0.1);
                    continue;
                }

                if (current_task->work()) {
                    this->m_done_queue->push(current_task);
                    current_task = nullptr;
                }
            }
        }

        void order_to_get_terminated() {
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

#if DAL_MULTITHREADING
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

    void TaskManager::update() {

#if DAL_MULTITHREADING
        auto task = this->m_done_queue.pop();
        if (nullptr == task) {
            return;
        }

        auto listener = this->m_registry.unregister(task.get());
        if (nullptr != listener) {
            listener->notify_task_done(task);
            return;
        }
#else
        this->m_current_task = this->m_wait_queue.pick_higher_priority(this->m_current_task);
        if (!this->m_current_task)
            return;

        if (this->m_current_task->work()) {
            auto listener = this->m_registry.unregister(this->m_current_task.get());
            if (nullptr != listener)
                listener->notify_task_done(this->m_current_task);

            this->m_current_task = nullptr;
        }
#endif

    }

    void TaskManager::destroy() {

#if DAL_MULTITHREADING
        for (auto& worker : this->m_workers) {
            worker.order_to_get_terminated();
        }

        for (auto& thread : this->m_threads) {
            if (thread.joinable())
                thread.join();
        }
#endif

    }

    void TaskManager::order_task(HTask task, ITaskListener* const client) {
        this->m_registry.registerTask(task.get(), client);
        this->m_wait_queue.push(task);
    }

}
