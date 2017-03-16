#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <future>
#include <queue>
#include <thread>

namespace qp {
namespace threading {

class Threadpool {
 public:
  // Starts a thread pool with the number of threads available on the machine.
  Threadpool() : Threadpool(std::thread::hardware_concurrency()) {}

  // Starts a thread pool with the specified number of threads.  Be careful as
  // creating too many threads will have adverse performance benefits.
  Threadpool(int n_threads);

  // Add work to the queue.  Returns a future contain the results.
  template <typename F, typename... Args>
  auto add(F&& f, Args&&... args) ->
      typename std::future<typename std::result_of<F(Args...)>::type>;

  // Shuts down the threadpool.  All tasks currently being executed will finish
  // and all threads will be joined.  All tasks still in the queue will be
  // aborted, and their futures will be invalidated.
  ~Threadpool();

 private:
  std::vector<std::thread> threads_;
  bool shutdown_ = false;
  std::queue<std::function<void()>> work_queue_;
  std::mutex mu_;
  std::condition_variable work_added_;

  // Worker function which actually carries out the tasks.
  void worker();
};

Threadpool::Threadpool(int n_threads) {
  for (int i = 0; i < n_threads; ++i) {
    threads_.emplace_back(&Threadpool::worker, this);
  }
}

template <typename F, typename... Args>
auto Threadpool::add(F&& f, Args&&... args) ->
    typename std::future<typename std::result_of<F(Args...)>::type> {
  using ReturnType = typename std::result_of<F(Args...)>::type;

  auto work = std::make_shared<std::packaged_task<ReturnType()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<ReturnType> ret = work->get_future();

  {
    std::lock_guard<std::mutex> lock(mu_);
    work_queue_.emplace([work]() { (*work)(); });
  }

  work_added_.notify_one();
  return ret;
}

void Threadpool::worker() {
  while (true) {
    std::function<void()> work;

    {
      std::unique_lock<std::mutex> lock(mu_);
      work_added_.wait(lock,
                       [this] { return shutdown_ || !work_queue_.empty(); });
      if (shutdown_) return;

      work = std::move(work_queue_.front());
      work_queue_.pop();
    }

    work();
  }
}

Threadpool::~Threadpool() {
  mu_.lock();
  shutdown_ = true;
  mu_.unlock();

  work_added_.notify_all();
  for (auto& thread : threads_) {
    thread.join();
  }
}

}  // namespace threading
}  // namespace qp

#endif /* THREADPOOL_H */
