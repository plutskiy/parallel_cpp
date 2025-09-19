#include "cyclic_barier.h"
#include <stdexcept>

CyclicBarrier::CyclicBarrier(size_t num_threads) 
    : threads_count(num_threads), 
      waiting_threads(0), 
      generation(0) {
    if (num_threads == 0) {
        throw std::invalid_argument("Number of threads cannot be zero");
    }
}

void CyclicBarrier::await() {
    std::unique_lock<std::mutex> lock(mutex);
    size_t current_generation = generation;
    
    if (++waiting_threads == threads_count) {
        // Последний поток достиг барьера
        waiting_threads = 0;
        generation++;
        condition.notify_all();
    } else {
        // Ожидаем остальные потоки
        condition.wait(lock, [this, current_generation] {
            return current_generation != generation;
        });
    }
}