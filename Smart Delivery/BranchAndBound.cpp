
#include "BranchAndBound.h"
#include "Greedy.h"
#include <algorithm>
#include <chrono>
#include <functional>
#include <limits>
using namespace std;

BranchBoundResult branchAndBound(
    const vector<vector<double>>& dist,
    double timeLimitSeconds
) {
    using Clock = chrono::steady_clock;
    auto start = Clock::now();

    int n = static_cast<int>(dist.size());

    if (n < 2)
        return { {}, 0.0, 0.0, false, false };

    // Greedy provides an initial complete route
    GreedyResult initial = greedyNearestNeighbor(dist);

    vector<int> bestRoute = initial.route;
    double bestDistance = initial.totalDistance;

    vector<bool> visited(n, false);
    visited[0] = true;

    vector<int> path = { 0 };
    bool timedOut = false;

    // Lower bound using a minimum spanning tree
    // of all unvisited customers
    auto lowerBound = [&](int current) -> double {
        vector<int> remaining;

        for (int i = 1; i < n; i++) {
            if (!visited[i])
                remaining.push_back(i);
        }

        if (remaining.empty())
            return dist[current][0];

        double fromCurrent =
            numeric_limits<double>::infinity();

        double toWarehouse =
            numeric_limits<double>::infinity();

        for (int city : remaining) {
            fromCurrent = min(
                fromCurrent, dist[current][city]
            );

            toWarehouse = min(
                toWarehouse, dist[city][0]
            );
        }

        // Prim's algorithm for MST lower bound
        int m = static_cast<int>(remaining.size());

        vector<double> closest(
            m, numeric_limits<double>::infinity()
        );

        vector<bool> inTree(m, false);
        closest[0] = 0.0;

        double mst = 0.0;

        for (int step = 0; step < m; step++) {
            int next = -1;

            for (int i = 0; i < m; i++) {
                if (!inTree[i] &&
                    (next == -1 ||
                        closest[i] < closest[next])) {
                    next = i;
                }
            }

            inTree[next] = true;
            mst += closest[next];

            for (int j = 0; j < m; j++) {
                if (!inTree[j]) {
                    closest[j] = min(
                        closest[j],
                        dist[remaining[next]][remaining[j]]
                    );
                }
            }
        }

        return fromCurrent + mst + toWarehouse;
        };

    // Branch and Bound search
    function<void(int, int, double)> search =
        [&](int current, int count, double cost) {

        double elapsed =
            chrono::duration<double>(
                Clock::now() - start
            ).count();

        if (elapsed >= timeLimitSeconds) {
            timedOut = true;
            return;
        }

        // All customers visited
        if (count == n) {
            double total = cost + dist[current][0];

            if (total < bestDistance) {
                bestDistance = total;
                bestRoute = path;
                bestRoute.push_back(0);
            }

            return;
        }

        // Prune if this branch cannot beat the best
        if (cost + lowerBound(current) >=
            bestDistance - 1e-10) {
            return;
        }

        vector<int> candidates;

        for (int i = 1; i < n; i++) {
            if (!visited[i])
                candidates.push_back(i);
        }

        // Explore closer customers first
        sort(candidates.begin(), candidates.end(),
            [&](int a, int b) {
                return dist[current][a] <
                    dist[current][b];
            }
        );

        for (int next : candidates) {
            if (timedOut)
                return;

            visited[next] = true;
            path.push_back(next);

            search(
                next,
                count + 1,
                cost + dist[current][next]
            );

            path.pop_back();
            visited[next] = false;
        }
        };

    search(0, 1, 0.0);

    double executionTime =
        chrono::duration<double, micro>(
            Clock::now() - start
        ).count();

    return {
        bestRoute,
        bestDistance,
        executionTime,
        initial.feasible,
        !timedOut
    };
}
