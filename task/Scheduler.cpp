#include "Scheduler.h"
#include "Task.h"
#include <functional>
#include <optional>

using namespace task;

namespace task {

    void Scheduler::init() {
        _instance.reset(new Scheduler());
    }

    void Scheduler::shutdown() {
        auto i = _instance.get();

        for (auto j = 0u; j < i->_threads.size(); ++j) {
            i->_queue.push({});
        }

        for (auto& th : i->_threads) {
            th.join();
        }
    }

    void Scheduler::schedule(std::shared_ptr<ITask> task) {
        _queue.push(task);
    }

    Scheduler::Scheduler() {
        auto body = [&] {
            for (;;) {
                std::shared_ptr<ITask> task;
                _queue.pop(task);
                if (!task)
                    return;
                (*task)();
            }
        };

        _threads.resize(std::thread::hardware_concurrency());
        for (auto& th : _threads) {
            th = std::thread(body);
        }
    }

    Scheduler* Scheduler::instance() {
        return _instance.get();
    }

    std::unique_ptr<Scheduler> Scheduler::_instance;

} // namespace task
