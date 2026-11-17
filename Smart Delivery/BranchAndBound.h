
#ifndef BRANCH_AND_BOUND_H
#define BRANCH_AND_BOUND_H

#include <vector>

struct BranchBoundResult {
    std::vector<int> route;
    double totalDistance;
    double executionTime;
    bool feasible;
    bool optimal;
};

BranchBoundResult branchAndBound(
    const std::vector<std::vector<double>>& dist,
    double timeLimitSeconds = 3.0
);

#endif
