#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace task {

    template <class T>
    class Queue {
        std::queue<T> _queue;
        std::mutex _m;
        std::condition_variable _cv;

    public:
        void push(T value) {
            auto lock = std::unique_lock(_m);
            _queue.push(value);
            _cv.notify_one();
        }

        void pop(T& value) {
            auto lock = std::unique_lock(_m);
            _cv.wait(lock, [this] { return !_queue.empty(); });
            value = _queue.front();
            _queue.pop();
        }
    };

} // namespace task
