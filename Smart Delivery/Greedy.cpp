
#include "Greedy.h"
#include <cmath>
#include <chrono>
#include <limits>
using namespace std;

vector<vector<double>> buildDistanceMatrix(
    const vector<Point>& locations
) {
    int n = locations.size();
    vector<vector<double>> dist(n, vector<double>(n));

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double dx = locations[i].x - locations[j].x;
            double dy = locations[i].y - locations[j].y;

            dist[i][j] = sqrt(dx * dx + dy * dy);
        }
    }

    return dist;
}

GreedyResult greedyNearestNeighbor(
    const vector<vector<double>>& dist
) {
    auto start = chrono::high_resolution_clock::now();

    int n = dist.size();
    vector<bool> visited(n, false);
    vector<int> route;

    int current = 0;
    double totalDistance = 0;

    route.push_back(0);
    visited[0] = true;

    // Visit the nearest unvisited customer
    for (int i = 1; i < n; i++) {
        int nearest = -1;
        double minDistance = numeric_limits<double>::infinity();

        for (int j = 1; j < n; j++) {
            if (!visited[j] && dist[current][j] < minDistance) {
                minDistance = dist[current][j];
                nearest = j;
            }
        }

        if (nearest == -1)
            break;

        visited[nearest] = true;
        route.push_back(nearest);

        totalDistance += dist[current][nearest];
        current = nearest;
    }

    // Return to warehouse
    totalDistance += dist[current][0];
    route.push_back(0);

    auto end = chrono::high_resolution_clock::now();

    double executionTime =
        chrono::duration<double, micro>(end - start).count();

    // Check if every customer was visited exactly once
    bool feasible = (route.size() == n + 1 &&
        route.front() == 0 &&
        route.back() == 0);

    for (int i = 1; i < n; i++) {
        int count = 0;

        for (int j = 1; j < (int)route.size() - 1; j++) {
            if (route[j] == i)
                count++;
        }

        if (count != 1)
            feasible = false;
    }

    return { route, totalDistance, executionTime, feasible };
}
