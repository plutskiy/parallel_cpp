#include "floyd_warshall.h"
#include <iostream>
#include <vector>
#include <climits>
#include <algorithm>

using namespace std;

FloydWarshall::FloydWarshall(const vector<vector<int>>& graph) {
    n = graph.size();
    dist = graph;
    next.resize(n, vector<int>(n, -1));
    hasNegativeCycle = false;
    
    // Инициализация next матрицы для восстановления пути
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (graph[i][j] != INT_MAX && i != j) {
                next[i][j] = j;
            }
        }
    }
}

void FloydWarshall::run() {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (dist[i][k] != INT_MAX && dist[k][j] != INT_MAX) {
                    if (dist[i][j] > dist[i][k] + dist[k][j]) {
                        dist[i][j] = dist[i][k] + dist[k][j];
                        next[i][j] = next[i][k];
                    }
                }
            }
        }
    }
    
    // Проверка на отрицательные циклы
    for (int i = 0; i < n; i++) {
        if (dist[i][i] < 0) {
            hasNegativeCycle = true;
            break;
        }
    }
}

int FloydWarshall::getDistance(int from, int to) const {
    if (from < 0 || from >= n || to < 0 || to >= n) {
        return INT_MAX;
    }
    return dist[from][to];
}

bool FloydWarshall::hasNegativeCycles() const {
    return hasNegativeCycle;
}

vector<int> FloydWarshall::getPath(int from, int to) const {
    vector<int> path;
    
    if (from < 0 || from >= n || to < 0 || to >= n || dist[from][to] == INT_MAX) {
        return path; // Пустой путь
    }
    
    int current = from;
    while (current != to) {
        path.push_back(current);
        current = next[current][to];
        if (current == -1) {
            return vector<int>(); // Путь не существует
        }
    }
    path.push_back(to);
    
    return path;
}

const vector<vector<int>>& FloydWarshall::getDistanceMatrix() const {
    return dist;
}

void FloydWarshall::printMatrix() const {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (dist[i][j] == INT_MAX) {
                cout << "INF\t";
            } else {
                cout << dist[i][j] << "\t";
            }
        }
        cout << endl;
    }
}