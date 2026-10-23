#ifndef CYCLIC_BARRIER_H
#define CYCLIC_BARRIER_H

#include <condition_variable>
#include <mutex>

class CyclicBarrier {
 public:
  explicit CyclicBarrier(size_t num_threads);
  void await();

 private:
  size_t threads_count;
  size_t waiting_threads;
  size_t generation;
  std::mutex mutex;
  std::condition_variable condition;
};

#endif