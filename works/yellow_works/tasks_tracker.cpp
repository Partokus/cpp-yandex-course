#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>
#include <variant>
#include <cmath>
#include <numeric>
#include <limits>
#include <tuple>

using namespace std;

enum class TaskStatus
{
    NEW,
    IN_PROGRESS,
    TESTING,
    DONE
};

// для хранения количества задач каждого статуса
using TasksInfo = map<TaskStatus, int>;

struct TeamTasks
{
    // Получить статистику по статусам задач конкретного разработчика
    const TasksInfo &GetPersonTasksInfo(const string &person) const
    {
        return _tasks_of_persons.at(person);
    }

    // Добавить новую задачу (в статусе NEW) для конкретного разработчитка
    void AddNewTask(const string &person)
    {
        ++_tasks_of_persons[person][TaskStatus::NEW];
    }

    // Обновить статусы по данному количеству задач конкретного разработчика,
    // подробности см. ниже
    tuple<TasksInfo, TasksInfo> PerformPersonTasks(const string &person, int task_count)
    {
        TasksInfo &tasks = _tasks_of_persons[person];
        TasksInfo old_tasks = tasks;
        TasksInfo updated_tasks = {};

        for (auto &[task_status, number_of_tasks] : tasks)
        {
            if (task_status == TaskStatus::DONE)
            {
                break;
            }

            while (task_count != 0)
            {
                --task_count;

                --old_tasks[task_status];
                ++updated_tasks[static_cast<TaskStatus>(static_cast<int>(task_status) + 1)];
                if (old_tasks.at(task_status) == 0)
                {
                    old_tasks.erase(task_status);
                    break;
                }
            }
        }

        tasks = old_tasks;
        for (const auto &[task_status, number_of_tasks] : updated_tasks)
        {
            tasks[task_status] += number_of_tasks;
        }

        old_tasks.erase(TaskStatus::DONE);

        return {updated_tasks, old_tasks};
    }

private:
    map<string, TasksInfo> _tasks_of_persons;
};

// Принимаем словарь по значению, чтобы иметь возможность
// обращаться к отсутствующим ключам с помощью [] и получать 0,
// не меняя при этом исходный словарь
void PrintTasksInfo(TasksInfo tasks_info)
{
    cout << tasks_info[TaskStatus::NEW] << " new tasks"
         << ", " << tasks_info[TaskStatus::IN_PROGRESS] << " tasks in progress"
         << ", " << tasks_info[TaskStatus::TESTING] << " tasks are being tested"
         << ", " << tasks_info[TaskStatus::DONE] << " tasks are done" << endl;
}

int main()
{
    TeamTasks tasks;
    tasks.AddNewTask("Ilia");
    for (int i = 0; i < 3; ++i)
    {
        tasks.AddNewTask("Ivan");
    }
    cout << "Ilia's tasks: ";
    PrintTasksInfo(tasks.GetPersonTasksInfo("Ilia"));
    cout << "Ivan's tasks: ";
    PrintTasksInfo(tasks.GetPersonTasksInfo("Ivan"));

    TasksInfo updated_tasks, untouched_tasks;

    tie(updated_tasks, untouched_tasks) =
        tasks.PerformPersonTasks("Ivan", 2);
    cout << "Updated Ivan's tasks: ";
    PrintTasksInfo(updated_tasks);
    cout << "Untouched Ivan's tasks: ";
    PrintTasksInfo(untouched_tasks);

    tie(updated_tasks, untouched_tasks) =
        tasks.PerformPersonTasks("Ivan", 2);
    cout << "Updated Ivan's tasks: ";
    PrintTasksInfo(updated_tasks);
    cout << "Untouched Ivan's tasks: ";
    PrintTasksInfo(untouched_tasks);
    return 0;
}
