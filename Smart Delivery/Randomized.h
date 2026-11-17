
#ifndef RANDOMIZED_H
#define RANDOMIZED_H

#include <vector>

constexpr int RANDOM_TRIALS = 100;

struct RandomizedResult {
    std::vector<int> route;
    double totalDistance;
    double executionTime;
    bool feasible;
};

RandomizedResult randomizedMultiStart(
    const std::vector<std::vector<double>>& dist,
    int trials = RANDOM_TRIALS
);

#endif
