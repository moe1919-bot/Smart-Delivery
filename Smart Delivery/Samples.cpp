
#include "Samples.h"
using namespace std;

vector<Point> getBuiltInSample(int index) {

    const vector<Point> locations = {
        { 0, 0 },    // Warehouse
        { 2, 4 },    // A
        { 5, 2 },    // B
        { 6, 6 },    // C
        { 8, 3 },    // D
        { 3, 7 },    // E
        { 9, 8 },    // F
        { 1, 9 },    // G
        { 10, 1 },   // H
        { 7, 10 },   // I
        { 11, 5 },   // J
        { 4, 11 },   // K
        { 12, 9 }    // L
    };

    const int sizes[5] = { 5, 6, 8, 10, 12 };

    if (index < 0 || index >= 5)
        return {};

    int customers = sizes[index];

    return vector<Point>(
        locations.begin(),
        locations.begin() + customers + 1
    );
}
