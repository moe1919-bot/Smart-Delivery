
#ifndef GREEDY_H
#define GREEDY_H

#include <vector>
using namespace std;

struct Point {
    double x, y;
};

struct GreedyResult {
    vector<int> route;
    double totalDistance;
    double executionTime;
    bool feasible;
};

vector<vector<double>> buildDistanceMatrix(
    const vector<Point>& locations
);

GreedyResult greedyNearestNeighbor(
    const vector<vector<double>>& dist
);

#endif
