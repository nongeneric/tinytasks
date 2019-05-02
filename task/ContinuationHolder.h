#pragma once

#include <vector>
#include <mutex>
#include <functional>

namespace task {

    class ContinuationHolder {
        std::mutex _m;
        std::vector<std::function<void()>> _actions;
        bool _scheduled = false;
        ContinuationHolder* _transferred = nullptr;

    public:
        void attach(std::function<void()> action);
        void schedule();
        void transfer(ContinuationHolder* dest);
    };

} // namespace task
