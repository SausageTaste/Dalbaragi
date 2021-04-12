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

    class ITask {

    public:
        virtual ~ITask() = default;

        virtual void run() = 0;

    };


    class ITaskListener {

    public:
        virtual ~ITaskListener() = default;

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
        TaskManager(const size_t thread_count);

        ~TaskManager();

        void update();

        // If client is null, there will be no notification and ITask object will be deleted.
        void order_task(std::unique_ptr<ITask> task, ITaskListener* const client);

    };

}
