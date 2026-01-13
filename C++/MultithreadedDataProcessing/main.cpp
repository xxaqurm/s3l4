#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <map>
#include <random>

using namespace std;

struct Employee {
    string name;
    string position;
    string department;
    double salary;
};

struct ProcessResult {
    vector<Employee> employees_above_avg;
    map<string, double> dept_avg_salary;
    chrono::milliseconds execution_time;
};

vector<Employee> generateEmployees(int count) {
    vector<Employee> employees;
    vector<string> names = {"sdf", "ewoic", "kekw", "fewv", "aew"};
    vector<string> positions = {"Разработчик", "Менеджер", "Аналитик", "Дизайнер", "Тестировщик"};
    vector<string> departments = {"IT", "Sales", "HR", "Finance", "Marketing"};
    
    mt19937 gen(random_device{}());
    uniform_real_distribution<> dis(40000, 150000);
    
    for (int i = 0; i < count; ++i) {
        employees.push_back({
            "# " + to_string(i) + names[i % names.size()],
            positions[i % positions.size()],
            departments[i % departments.size()],
            dis(gen)
        });
    }
    return employees;
}

ProcessResult singleThreadProcess(const vector<Employee>& employees) {
    auto start = chrono::high_resolution_clock::now();
    
    ProcessResult result;
    map<string, vector<double>> dept_salaries;
    
    // зарплаты по отделам
    for (const auto& emp : employees) {
        dept_salaries[emp.department].push_back(emp.salary);
    }
    
    // средняя зарплата по каждому отделу
    for (auto& [dept, salaries] : dept_salaries) {
        double avg = accumulate(salaries.begin(), salaries.end(), 0.0) / salaries.size();
        result.dept_avg_salary[dept] = avg;
    }
    
    for (const auto& emp : employees) {
        if (emp.salary > result.dept_avg_salary[emp.department]) {
            result.employees_above_avg.push_back(emp);
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    result.execution_time = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    return result;
}

ProcessResult multiThreadProcess(const vector<Employee>& employees, int num_threads) {
    auto start = chrono::high_resolution_clock::now();
    
    ProcessResult result;
    map<string, vector<double>> dept_salaries;
    mutex dept_mutex;
    
    // Параллельная группировка данных по отделам
    int chunk_size = (employees.size() + num_threads - 1) / num_threads;
    vector<thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, chunk_size, &employees, &dept_salaries, &dept_mutex]() {
            int start_idx = t * chunk_size;
            int end_idx = min(start_idx + chunk_size, (int)employees.size());
            
            map<string, vector<double>> local_dept_salaries;
            
            // Локальная обработка данных
            for (int i = start_idx; i < end_idx; ++i) {
                local_dept_salaries[employees[i].department].push_back(employees[i].salary);
            }
            
            // Объединение результатов
            {
                lock_guard<mutex> lock(dept_mutex);
                for (auto& [dept, salaries] : local_dept_salaries) {
                    dept_salaries[dept].insert(
                        dept_salaries[dept].end(),
                        salaries.begin(),
                        salaries.end()
                    );
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Вычисление средней зарплаты по отделам
    for (auto& [dept, salaries] : dept_salaries) {
        double avg = accumulate(salaries.begin(), salaries.end(), 0.0) / salaries.size();
        result.dept_avg_salary[dept] = avg;
    }
    
    // Параллельный поиск сотрудников выше среднего
    vector<Employee> local_results[num_threads];
    threads.clear();
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, chunk_size, &employees, &result, &local_results]() {
            int start_idx = t * chunk_size;
            int end_idx = min(start_idx + chunk_size, (int)employees.size());
            
            for (int i = start_idx; i < end_idx; ++i) {
                if (employees[i].salary > result.dept_avg_salary[employees[i].department]) {
                    local_results[t].push_back(employees[i]);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Объединение результатов
    for (int t = 0; t < num_threads; ++t) {
        result.employees_above_avg.insert(
            result.employees_above_avg.end(),
            local_results[t].begin(),
            local_results[t].end()
        );
    }
    
    auto end = chrono::high_resolution_clock::now();
    result.execution_time = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    return result;
}

void printResults(const ProcessResult& result, const string& label) {
    cout << "\n\n";
    cout << label << "\n";
    cout << "\n";
    cout << "Время обработки: " << result.execution_time.count() << " ms\n\n";
    
    cout << "--- Средняя зарплата по отделам ---\n";
    for (const auto& [dept, avg] : result.dept_avg_salary) {
        cout << "  " << left << setw(20) << dept << ": " << fixed << setprecision(2) << avg << " руб.\n";
    }
    
    cout << "\n--- Сотрудники с зарплатой выше средней по отделу (" 
         << result.employees_above_avg.size() << " чел.) ---\n";
    cout << left << setw(30) << "ФИО" 
         << setw(25) << "Должность" 
         << setw(25) << "Отдел" 
         << setw(25) << "Зарплата\n";
    cout << string(80, '-') << "\n";
    
    for (const auto& emp : result.employees_above_avg) {
        cout << left << setw(26) << emp.name 
             << setw(25) << emp.position 
             << setw(25) << emp.department 
             << setw(25) << fixed << setprecision(2) << emp.salary << "\n";
    }
}

int main() {
    int ARRAY_SIZE;
    int NUM_THREADS;
    
    cout << "> Многопоточная обработка данных о сотрудниках\n\n";
    
    cout << "Введите размер массива (количество сотрудников): ";
    cin >> ARRAY_SIZE;
    
    if (ARRAY_SIZE <= 0) {
        cerr << "Ошибка: размер массива должен быть положительным числом!\n";
        return 1;
    }
    
    cout << "Введите количество потоков: ";
    cin >> NUM_THREADS;
    
    if (NUM_THREADS <= 0) {
        cerr << "Ошибка: количество потоков должно быть положительным числом\n";
        return 1;
    }

    cout << "\nГенерирование данных...\n";
    auto employees = generateEmployees(ARRAY_SIZE);
    
    cout << "Однопоточная обработка...\n";
    auto single_result = singleThreadProcess(employees);
    
    cout << "Многопоточная обработка...\n";
    auto multi_result = multiThreadProcess(employees, NUM_THREADS);
    
    cout << "\n> СРАВНЕНИЕ\n";
    cout << "Однопоточно:  " << single_result.execution_time.count() << " ms\n";
    cout << "Многопоточно: " << multi_result.execution_time.count() << " ms\n";
    double speedup = (double)single_result.execution_time.count() / multi_result.execution_time.count();
    cout << "Ускорение:    " << fixed << setprecision(2) << speedup << "x\n";
    
    char show_results;
    cout << "\nВывести подробные результаты? (y/n): ";
    cin >> show_results;
    
    if (show_results == 'y' || show_results == 'Y') {
        printResults(single_result, "ОДНОПОТОЧНАЯ ОБРАБОТКА");
        printResults(multi_result, "МНОГОПОТОЧНАЯ ОБРАБОТКА");
    }
    
    return 0;
}