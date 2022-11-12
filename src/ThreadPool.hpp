#pragma once

#include <queue>
#include <thread>
#include <future>

struct ThreadPool final {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency()) noexcept : token(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back(&ThreadPool::runThreadLoop, this);
        }
    }

    ~ThreadPool() {
        stop();
    }

public:
    auto submit(std::invocable auto&& fn) -> std::future<decltype(fn())> {
        if constexpr(std::is_void_v<decltype(fn())>) {
            auto job = std::packaged_task<void()>{std::forward<decltype(fn)>(fn)};
            auto out = job.get_future();

            std::unique_lock lock{guard};
            jobs.emplace(std::move(job));
            lock.unlock();

            signal.notify_one();
            return out;
        } else {
            auto result = std::promise<decltype(fn())>{};

            auto out = result.get_future();
            auto job = std::packaged_task<void()>{[
                fn = std::forward<decltype(fn)>(fn),
                result = std::move(result)
            ] mutable {
                try {
                    result.set_value(fn());
                } catch (...) {
                    result.set_exception(std::current_exception());
                }
            }};

            std::unique_lock lock{guard};
            jobs.emplace(std::move(job));
            lock.unlock();

            signal.notify_one();
            return out;
        }
    }

    void stop() {
        std::unique_lock lock{guard};
        token.store(true);
        lock.unlock();

        signal.notify_all();

        for (auto& worker : workers) {
            worker.join();
        }
    }

private:
    void runThreadLoop() {
        while (true) {
            std::unique_lock lock{guard};
            if (jobs.empty()) {
                signal.wait(lock, [this] {
                    return !jobs.empty() || token;
                });
            }

            if (token) {
                break;
            }

            auto job = std::move(jobs.front());
            jobs.pop();

            lock.unlock();

            job();
        }
    }

private:
    std::mutex guard = {};
    std::atomic<bool> token = {};
    std::condition_variable signal = {};
    std::vector<std::thread> workers = {};
    std::queue<std::packaged_task<void()>> jobs = {};
};