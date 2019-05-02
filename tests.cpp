#include <catch2/catch.hpp>
#include "task/Task.h"
#include "task/Scheduler.h"
#include <numeric>
#include <random>

using namespace task;

TEST_CASE("simple_tasks") {
    Scheduler::init();

    auto addInts = [](int a, int b) {
        return a + b;
    };
    auto getInt = []{
        return 10;
    };
    auto getIntTask1 = make_task(getInt);
    auto getIntTask2 = make_task(getInt);
    auto addIntsTask = make_task(addInts, getIntTask1, getIntTask2);

    auto res = addIntsTask->result();
    REQUIRE( res == 20 );

    Scheduler::shutdown();
}

TEST_CASE("combine_type_erased_tasks") {
    Scheduler::init();

    auto addInts = [](int a, int b) {
        return a + b;
    };
    auto getInt = []{
        return 10;
    };
    SPTask<int> getIntTask1 = make_task(getInt);
    SPTask<int> getIntTask2 = make_task(getInt);
    auto addIntsTask = make_task(addInts, getIntTask1, getIntTask2);

    auto res = addIntsTask->result();
    REQUIRE( res == 20 );

    Scheduler::shutdown();
}

TEST_CASE("when_all_vector") {
    Scheduler::init();

    std::vector<SPTask<int>> tasks;
    for (int i = 0; i < 5; ++i) {
        tasks.push_back(make_task([=] {
            return i * i;
        }));
    }

    auto root = make_task([=](auto vec) {
        return std::accumulate(begin(vec), end(vec), 0);
    }, when_all(tasks));

    REQUIRE(root->result() == 1 + 2 * 2 + 3 * 3 + 4 * 4);

    Scheduler::shutdown();
}

TEST_CASE("dynamic_task_creation") {
    Scheduler::init();

    auto getIntTask1 = make_task([]{return 0;});
    auto getIntTask2 = make_task([]{return 5;});
    auto task = make_task([](int x, int y) {
        std::vector<SPTask<int>> tasks;
        for (int i = x; i < y; ++i) {
            tasks.push_back(make_task([=]{ return i * 2; }));
        }
        return when_all(tasks);
    }, getIntTask1, getIntTask2);

    auto sumTask = make_task([](auto vec) {
        return std::accumulate(begin(vec), end(vec), 0);
    }, task);

    REQUIRE(sumTask->result() == 2 + 2 * 2 + 3 * 2 + 4 * 2);

    Scheduler::shutdown();
}

template <class Iter>
Iter qsPartition(Iter begin, Iter end) {
    auto i = begin, p = end - 1;
    for (auto j = begin; j < p; ++j) {
        if (*j <= *p) {
            std::iter_swap(i, j);
            ++i;
        }
    }
    std::iter_swap(p, i);
    return i;
}

template <class Iter>
void quickSortSeq(Iter begin, Iter end) {
    if (begin >= end)
        return;
    auto p = qsPartition(begin, end);
    quickSortSeq(begin, p),
    quickSortSeq(p + 1, end);
}

template <class Iter>
SPTask<int> quickSort(Iter begin, Iter end) {
    if (begin >= end)
        return make_task([] { return 0; });
    if (std::distance(begin, end) < 1000) {
        quickSortSeq(begin, end);
        return make_task([] { return 0; });
    }
    auto partition = make_task([=] { return qsPartition(begin, end); });
    auto recurse = make_task([=] (auto p) {
        return when_all(std::vector{
            quickSort(begin, p),
            quickSort(p + 1, end)});
    }, partition);
    return make_task([] (auto) { return 0; }, recurse);
}

TEST_CASE("quick_sort") {
    Scheduler::init();

    std::default_random_engine g;
    std::uniform_int_distribution<int> d(0, 10000);
    std::vector<int> vec;
    for (int i = 0; i < 10000; ++i) {
        vec.push_back(d(g));
    }

    quickSort(begin(vec), end(vec))->result();

    REQUIRE(std::is_sorted(begin(vec), end(vec)));

    Scheduler::shutdown();
}

TEST_CASE("continuation_transfer") {
    Scheduler::init();

    auto sleep = [] { std::this_thread::sleep_for(std::chrono::milliseconds(400)); };

    auto t1 = make_task([&] {
        sleep();
        int a = 10;
        return make_task([&, a] {
            sleep();
            int b = a + 10;
            return make_task([&, b] {
                return b + 10;
            });
        });
    });

    auto t2 = make_task([&] {
        sleep();
        int a = 1;
        return make_task([&, a] {
            sleep();
            int b = a + 1;
            return make_task([&, b] {
                return b + 1;
            });
        });
    });

    auto t3 = make_task([] (auto vec) {
        return vec[0] + vec[1];
    }, when_all(std::vector{t1, t2}));

    REQUIRE(t3->result() == 33);

    Scheduler::shutdown();
}
