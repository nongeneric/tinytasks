#pragma once

#include "Queue.h"
#include <thread>
#include <unordered_set>
#include <vector>

namespace task {

    class ITask;

    class Scheduler {
        static std::unique_ptr<Scheduler> _instance;
        Queue<std::shared_ptr<ITask>> _queue;
        std::vector<std::thread> _threads;

        Scheduler();

    public:
        static Scheduler* instance();
        static void init();
        static void shutdown();
        void schedule(std::shared_ptr<ITask> task);
    };

} // namespace task
