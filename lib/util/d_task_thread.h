#pragma once

#include <mutex>
#include <queue>
#include <memory>
#include <vector>
#include <thread>
#include <unordered_map>
#include <unordered_set>


#define DAL_MULTITHREADING


namespace dal {

    enum class PriorityClass {
        most_wanted,
        within_frame,
        can_be_delayed,
        easy,
        least_wanted,
    };


    class ITask {

    public:
        virtual ~ITask() = default;

        virtual void on_delay() = 0;

        virtual bool work() = 0;

        virtual float evaluate_priority() = 0;

    };


    class IPriorityTask : public ITask {

    private:
        size_t m_delayed_count = 0;
        PriorityClass m_priority = PriorityClass::most_wanted;

    public:
        void on_delay() override {
            ++this->m_delayed_count;
        }

        float evaluate_priority() override {
            return static_cast<float>(PriorityClass::least_wanted) - static_cast<float>(this->m_priority);
        }

    };


    class ITaskListener {

    public:
        ITaskListener() = default;
        virtual ~ITaskListener() = default;

        ITaskListener(const ITaskListener&) = delete;
        ITaskListener(ITaskListener&&) = delete;
        ITaskListener& operator=(const ITaskListener&) = delete;
        ITaskListener& operator=(ITaskListener&&) = delete;

        virtual void notify_task_done(std::unique_ptr<ITask> task) = 0;

    };


    class TaskManager {

#ifdef DAL_MULTITHREADING
    private:
        class Worker;

        class TaskQueue {

        private:
            std::queue<std::unique_ptr<dal::ITask>> m_q;
            std::mutex m_mut;

        public:
            void push(std::unique_ptr<dal::ITask> t);
            std::unique_ptr<dal::ITask> pop(void);
            size_t size(void);

        };


        class TaskRegistry {

        private:
            std::unordered_map<const void*, dal::ITaskListener*> m_listers;
            std::unordered_set<const void*> m_orphan_tasks;

        public:
            TaskRegistry(const TaskRegistry&) = delete;
            TaskRegistry& operator=(const TaskRegistry&) = delete;

        public:
            TaskRegistry(void) = default;
            void registerTask(const dal::ITask* const task, dal::ITaskListener* const listener);
            dal::ITaskListener* unregister(const dal::ITask* const task);

        };


    private:
        std::vector<Worker> m_workers;
        std::vector<std::thread> m_threads;

        TaskQueue m_wait_queue, m_done_queue;
        TaskRegistry m_registry;
#endif

    public:
        TaskManager(const TaskManager&) = delete;
        TaskManager& operator=(const TaskManager&) = delete;
        TaskManager(TaskManager&&) = delete;
        TaskManager& operator=(TaskManager&&) = delete;

    public:
        TaskManager();

        ~TaskManager();

        void init(const size_t thread_count);

        void destroy();

        void update();

        // If client is null, there will be no notification and ITask object will be deleted.
        void order_task(std::unique_ptr<ITask>&& task, ITaskListener* const client);

    };

}
