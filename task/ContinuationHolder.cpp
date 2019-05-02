#include "ContinuationHolder.h"

#include "Scheduler.h"
#include "assert.h"

namespace task {

    void ContinuationHolder::attach(std::function<void()> action) {
        auto lock = std::lock_guard(_m);
        if (_transferred) {
            _transferred->attach(action);
        } else if (_scheduled) {
            action();
        } else {
            _actions.push_back(action);
        }
    }

    void ContinuationHolder::schedule() {
        {
            auto lock = std::lock_guard(_m);
            assert(!_scheduled);
            _scheduled = true;
        }
        for (auto& action : _actions) {
            action();
        }
    }

    void ContinuationHolder::transfer(ContinuationHolder* dest) {
        {
            auto lock = std::lock_guard(_m);
            assert(!_scheduled);
            _transferred = dest;
        }
        for (auto& action : _actions) {
            dest->attach(action);
        }
    }

} // namespace task
