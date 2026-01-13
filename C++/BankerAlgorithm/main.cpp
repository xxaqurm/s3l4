#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <iomanip>

using namespace std;

// Алгоритм банкира для избежания deadlock'а
class BankersAlgorithm {
private:
    int num_processes;
    int num_resources;
    
    vector<vector<int>> max_need;      // максимум, что может потребоваться каждому процессу
    vector<vector<int>> allocated;     // сколько выделено каждому процессу
    vector<int> available;             // сколько ресурсов свободно
    
    mutex banker_mutex;
    
    // Проверить, является ли состояние безопасным
    bool isSafe() {
        vector<int> work = available;
        vector<bool> finish(num_processes, false);
        int count = 0;
        
        while (count < num_processes) {
            bool found = false;
            for (int i = 0; i < num_processes; ++i) {
                if (finish[i]) continue;
                
                // Проверить, может ли процесс завершиться
                bool can_finish = true;
                for (int j = 0; j < num_resources; ++j) {
                    int need = max_need[i][j] - allocated[i][j];
                    if (work[j] < need) {
                        can_finish = false;
                        break;
                    }
                }
                
                if (can_finish) {
                    // Процесс может завершиться, освобождает ресурсы
                    for (int j = 0; j < num_resources; ++j) {
                        work[j] += allocated[i][j];
                    }
                    finish[i] = true;
                    found = true;
                    count++;
                    break;
                }
            }
            
            if (!found) {
                // Не найден ни один процесс, который может завершиться
                return false;
            }
        }
        
        return true;
    }
    
public:
    BankersAlgorithm(int p, int r) : num_processes(p), num_resources(r) {
        max_need.assign(p, vector<int>(r));
        allocated.assign(p, vector<int>(r, 0));
        available.assign(r, 0);
    }
    
    // Инициализировать систему
    void initialize(
        const vector<int>& total_resources, 
        const vector<vector<int>>& max_needs
    ) {
        available = total_resources;
        max_need = max_needs;
    }
    
    // Запросить ресурсы
    bool requestResources(int process_id, const vector<int>& request) {
        lock_guard<mutex> lock(banker_mutex);
        
        for (int i = 0; i < num_resources; ++i) {
            if (request[i] > max_need[process_id][i] - allocated[process_id][i]) {
                return false;  // Запрос превышает максимум
            }
            if (request[i] > available[i]) {
                return false;  // Нет достаточно свободных ресурсов
            }
        }
        
        // Пробуем выделить ресурсы
        for (int i = 0; i < num_resources; ++i) {
            available[i] -= request[i];
            allocated[process_id][i] += request[i];
        }
        
        // Проверить, является ли состояние безопасным
        if (isSafe()) {
            return true;
        } else {  // риск deadlock
            for (int i = 0; i < num_resources; ++i) {
                available[i] += request[i];
                allocated[process_id][i] -= request[i];
            }
            return false;
        }
    }
    
    // Освободить ресурсы
    void releaseResources(int process_id, const vector<int>& release) {
        lock_guard<mutex> lock(banker_mutex);
        
        for (int i = 0; i < num_resources; ++i) {
            allocated[process_id][i] -= release[i];
            available[i] += release[i];
        }
    }
    
    // Вывести состояние системы
    void printState() {
        lock_guard<mutex> lock(banker_mutex);
        
        cout << "Available resources: ";
        for (int r : available) cout << r << " ";
        cout << "\n";
        
        cout << "Allocated:\n";
        for (int i = 0; i < num_processes; ++i) {
            cout << "  Process " << i << ": ";
            for (int j = 0; j < num_resources; ++j) {
                cout << allocated[i][j] << " ";
            }
            cout << "\n";
        }
    }
};

int main() {
    cout << "BANKER'S ALGORITHM \n";
    cout << "Deadlock avoidance algorithm\n\n";
    
    int num_processes = 5;  // threads
    int num_resources = 3;
    
    vector<int> total_resources = {10, 5, 7};
    
    // Максимальные потребности для каждого процесса
    vector<vector<int>> max_needs = {
        {7, 5, 3},
        {3, 2, 2},
        {9, 0, 2},
        {2, 2, 2},
        {4, 3, 3}
    };
    
    BankersAlgorithm banker(num_processes, num_resources);
    banker.initialize(total_resources, max_needs);
    
    cout << "Initial state:\n";
    banker.printState();
    cout << "\n";
    
    // Симуляция запросов ресурсов
    vector<thread> threads;
    mt19937 gen(random_device{}());
    
    for (int pid = 0; pid < num_processes; ++pid) {
        threads.emplace_back([&banker, pid, &gen]() {
            this_thread::sleep_for(chrono::milliseconds(pid * 100));
            
            // Каждый процесс делает несколько запросов
            for (int req = 0; req < 3; ++req) {
                uniform_int_distribution<> dis(0, 2);
                vector<int> request = {dis(gen), dis(gen), dis(gen)};
                
                cout << "Process " << pid << " requests: ";
                for (int r : request) cout << r << " ";
                
                if (banker.requestResources(pid, request)) {
                    cout << " -> GRANTED\n";
                    this_thread::sleep_for(chrono::milliseconds(50));
                    
                    // Освободить ресурсы
                    banker.releaseResources(pid, request);
                    cout << "Process " << pid << " releases resources\n";
                } else {
                    cout << " -> DENIED (deadlock risk)\n";
                }
            }
        });
    }
    
    // Ждём завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }
    
    cout << "\n Final state \n";
    banker.printState();
    
    return 0;
}