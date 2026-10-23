#ifndef FLOYD_WARSHALL_H
#define FLOYD_WARSHALL_H

#include <climits>
#include <vector>
#include <thread>

class FloydWarshall {
 private:
  std::vector<std::vector<int>> dist;
  std::vector<std::vector<int>> next;  // Для восстановления пути
  int n;
  bool hasNegativeCycle;

 public:
  // Конструктор принимает матрицу смежности
  FloydWarshall(const std::vector<std::vector<int>>& graph);

  // Запуск алгоритма
  void run(unsigned int num_threads = std::thread::hardware_concurrency());

  // Получение кратчайшего расстояния между двумя вершинами
  int getDistance(int from, int to) const;

  // Проверка наличия отрицательного цикла
  bool hasNegativeCycles() const;

  // Восстановление пути между двумя вершинами
  std::vector<int> getPath(int from, int to) const;

  // Получение всей матрицы расстояний
  const std::vector<std::vector<int>>& getDistanceMatrix() const;

  // Утилита для вывода матрицы
  void printMatrix() const;
};

#endif