#pragma once

#include "Scheduler.h"
#include "ContinuationHolder.h"
#include <functional>
#include <future>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>
#include <atomic>
#include <assert.h>

namespace task {

    struct ITask {
        virtual void operator()() = 0;
        virtual ~ITask() = default;
    };

    template <class T>
    class Task : public ITask, public std::enable_shared_from_this<Task<T>> {
    protected:
        std::shared_future<T> _future;
        std::atomic<int> _unscheduledDeps = 0;
        ContinuationHolder _ch;

        template <class Ff, class... Psf>
        friend class TaskImpl;

    public:
        using R = T;

        Task(int unscheduledDeps) : _unscheduledDeps(unscheduledDeps) { }

        R result() {
            return _future.get();
        }

        void addOnSchedule(std::function<void()> action) {
            _ch.attach(action);
        }

        ContinuationHolder* ch() {
            return &_ch;
        }

        void signalDependency() {
            if (!--_unscheduledDeps) {
                Scheduler::instance()->schedule(this->shared_from_this());
            }
        }
    };

    template <class T>
    using SPTask = std::shared_ptr<Task<T>>;

    template <class T>
    struct TaskTraits {
        static constexpr bool isTask = false;
    };

    template <class T>
    struct TaskTraits<SPTask<T>> {
        static constexpr bool isTask = true;
    };

    template <class F, class... Ps>
    class TaskImpl : public Task<std::invoke_result_t<F, typename Ps::element_type::R...>> {
    public:
        using R = std::invoke_result_t<F, typename Ps::element_type::R...>;

    private:
        std::promise<R> _promise;
        F _body;
        std::tuple<Ps...> _deps;

    public:
        TaskImpl(int depCount, F body, Ps... ps)
            : Task<R>(depCount), _body(body), _deps(ps...) {
            Task<R>::_future = _promise.get_future();
        }

        void operator()() override {
            auto i = [&](Ps... ps) {
                _promise.set_value(_body(ps->_future.get()...));
            };
            std::apply(i, _deps);
            if constexpr (TaskTraits<R>::isTask) {
                auto& task = Task<R>::_future.get();
                Task<R>::_ch.transfer(task->ch());
            } else {
                Task<R>::_ch.schedule();
            }
        }
    };

    template <class T>
    struct TaskJoiner {
        T operator()(T t) {
            return t;
        }
    };

    template <class T>
    struct TaskJoiner<SPTask<SPTask<T>>> {
        auto operator()(auto task) {
            return make_task([=] (auto x) { return x->result(); }, task);
        }
    };

    template <class F, class... T>
    auto make_task(F f, T... x) {
        const auto depCount = sizeof...(T);
        auto task = std::make_shared<TaskImpl<F, T...>>(depCount, f, x...);
        if constexpr (depCount == 0) {
            Scheduler::instance()->schedule(task);
        } else {
            (..., x->addOnSchedule([=] {
                task->signalDependency();
            }));
        }
        using TaskType = SPTask<typename TaskImpl<F, T...>::R>;
        return TaskJoiner<TaskType>()(task);
    }

    template <class R>
    auto when_all(std::vector<SPTask<R>> deps) {
        assert(!deps.empty());
        auto collect = [=] {
            std::vector<R> vec;
            for (auto dep : deps) {
                vec.push_back(dep->result());
            }
            return vec;
        };
        auto task = std::make_shared<TaskImpl<decltype(collect)>>(deps.size(), collect);
        for (auto dep : deps) {
            dep->addOnSchedule([=] {
                task->signalDependency();
            });
        }
        return static_cast<SPTask<std::vector<R>>>(task);
    }

} // namespace task
