
#include "Randomized.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <random>
#include <utility>
using namespace std;

RandomizedResult randomizedMultiStart(
    const vector<vector<double>>& dist,
    int trials
) {
    auto start = chrono::steady_clock::now();

    int n = static_cast<int>(dist.size());

    if (n < 2 || trials < 1)
        return { {}, 0.0, 0.0, false };

    mt19937 rng(random_device{}());

    double bestDistance =
        numeric_limits<double>::infinity();

    vector<int> bestRoute;

    // Run independent randomized attempts
    for (int trial = 0; trial < trials; trial++) {
        vector<bool> visited(n, false);
        vector<int> route = { 0 };

        int current = 0;
        double totalDistance = 0.0;

        visited[0] = true;

        // Visit every customer exactly once
        for (int step = 1; step < n; step++) {
            vector<pair<double, int>> candidates;

            for (int customer = 1; customer < n; customer++) {
                if (!visited[customer]) {
                    candidates.push_back({
                        dist[current][customer],
                        customer
                        });
                }
            }

            // Sort from closest to farthest
            sort(candidates.begin(), candidates.end());

            // Randomly choose one of the 2 nearest
            int choices = min(
                2, static_cast<int>(candidates.size())
            );

            uniform_int_distribution<int> pick(
                0, choices - 1
            );

            int next = candidates[pick(rng)].second;

            totalDistance += dist[current][next];
            visited[next] = true;
            route.push_back(next);

            current = next;
        }

        // Return to warehouse
        totalDistance += dist[current][0];
        route.push_back(0);

        // Keep the shortest route found
        if (totalDistance < bestDistance) {
            bestDistance = totalDistance;
            bestRoute = route;
        }
    }

    // Check feasibility
    bool feasible =
        static_cast<int>(bestRoute.size()) == n + 1 &&
        bestRoute.front() == 0 &&
        bestRoute.back() == 0;

    vector<int> counts(n, 0);

    for (int i = 1;
        i < static_cast<int>(bestRoute.size()) - 1;
        i++) {

        int city = bestRoute[i];

        if (city > 0 && city < n)
            counts[city]++;
        else
            feasible = false;
    }

    for (int city = 1; city < n; city++) {
        if (counts[city] != 1)
            feasible = false;
    }

    double executionTime =
        chrono::duration<double, micro>(
            chrono::steady_clock::now() - start
        ).count();

    return {
        bestRoute,
        bestDistance,
        executionTime,
        feasible
    };
}
