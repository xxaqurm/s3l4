#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>

using namespace std;

class BankerAlgorithm {
private:
    int num_processes;              // Количество процессов
    int num_resources;              // Количество типов ресурсов
    vector<int> total;              // Общее количество ресурсов
    vector<int> available;          // Доступные ресурсы
    vector<vector<int>> max_need;   // Максимальная потребность каждого процесса
    vector<vector<int>> allocated;  // Выделенные ресурсы процессам
    vector<vector<int>> need;       // Оставшаяся потребность
    vector<bool> finished;          // Флаг завершения процесса

public:
    BankerAlgorithm(int p, int r) : num_processes(p), num_resources(r) {
        total.resize(r, 0);
        available.resize(r, 0);
        max_need.resize(p, vector<int>(r, 0));
        allocated.resize(p, vector<int>(r, 0));
        need.resize(p, vector<int>(r, 0));
        finished.resize(p, false);
    }

    // Инициализация системы
    void initialize(vector<int>& tot, vector<vector<int>>& max_arr) {
        total = tot;
        available = tot;
        max_need = max_arr;
        
        // Вычисляем потребность = максимум - выделено (изначально выделено = 0)
        for (int i = 0; i < num_processes; i++) {
            for (int j = 0; j < num_resources; j++) {
                need[i][j] = max_need[i][j];
            }
        }
    }

    // Алгоритм проверки безопасности
    bool isSafe(vector<int>& work, vector<bool>& finish, vector<int>& safe_seq) {
        fill(finish.begin(), finish.end(), false);
        safe_seq.clear();
        work = available;

        // Ищем процесс, который может быть выполнен
        for (int count = 0; count < num_processes; count++) {
            bool found = false;

            for (int i = 0; i < num_processes; i++) {
                if (!finish[i]) {
                    // Проверяем, может ли процесс i получить нужные ресурсы
                    bool can_allocate = true;
                    for (int j = 0; j < num_resources; j++) {
                        if (need[i][j] > work[j]) {
                            can_allocate = false;
                            break;
                        }
                    }

                    if (can_allocate) {
                        // Выделяем ресурсы и "выполняем" процесс
                        for (int j = 0; j < num_resources; j++) {
                            work[j] += allocated[i][j];
                        }
                        finish[i] = true;
                        safe_seq.push_back(i);
                        found = true;
                    }
                }
            }

            if (!found) {
                return false;  // Нет безопасной последовательности
            }
        }
        return true;  // Безопасная последовательность найдена
    }

    // Запрос ресурсов
    bool requestResource(int process_id, vector<int>& request) {
        cout << "\n>>> Процесс " << process_id << " запрашивает ресурсы: ";
        for (int r : request) cout << r << " ";
        cout << "\n";

        // Проверка 1: Запрос не превышает потребность
        for (int i = 0; i < num_resources; i++) {
            if (request[i] > need[process_id][i]) {
                cout << "❌ Ошибка: процесс запросил больше, чем ему нужно!\n";
                return false;
            }
        }

        // Проверка 2: Запрос не превышает доступные ресурсы
        for (int i = 0; i < num_resources; i++) {
            if (request[i] > available[i]) {
                cout << "❌ Ожидание: недостаточно доступных ресурсов\n";
                return false;
            }
        }

        // Временно выделяем ресурсы
        for (int i = 0; i < num_resources; i++) {
            available[i] -= request[i];
            allocated[process_id][i] += request[i];
            need[process_id][i] -= request[i];
        }

        // Проверка безопасности
        vector<int> work;
        vector<bool> finish;
        vector<int> safe_seq;
        finish.resize(num_processes);

        if (isSafe(work, finish, safe_seq)) {
            cout << "✅ Ресурсы выделены безопасно\n";
            cout << "   Безопасная последовательность: ";
            for (int p : safe_seq) cout << "P" << p << " ";
            cout << "\n";
            return true;
        } else {
            // Откатываем выделение
            for (int i = 0; i < num_resources; i++) {
                available[i] += request[i];
                allocated[process_id][i] -= request[i];
                need[process_id][i] += request[i];
            }
            cout << "❌ Запрос может привести к deadlock'у. Отклонено\n";
            return false;
        }
    }

    // Освобождение ресурсов
    void releaseResource(int process_id, vector<int>& release) {
        cout << "\n>>> Процесс " << process_id << " освобождает ресурсы: ";
        for (int r : release) cout << r << " ";
        cout << "\n";

        for (int i = 0; i < num_resources; i++) {
            allocated[process_id][i] -= release[i];
            available[i] += release[i];
            need[process_id][i] += release[i];
        }
        cout << "✅ Ресурсы освобождены\n";
    }

    // Вывод состояния системы
    void printState() {
        cout << "\n" << string(80, '=') << "\n";
        cout << "СОСТОЯНИЕ СИСТЕМЫ\n";
        cout << string(80, '=') << "\n";

        cout << "\nДоступные ресурсы: ";
        for (int r : available) cout << r << " ";
        cout << "\n\n";

        cout << left << setw(8) << "Процесс";
        cout << setw(25) << "Выделено";
        cout << setw(25) << "Максимум";
        cout << setw(25) << "Потребность\n";
        cout << string(80, '-') << "\n";

        for (int i = 0; i < num_processes; i++) {
            cout << left << setw(8) << ("P" + to_string(i));
            
            for (int j = 0; j < num_resources; j++) {
                cout << allocated[i][j] << " ";
            }
            cout << setw(25 - num_resources);
            
            for (int j = 0; j < num_resources; j++) {
                cout << max_need[i][j] << " ";
            }
            cout << setw(25 - num_resources);
            
            for (int j = 0; j < num_resources; j++) {
                cout << need[i][j] << " ";
            }
            cout << "\n";
        }
    }

    // Проверка начального состояния на безопасность
    bool checkInitialSafety() {
        cout << "\n>>> Проверка безопасности начального состояния...\n";
        vector<int> work;
        vector<bool> finish;
        vector<int> safe_seq;
        finish.resize(num_processes);

        if (isSafe(work, finish, safe_seq)) {
            cout << "✅ Система в безопасном состоянии\n";
            cout << "   Безопасная последовательность: ";
            for (int p : safe_seq) cout << "P" << p << " ";
            cout << "\n";
            return true;
        } else {
            cout << "❌ Система в НЕБЕЗОПАСНОМ состоянии (возможен deadlock)\n";
            return false;
        }
    }
};

int main() {
    // Пример 1: Классический тест
    cout << "ПРИМЕР 1: Классическая система (3 процесса, 3 ресурса)\n";
    cout << string(80, '-') << "\n";

    int p = 3;  // Процессы: P0, P1, P2
    int r = 3;  // Ресурсы: A, B, C

    vector<int> total = {10, 5, 7};  // Всего доступно
    vector<vector<int>> max_need = {
        {7, 5, 3},   // P0: макс потребность
        {3, 2, 2},   // P1
        {9, 0, 2}    // P2
    };

    BankerAlgorithm banker(p, r);
    banker.initialize(total, max_need);

    // Начальное состояние
    banker.printState();
    banker.checkInitialSafety();

    // Запросы ресурсов
    vector<int> req1 = {0, 1, 0};
    banker.requestResource(0, req1);
    banker.printState();

    vector<int> req2 = {2, 0, 0};
    banker.requestResource(1, req2);
    banker.printState();

    vector<int> req3 = {3, 0, 2};
    banker.requestResource(2, req3);
    banker.printState();

    // Освобождение ресурсов
    vector<int> rel0 = {0, 1, 0};
    banker.releaseResource(0, rel0);
    banker.printState();

    // Попытка опасного запроса
    cout << "\n>>> Попытка запроса, который может привести к deadlock'у\n";
    vector<int> dangerous_req = {5, 4, 3};
    banker.requestResource(0, dangerous_req);
    banker.printState();

    cout << "\n" << string(80, '=') << "\n";
    cout << "ПРИМЕР 2: Интерактивный режим\n";
    cout << string(80, '-') << "\n\n";

    int p2 = 2;
    int r2 = 2;
    vector<int> total2 = {6, 4};
    vector<vector<int>> max_need2 = {
        {4, 2},
        {3, 3}
    };

    BankerAlgorithm banker2(p2, r2);
    banker2.initialize(total2, max_need2);
    banker2.printState();
    banker2.checkInitialSafety();

    cout << "\n>>> Процесс 0 запрашивает (1, 1)\n";
    vector<int> req = {1, 1};
    banker2.requestResource(0, req);
    banker2.printState();

    cout << "\n>>> Процесс 1 запрашивает (2, 1)\n";
    vector<int> req_p1 = {2, 1};
    banker2.requestResource(1, req_p1);
    banker2.printState();

    cout << "\n>>> Процесс 0 освобождает (1, 1)\n";
    vector<int> release = {1, 1};
    banker2.releaseResource(0, release);
    banker2.printState();

    return 0;
}
