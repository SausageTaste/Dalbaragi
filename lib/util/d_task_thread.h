#pragma once

#include <mutex>
#include <queue>
#include <memory>
#include <vector>
#include <thread>
#include <unordered_map>
#include <unordered_set>


#define DAL_MULTITHREADING true


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

        virtual float evaluate_priority() const = 0;

    };

    using HTask = std::shared_ptr<ITask>;

    inline auto compare_task_priority = [](const HTask& one, const HTask& other) {
        if (one == other)
            return false;
        else
            return one->evaluate_priority() < other->evaluate_priority();
    };


    class IPriorityTask : public ITask {

    private:
        size_t m_delayed_count;
        PriorityClass m_priority;

    public:
        explicit
        IPriorityTask(const PriorityClass priority) noexcept;

        void set_priority_class(const PriorityClass priority) noexcept;

        void on_delay() override;

        float evaluate_priority() const override;

    };


    class ITaskListener {

    public:
        ITaskListener() = default;
        virtual ~ITaskListener() = default;

        ITaskListener(const ITaskListener&) = delete;
        ITaskListener(ITaskListener&&) = delete;
        ITaskListener& operator=(const ITaskListener&) = delete;
        ITaskListener& operator=(ITaskListener&&) = delete;

        virtual void notify_task_done(HTask& task) = 0;

    };


    class TaskManager {

    private:
        class Worker;


        class TaskRegistry {

        private:
            std::unordered_map<const void*, dal::ITaskListener*> m_listers;
            std::unordered_set<const void*> m_orphan_tasks;

        public:
            TaskRegistry(const TaskRegistry&) = delete;
            TaskRegistry& operator=(const TaskRegistry&) = delete;

        public:
            TaskRegistry() = default;

            void registerTask(const dal::ITask* const task, dal::ITaskListener* const listener);

            dal::ITaskListener* unregister(const dal::ITask* const task);

        };


        class TaskQueue {

        private:
            using queue_t = std::priority_queue<HTask, std::vector<HTask>, decltype(compare_task_priority)>;

        private:
            queue_t m_q;
            std::mutex m_mut;

        public:
            TaskQueue()
                : m_q(compare_task_priority)
            {

            }

            void push(HTask& t);

            HTask pop();

            HTask pick_higher_priority(HTask& t);

            size_t size();

        };


    private:

#if DAL_MULTITHREADING
        std::vector<Worker> m_workers;
        std::vector<std::thread> m_threads;
        TaskQueue m_done_queue;
#else
        HTask m_current_task = nullptr;
#endif
        TaskRegistry m_registry;
        TaskQueue m_wait_queue;

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
        void order_task(HTask task, ITaskListener* const client);

    };

}
