#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <random>

const int NUM_PHILOSOPHERS = 5;
const int times = 5;

std::mutex forks[NUM_PHILOSOPHERS];
std::mutex output_mutex;
std::condition_variable cv;
bool forks_available[NUM_PHILOSOPHERS] = {true, true, true, true, true};

void think(int philosopher_id) {
    {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "Философ " << philosopher_id << " размышляет." << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 2000));
}

void eat(int philosopher_id) {
    {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "Философ " << philosopher_id << " ест." << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 1000));
}

void philosopher(int id) {
    int left_fork = id;
    int right_fork = (id + 1) % NUM_PHILOSOPHERS;
    int count = 0;

    while (count < times) {
        think(id);

        // Попытка взять вилки
        std::unique_lock<std::mutex> lock(forks[0]); // Лочим первый мьютекс для условий
        cv.wait(lock, [&]() {
            return forks_available[left_fork] && forks_available[right_fork];
        });

        forks_available[left_fork] = false;
        forks_available[right_fork] = false;
        lock.unlock();

        eat(id);

        // Возвращение вилок
        {
            std::lock_guard<std::mutex> lock(forks[0]);
            forks_available[left_fork] = true;
            forks_available[right_fork] = true;
        }
        cv.notify_all(); // Уведомляем других философов
        count++;
    }
}

int main() {
    srand(time(nullptr));
    std::vector<std::thread> philosophers;

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers.emplace_back(philosopher, i);
    }

    for (auto& ph : philosophers) {
        ph.join();
    }

    return 0;
}