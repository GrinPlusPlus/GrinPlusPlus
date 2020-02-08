#pragma once

#include <iomanip>
#include <map>

#include "ctpl_stl.h"

#include "InterruptableSleep.h"
#include "Cron.h"

namespace Bosma {
    using Clock = std::chrono::system_clock;

    class Task {
    public:
        explicit Task(uint64_t taskId, std::function<void()> &&f, bool recur = false, bool interval = false) :
            taskId(taskId), f(std::move(f)), recur(recur), interval(interval) {}

        virtual Clock::time_point get_new_time() const = 0;
        uint64_t GetId() const noexcept { return taskId; }

        uint64_t taskId;
        std::function<void()> f;
        bool recur;
        bool interval;
    };

    class InTask : public Task {
    public:
        explicit InTask(uint64_t taskId, std::function<void()> &&f) : Task(taskId, std::move(f)) {}

        // dummy time_point because it's not used
        Clock::time_point get_new_time() const override { return Clock::time_point(Clock::duration(0)); }
    };

    class EveryTask : public Task {
    public:
        EveryTask(uint64_t taskId, Clock::duration time, std::function<void()> &&f, bool interval = false) :
                Task(taskId, std::move(f), true, interval), time(time) {}

        Clock::time_point get_new_time() const override {
          return Clock::now() + time;
        };
        Clock::duration time;
    };

    class CronTask : public Task {
    public:
        CronTask(uint64_t taskId, const std::string &expression, std::function<void()> &&f) : Task(taskId, std::move(f), true),
                                                                             cron(expression) {}

        Clock::time_point get_new_time() const override {
          return cron.cron_to_next();
        };
        Cron cron;
    };

    inline bool try_parse(std::tm &tm, const std::string &expression, const std::string &format) {
      std::stringstream ss(expression);
      return !(ss >> std::get_time(&tm, format.c_str())).fail();
    }

    class Scheduler {
    public:
        explicit Scheduler(unsigned int max_n_tasks = 4) : nextTaskId(1), done(false), threads(max_n_tasks + 1) {
            threads.push([this](int) {
                while (!done) {
                    if (tasks.empty()) {
                        sleeper.sleep();
                    } else {
                        auto time_of_first_task = (*tasks.begin()).first;
                        sleeper.sleep_until(time_of_first_task);
                    }
                    manage_tasks();
                }
            });
        }

        Scheduler(const Scheduler &) = delete;
        Scheduler(Scheduler &&) noexcept = delete;
        Scheduler &operator=(const Scheduler &) = delete;
        Scheduler &operator=(Scheduler &&) noexcept = delete;

        ~Scheduler() {
            done = true;
            sleeper.interrupt();
        }

        template<typename _Callable, typename... _Args>
        uint64_t in(const Clock::time_point time, _Callable &&f, _Args &&... args) {
            std::shared_ptr<Task> t = std::make_shared<InTask>(
                    nextTaskId++, std::bind(std::forward<_Callable>(f), std::forward<_Args>(args)...));
            return add_task(time, std::move(t));
        }

        template<typename _Callable, typename... _Args>
        uint64_t in(const Clock::duration time, _Callable &&f, _Args &&... args) {
            return in(Clock::now() + time, std::forward<_Callable>(f), std::forward<_Args>(args)...);
        }

        template<typename _Callable, typename... _Args>
        uint64_t at(const std::string &time, _Callable &&f, _Args &&... args) {
            // get current time as a tm object
            auto time_now = Clock::to_time_t(Clock::now());
            std::tm tm = *std::localtime(&time_now);

            // our final time as a time_point
            Clock::time_point tp;

            if (try_parse(tm, time, "%H:%M:%S")) {
                // convert tm back to time_t, then to a time_point and assign to final
                tp = Clock::from_time_t(std::mktime(&tm));

                // if we've already passed this time, the user will mean next day, so add a day.
                if (Clock::now() >= tp) {
                    tp += std::chrono::hours(24);
                }
            } else if (try_parse(tm, time, "%Y-%m-%d %H:%M:%S")) {
                tp = Clock::from_time_t(std::mktime(&tm));
            } else if (try_parse(tm, time, "%Y/%m/%d %H:%M:%S")) {
                tp = Clock::from_time_t(std::mktime(&tm));
            } else {
                // could not parse time
                throw std::runtime_error("Cannot parse time string: " + time);
            }

            return in(tp, std::forward<_Callable>(f), std::forward<_Args>(args)...);
        }

        template<typename _Callable, typename... _Args>
        uint64_t every(const Clock::duration time, _Callable &&f, _Args &&... args) {
            std::shared_ptr<Task> t = std::make_shared<EveryTask>(nextTaskId++, time, std::bind(std::forward<_Callable>(f),
                                                                                std::forward<_Args>(args)...));
            auto next_time = t->get_new_time();
            return add_task(next_time, std::move(t));
        }

        void RemoveTask(const uint64_t taskId) {
            std::lock_guard<std::mutex> l(lock);

            for (auto i = tasks.begin(); i != tasks.end(); ++i) {
                if (i->second->GetId() == taskId) {
                    tasks.erase(i);
                    break;
                }
            }
        }

// expression format:
// from https://en.wikipedia.org/wiki/Cron#Overview
//    ┌───────────── minute (0 - 59)
//    │ ┌───────────── hour (0 - 23)
//    │ │ ┌───────────── day of month (1 - 31)
//    │ │ │ ┌───────────── month (1 - 12)
//    │ │ │ │ ┌───────────── day of week (0 - 6) (Sunday to Saturday)
//    │ │ │ │ │
//    │ │ │ │ │
//    * * * * *
        template<typename _Callable, typename... _Args>
        uint64_t cron(const std::string &expression, _Callable &&f, _Args &&... args) {
            std::shared_ptr<Task> t = std::make_shared<CronTask>(nextTaskId++, expression, std::bind(std::forward<_Callable>(f),
                                                                                        std::forward<_Args>(args)...));
            const uint64_t taskId = t->GetId();
            auto next_time = t->get_new_time();
            return add_task(next_time, std::move(t));
        }

        template<typename _Callable, typename... _Args>
        uint64_t interval(const Clock::duration time, _Callable &&f, _Args &&... args) {
            std::shared_ptr<Task> t = std::make_shared<EveryTask>(nextTaskId++, time, std::bind(std::forward<_Callable>(f),
                                                                                std::forward<_Args>(args)...), true);
            return add_task(Clock::now(), std::move(t));
        }

    private:
        std::atomic<uint64_t> nextTaskId;
        std::atomic<bool> done;

        Bosma::InterruptableSleep sleeper;

        std::multimap<Clock::time_point, std::shared_ptr<Task>> tasks;
        std::mutex lock;
        ctpl::thread_pool threads;

        uint64_t add_task(const Clock::time_point time, std::shared_ptr<Task> t) {
          std::lock_guard<std::mutex> l(lock);
          const uint64_t taskId = t->GetId();
          tasks.emplace(time, std::move(t));
          sleeper.interrupt();
          return taskId;
        }

        void manage_tasks() {
            std::lock_guard<std::mutex> l(lock);

            auto end_of_tasks_to_run = tasks.upper_bound(Clock::now());

            // if there are any tasks to be run and removed
            if (end_of_tasks_to_run != tasks.begin()) {
            // keep track of tasks that will be re-added
            decltype(tasks) recurred_tasks;

            // for all tasks that have been triggered
            for (auto i = tasks.begin(); i != end_of_tasks_to_run; ++i) {

                auto &task = (*i).second;

                if (task->interval) {
                    // if it's an interval task, only add the task back after f() is completed
                    threads.push([this, task](int) {
                        task->f();
                        // no risk of race-condition,
                        // add_task() will wait for manage_tasks() to release lock
                        add_task(task->get_new_time(), task);
                    });
                } else {
                    threads.push([task](int) {
                        task->f();
                    });
                    // calculate time of next run and add the new task to the tasks to be recurred
                    if (task->recur) {
                        recurred_tasks.emplace(task->get_new_time(), std::move(task));
                    }
                }
            }

            // remove the completed tasks
            tasks.erase(tasks.begin(), end_of_tasks_to_run);

            // re-add the tasks that are recurring
            for (auto &task : recurred_tasks)
                tasks.emplace(task.first, std::move(task.second));
          }
        }
    };
}