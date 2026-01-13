#include <iostream>
#include <iomanip>
#include <vector>
#include <queue>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <random>

#include <thread>
#include <mutex>
#include <semaphore>
#include <barrier>
#include <atomic>
#include <condition_variable>

using namespace std;

constexpr int THREADS = 10;        // Default number of threads
constexpr int ITERATIONS = 10000;  // Default iterations per thread

string randomChars(size_t len) {
    static thread_local mt19937 gen(random_device{}());
    uniform_int_distribution<> dis(32, 126);

    string s;
    s.reserve(len);
    for (int i = 0; i < len; ++i) {
        s += static_cast<char>(dis(gen));
    }
    return s;
}

void printStats(const vector<size_t>& times) {
    size_t total = accumulate(times.begin(), times.end(), 0LL);
    size_t avg = total / times.size();
    size_t minTime = *min_element(times.begin(), times.end());
    size_t maxTime = *max_element(times.begin(), times.end());
    
    cout << "Avg: " << avg << " ms | Min: " << minTime << " ms | Max: " << maxTime << " ms\n";
}

struct TestResult {
    string name;
    size_t avg_time;
};

vector<TestResult> all_results;

class MutexTest {
    mutex m;
    int cnt = 0;
    vector<size_t> thread_times;
public:
    void benchmark() {
        cnt = 0;
        thread_times.assign(THREADS, 0);
        vector<thread> threads;
        
        for (int i = 0; i < THREADS; ++i) {
            threads.emplace_back([this, i]() {
                auto start = chrono::high_resolution_clock::now();
                for (int j = 0; j < ITERATIONS; ++j) {
                    {
                        lock_guard<mutex> lock(m);
                        cnt++;
                    }
                    randomChars(5);
                }
                auto end = chrono::high_resolution_clock::now();
                thread_times[i] = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }

        for (int i = 0; i < THREADS; ++i) {
            cout << "Thread " << i << " (" << thread_times[i] << " ms)\n";
        }
        size_t avg = accumulate(thread_times.begin(), thread_times.end(), 0LL) / THREADS;
        printStats(thread_times);
        all_results.push_back({"MUTEX", avg});
    }
};

class SemaphoreTest {
    counting_semaphore<1> sem{1};  // максимум 1 поток
    int cnt = 0;
    vector<size_t> thread_times;
public:
    void benchmark() {
        cnt = 0;
        thread_times.assign(THREADS, 0);
        vector<thread> threads;
        
        for (int i = 0; i < THREADS; ++i) {
            threads.emplace_back([this, i]() {
                auto start = chrono::high_resolution_clock::now();
                for (int j = 0; j < ITERATIONS; ++j) {
                    sem.acquire();
                    cnt++;
                    sem.release();
                    randomChars(5);
                }
                auto end = chrono::high_resolution_clock::now();
                thread_times[i] = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            });
        }
        
        for (auto& t : threads) t.join();
        for (int i = 0; i < THREADS; ++i) {
            cout << "Thread " << i << " (" << thread_times[i] << " ms)\n";
        }
        size_t avg = accumulate(thread_times.begin(), thread_times.end(), 0LL) / THREADS;
        printStats(thread_times);
        all_results.push_back({"SEMAPHORE", avg});
    }
};

class BarrierTest {
    barrier<> bar{THREADS};
    int cnt = 0;
    vector<size_t> thread_times;
public:
    void benchmark() {
        cnt = 0;
        thread_times.assign(THREADS, 0);
        
        constexpr int BARRIER_ITERATIONS = 100;
        for (int iter = 0; iter < BARRIER_ITERATIONS; ++iter) {
            vector<thread> threads;
            for (int i = 0; i < THREADS; ++i) {
                threads.emplace_back([this, i]() {
                    auto start = chrono::high_resolution_clock::now();
                    bar.arrive_and_wait();  // Синхронизация потоков
                    cnt++;
                    randomChars(5);
                    auto end = chrono::high_resolution_clock::now();
                    thread_times[i] += chrono::duration_cast<chrono::milliseconds>(end - start).count();
                });
            }
            for (auto& t : threads) t.join();
        }
        for (int i = 0; i < THREADS; ++i) {
            cout << "Thread " << i << " (" << thread_times[i] << " ms)\n";
        }
        size_t avg = accumulate(thread_times.begin(), thread_times.end(), 0LL) / THREADS;
        printStats(thread_times);
        all_results.push_back({"BARRIER", avg});
    }
};

class SpinLockTest {
    atomic<bool> flag{false};
    int cnt = 0;
    vector<size_t> thread_times;
public:
    void benchmark() {
        cnt = 0;
        thread_times.assign(THREADS, 0);
        vector<thread> threads;
        
        for (int i = 0; i < THREADS; ++i) {
            threads.emplace_back([this, i]() {
                auto start = chrono::high_resolution_clock::now();
                for (int j = 0; j < ITERATIONS; ++j) {
                    // Spin-wait: крутиться, пока не захватим лок
                    while (flag.exchange(true, memory_order_acquire));
                    cnt++;
                    flag.store(false, memory_order_release);
                    randomChars(5);
                }
                auto end = chrono::high_resolution_clock::now();
                thread_times[i] = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            });
        }
        
        for (auto& t : threads) t.join();
        for (int i = 0; i < THREADS; ++i) {
            cout << "Thread " << i << " (" << thread_times[i] << " ms)\n";
        }
        size_t avg = accumulate(thread_times.begin(), thread_times.end(), 0LL) / THREADS;
        printStats(thread_times);
        all_results.push_back({"SPINLOCK", avg});
    }
};

class SpinWaitTest {
    atomic<int> cnt{0};
    vector<size_t> thread_times;
public:
    void benchmark() {
        cnt.store(0);
        thread_times.assign(THREADS, 0);
        vector<thread> threads;
        
        for (int i = 0; i < THREADS; ++i) {
            threads.emplace_back([this, i]() {
                auto start = chrono::high_resolution_clock::now();
                for (int j = 0; j < ITERATIONS; ++j) {
                    cnt.fetch_add(1, memory_order_relaxed);  // Без синхронизации
                    randomChars(5);
                }
                auto end = chrono::high_resolution_clock::now();
                thread_times[i] = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            });
        }
        
        for (auto& t : threads) t.join();
        for (int i = 0; i < THREADS; ++i) {
            cout << "Thread " << i << " (" << thread_times[i] << " ms)\n";
        }
        size_t avg = accumulate(thread_times.begin(), thread_times.end(), 0LL) / THREADS;
        printStats(thread_times);
        all_results.push_back({"SPINWAIT", avg});
    }
};


class MonitorTest {
    mutex m;
    condition_variable cv;
    queue<int> buffer;
    atomic<int> produced{0};
    vector<size_t> thread_times;
public:
    void benchmark() {
        produced.store(0);
        thread_times.assign(THREADS, 0);
        vector<thread> threads;
        
        constexpr int HALF = THREADS / 2;
        
        // Producer потоки - добавляют данные в буфер
        for (int i = 0; i < HALF; ++i) {
            threads.emplace_back([this, i]() {
                auto start = chrono::high_resolution_clock::now();
                for (int j = 0; j < ITERATIONS; ++j) {
                    {
                        unique_lock<mutex> lock(m);
                        buffer.push(j);           // Добавить в буфер
                        produced.fetch_add(1);
                    }
                    cv.notify_one();              // Пробудить ожидающий thread
                    randomChars(5);               // Работа вне критической секции
                }
                auto end = chrono::high_resolution_clock::now();
                thread_times[i] = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            });
        }
        
        // Consumer потоки - извлекают данные из буфера
        for (int i = HALF; i < THREADS; ++i) {
            threads.emplace_back([this, i]() {
                auto start = chrono::high_resolution_clock::now();
                int consumed = 0;
                while (consumed < (ITERATIONS / 2)) {
                    {
                        unique_lock<mutex> lock(m);
                        // Ждём, пока буфер не будет готов
                        cv.wait(lock, [this]() { 
                            return !buffer.empty() || produced.load() >= ITERATIONS; 
                        });
                        
                        if (!buffer.empty()) {
                            buffer.pop();
                            consumed++;
                        }
                    }
                    randomChars(5);
                }
                auto end = chrono::high_resolution_clock::now();
                thread_times[i] = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            });
        }
        
        for (auto& t : threads) t.join();
        for (int i = 0; i < THREADS; ++i) {
            cout << "Thread " << i << " (" << thread_times[i] << " ms)\n";
        }
        size_t avg = accumulate(thread_times.begin(), thread_times.end(), 0LL) / THREADS;
        printStats(thread_times);
        all_results.push_back({"MONITOR", avg});
    }
};

int main() {
    cout << "Threads: " << THREADS << " | Iterations: " << ITERATIONS << "\n\n";
    
    cout << "> MUTEX\n";
    MutexTest().benchmark();
    cout << "\n";
    
    cout << "> SEMAPHORE\n";
    SemaphoreTest().benchmark();
    cout << "\n";
    
    cout << "> BARRIER\n";
    BarrierTest().benchmark();
    cout << "\n";
    
    cout << "> SPINLOCK\n";
    SpinLockTest().benchmark();
    cout << "\n";
    
    cout << "> SPINWAIT\n";
    SpinWaitTest().benchmark();
    cout << "\n";
    
    cout << "> MONITOR\n";
    MonitorTest().benchmark();
    cout << "\n";
    
    cout << "> COMPARISON\n";
    sort(all_results.begin(), all_results.end(), [](const TestResult& a, const TestResult& b) {
        return a.avg_time < b.avg_time;
    });
    
    for (size_t i = 0; i < all_results.size(); ++i) {
        cout << (i+1) << ". " << all_results[i].name << ": (avg) " << all_results[i].avg_time << " ms\n";
    }
    
    return 0;
}
