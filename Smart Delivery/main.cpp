
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commdlg.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "Greedy.h"
#include "BranchAndBound.h"
#include "Randomized.h"
#include "Samples.h"

#pragma comment(lib, "Comdlg32.lib")

using namespace std;
namespace fs = std::filesystem;

// GUI IDs
#define ID_SAMPLE_FIRST 101
#define ID_EXTERNAL 106
#define ID_BROWSE 107
#define ID_GREEDY 108
#define ID_BRANCH 109
#define ID_RANDOM100 110
#define ID_RANDOM500 111
#define ID_TIMER 200

const int TEST_SIZES[5] = { 5, 6, 8, 10, 12 };

using Sample = vector<Point>;

// GUI controls
HWND sampleRadios[5] = {};
HWND externalRadio = nullptr;
HWND fileBox = nullptr;
HWND browseButton = nullptr;
HWND greedyButton = nullptr;
HWND branchButton = nullptr;
HWND random100Button = nullptr;
HWND random500Button = nullptr;
HWND resultBox = nullptr;

int selectedInput = 0;
wstring externalFile;

future<wstring> runningTask;
bool isRunning = false;

enum class Algorithm {
    Greedy,
    BranchAndBound,
    Randomized100,
    Randomized500
};

struct RouteResult {
    vector<int> route;
    double distance;
    double executionTime;
    bool feasible;
    bool optimal;
};

// -------------------------------------
// READ EXTERNAL FILES
// -------------------------------------

bool loadSamples(
    const fs::path& path,
    Algorithm algorithm,
    vector<Sample>& samples,
    wstring& error
) {
    ifstream file(path);

    if (!file.is_open()) {
        error = L"Cannot open the selected file.";
        return false;
    }

    int testCases;

    if (!(file >> testCases) ||
        testCases < 1 || testCases > 50000) {

        error = L"Invalid number of test cases.";
        return false;
    }

    if (algorithm == Algorithm::BranchAndBound &&
        testCases > 20) {

        error = L"Branch and Bound supports at most "
            L"20 test cases per file.";
        return false;
    }

    if ((algorithm == Algorithm::Randomized100 ||
        algorithm == Algorithm::Randomized500) &&
        testCases > 200) {

        error = L"Randomized supports at most "
            L"200 test cases per file.";
        return false;
    }

    for (int test = 0; test < testCases; test++) {

        int n;

        if (!(file >> n) || n < 5 || n > 100) {
            error = L"Invalid customer count in test " +
                to_wstring(test + 1);
            return false;
        }

        if (algorithm == Algorithm::BranchAndBound &&
            n > 12) {

            error = L"Branch and Bound supports at most "
                L"12 customers.";
            return false;
        }

        Sample locations(n + 1);

        for (int i = 0; i <= n; i++) {

            if (!(file >> locations[i].x >>
                locations[i].y)) {

                error = L"Invalid coordinates in test " +
                    to_wstring(test + 1);
                return false;
            }
        }

        samples.push_back(locations);
    }

    return true;
}

// -------------------------------------
// RUN SELECTED ALGORITHM
// -------------------------------------

RouteResult solveRoute(
    const vector<vector<double>>& dist,
    Algorithm algorithm
) {
    if (algorithm == Algorithm::Greedy) {

        GreedyResult r = greedyNearestNeighbor(dist);

        return {
            r.route,
            r.totalDistance,
            r.executionTime,
            r.feasible,
            false
        };
    }

    if (algorithm == Algorithm::BranchAndBound) {

        BranchBoundResult r = branchAndBound(dist, 3.0);

        return {
            r.route,
            r.totalDistance,
            r.executionTime,
            r.feasible,
            r.optimal
        };
    }

    int trials =
        (algorithm == Algorithm::Randomized500)
        ? 500 : 100;

    RandomizedResult r =
        randomizedMultiStart(dist, trials);

    return {
        r.route,
        r.totalDistance,
        r.executionTime,
        r.feasible,
        false
    };
}

// -------------------------------------
// DISPLAY ROUTE
// -------------------------------------

void printLocation(
    wostringstream& output,
    int city
) {
    if (city == 0)
        output << L"Warehouse";

    else if (city <= 26)
        output << wchar_t(L'A' + city - 1);

    else
        output << L"C" << city;
}

// -------------------------------------
// PROCESS SAMPLES
// -------------------------------------

wstring runSamples(
    const vector<Sample>& samples,
    Algorithm algorithm
) {
    wostringstream output;
    output << fixed << setprecision(2);

    if (algorithm == Algorithm::Greedy)
        output << L"GREEDY NEAREST NEIGHBOR\r\n";

    else if (algorithm == Algorithm::BranchAndBound)
        output << L"BRANCH AND BOUND\r\n";

    else if (algorithm == Algorithm::Randomized100)
        output << L"RANDOMIZED MULTI-START (100 TRIALS)\r\n";

    else
        output << L"RANDOMIZED MULTI-START (500 TRIALS)\r\n";

    output << L"================================\r\n\r\n";

    int feasibleCount = 0;
    int optimalCount = 0;
    double totalTime = 0.0;

    for (int test = 0;
        test < static_cast<int>(samples.size());
        test++) {

        const Sample& locations = samples[test];

        int n = static_cast<int>(locations.size()) - 1;

        vector<vector<double>> dist =
            buildDistanceMatrix(locations);

        RouteResult result = solveRoute(dist, algorithm);

        totalTime += result.executionTime;

        if (result.feasible)
            feasibleCount++;

        if (result.optimal)
            optimalCount++;

        if (test >= 100)
            continue;

        output << L"Sample " << test + 1
            << L" (" << n << L" customers)\r\n";

        output << L"Route: ";

        for (int i = 0;
            i < static_cast<int>(result.route.size());
            i++) {

            printLocation(output, result.route[i]);

            if (i + 1 <
                static_cast<int>(result.route.size()))
                output << L" -> ";
        }

        output << L"\r\nTotal Distance: "
            << result.distance;

        output << L"\r\nExecution Time: "
            << result.executionTime
            << L" microseconds";

        output << L"\r\nFeasible: "
            << (result.feasible ? L"Yes" : L"No");

        if (algorithm == Algorithm::BranchAndBound) {

            output << L"\r\nOptimal: "
                << (result.optimal
                    ? L"Yes (proven)"
                    : L"Not proven (time limit)");
        }
        else {
            output << L"\r\nOptimal: Not guaranteed";
        }

        if (algorithm == Algorithm::Randomized100)
            output << L"\r\nTrials: 100";

        if (algorithm == Algorithm::Randomized500)
            output << L"\r\nTrials: 500";

        output << L"\r\n--------------------------------\r\n\r\n";
    }

    if (samples.size() > 100)
        output << L"Only the first 100 routes displayed.\r\n\r\n";

    output << L"Total Samples: "
        << samples.size() << L"\r\n";

    output << L"Feasible Routes: "
        << feasibleCount << L" / "
        << samples.size() << L"\r\n";

    output << L"Total Algorithm Time: "
        << totalTime << L" microseconds\r\n";

    if (algorithm == Algorithm::BranchAndBound) {

        output << L"Optimal Routes Proven: "
            << optimalCount << L" / "
            << samples.size() << L"\r\n";
    }

    return output.str();
}

// -------------------------------------
// BUILT-IN OR EXTERNAL INPUT
// -------------------------------------

wstring runSelectedInput(
    int selection,
    const fs::path& path,
    Algorithm algorithm
) {
    vector<Sample> samples;

    if (selection >= 0 && selection < 5) {

        // Samples come directly from Samples.cpp
        Sample sample = getBuiltInSample(selection);

        if (sample.empty())
            return L"Error: Invalid built-in sample.";

        samples.push_back(sample);
    }
    else {

        // Optional external text file
        wstring error;

        if (!loadSamples(path, algorithm, samples, error))
            return L"INPUT ERROR: " + error;
    }

    return runSamples(samples, algorithm);
}

// -------------------------------------
// FILE EXPLORER
// -------------------------------------

bool chooseTextFile(
    HWND window,
    wstring& selected
) {
    wchar_t filename[4096] = {};

    OPENFILENAMEW ofn = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = window;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = 4096;

    ofn.lpstrFilter =
        L"Text Files (*.txt)\0*.txt\0"
        L"All Files (*.*)\0*.*\0";

    ofn.nFilterIndex = 1;

    ofn.Flags =
        OFN_FILEMUSTEXIST |
        OFN_PATHMUSTEXIST |
        OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&ofn))
        return false;

    selected = filename;
    return true;
}

// -------------------------------------
// RADIO BUTTON SELECTION
// -------------------------------------

void selectRadio(int choice) {

    selectedInput = choice;

    for (int i = 0; i < 5; i++) {

        SendMessageW(
            sampleRadios[i],
            BM_SETCHECK,
            (choice == i) ? BST_CHECKED : BST_UNCHECKED,
            0
        );
    }

    SendMessageW(
        externalRadio,
        BM_SETCHECK,
        (choice == 5) ? BST_CHECKED : BST_UNCHECKED,
        0
    );
}

void browseExternal(HWND window) {

    wstring selected;

    if (!chooseTextFile(window, selected))
        return;

    externalFile = selected;

    SetWindowTextW(
        fileBox,
        externalFile.c_str()
    );

    selectRadio(5);

    SetWindowTextW(
        resultBox,
        L"External file selected.\r\n"
        L"Choose an algorithm to run."
    );
}

// -------------------------------------
// CREATE GUI CONTROLS
// -------------------------------------

HWND addControl(
    HWND parent,
    HINSTANCE instance,
    const wchar_t* type,
    const wchar_t* title,
    DWORD style,
    int x, int y,
    int width, int height,
    int id = 0
) {
    HWND control = CreateWindowW(
        type,
        title,
        WS_CHILD | WS_VISIBLE | style,
        x, y, width, height,
        parent,
        reinterpret_cast<HMENU>(
            static_cast<INT_PTR>(id)
            ),
        instance,
        nullptr
    );

    if (control) {

        SendMessageW(
            control,
            WM_SETFONT,
            (WPARAM)GetStockObject(DEFAULT_GUI_FONT),
            TRUE
        );
    }

    return control;
}

// -------------------------------------
// ENABLE / DISABLE BUTTONS
// -------------------------------------

void setRunning(bool running) {

    isRunning = running;

    for (HWND radio : sampleRadios)
        EnableWindow(radio, !running);

    HWND controls[] = {
        externalRadio,
        browseButton,
        greedyButton,
        branchButton,
        random100Button,
        random500Button
    };

    for (HWND control : controls)
        EnableWindow(control, !running);
}

// -------------------------------------
// START ALGORITHM
// -------------------------------------

void startAlgorithm(
    HWND window,
    Algorithm algorithm
) {
    if (isRunning)
        return;

    int selection = selectedInput;
    fs::path inputPath;

    if (selection == 5) {

        if (externalFile.empty()) {

            MessageBoxW(
                window,
                L"Please select a text file first.",
                L"No File Selected",
                MB_OK | MB_ICONWARNING
            );

            return;
        }

        inputPath = externalFile;
    }

    setRunning(true);

    SetWindowTextW(
        resultBox,
        L"Running algorithm...\r\n"
        L"Results will appear when finished."
    );

    runningTask = async(
        launch::async,
        [selection, inputPath, algorithm]() {

            return runSelectedInput(
                selection,
                inputPath,
                algorithm
            );
        }
    );

    SetTimer(window, ID_TIMER, 100, nullptr);
}

// -------------------------------------
// WINDOWS EVENTS
// -------------------------------------

LRESULT CALLBACK WindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    switch (message) {

    case WM_CREATE: {

        HINSTANCE instance =
            ((LPCREATESTRUCTW)lParam)->hInstance;

        addControl(
            window, instance,
            L"STATIC",
            L"Choose a built-in sample:",
            0,
            16, 15, 350, 24
        );

        // Five built-in sample radio buttons
        for (int i = 0; i < 5; i++) {

            wstring label =
                to_wstring(TEST_SIZES[i]) +
                L" customers";

            sampleRadios[i] = addControl(
                window, instance,
                L"BUTTON",
                label.c_str(),
                BS_AUTORADIOBUTTON |
                (i == 0 ? WS_GROUP : 0),
                25, 45 + i * 27,
                250, 24,
                ID_SAMPLE_FIRST + i
            );
        }

        addControl(
            window, instance,
            L"STATIC",
            L"All five samples are included in the executable.",
            0,
            320, 60, 480, 24
        );

        addControl(
            window, instance,
            L"STATIC",
            L"No sample files required.",
            0,
            320, 95, 400, 24
        );

        addControl(
            window, instance,
            L"STATIC",
            L"Or use an external file:",
            0,
            16, 198, 300, 24
        );

        externalRadio = addControl(
            window, instance,
            L"BUTTON",
            L"External .txt file",
            BS_AUTORADIOBUTTON,
            25, 225, 250, 24,
            ID_EXTERNAL
        );

        fileBox = addControl(
            window, instance,
            L"EDIT",
            L"No external file selected",
            WS_BORDER | ES_READONLY | ES_AUTOHSCROLL,
            16, 258, 680, 28
        );

        browseButton = addControl(
            window, instance,
            L"BUTTON",
            L"Browse...",
            BS_PUSHBUTTON,
            713, 258, 135, 28,
            ID_BROWSE
        );

        greedyButton = addControl(
            window, instance,
            L"BUTTON",
            L"Run Greedy",
            BS_PUSHBUTTON,
            16, 311, 160, 36,
            ID_GREEDY
        );

        branchButton = addControl(
            window, instance,
            L"BUTTON",
            L"Run Branch && Bound",
            BS_PUSHBUTTON,
            188, 311, 200, 36,
            ID_BRANCH
        );

        random100Button = addControl(
            window, instance,
            L"BUTTON",
            L"Run Randomized (100)",
            BS_PUSHBUTTON,
            400, 311, 215, 36,
            ID_RANDOM100
        );

        random500Button = addControl(
            window, instance,
            L"BUTTON",
            L"Run Randomized (500)",
            BS_PUSHBUTTON,
            627, 311, 215, 36,
            ID_RANDOM500
        );

        addControl(
            window, instance,
            L"STATIC",
            L"Results:",
            0,
            16, 365, 300, 24
        );

        resultBox = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"Choose a sample and run an algorithm.",
            WS_CHILD | WS_VISIBLE |
            WS_VSCROLL |
            ES_MULTILINE |
            ES_AUTOVSCROLL |
            ES_READONLY,
            16, 390, 844, 300,
            window, nullptr, instance, nullptr
        );

        SendMessageW(
            resultBox,
            WM_SETFONT,
            (WPARAM)GetStockObject(DEFAULT_GUI_FONT),
            TRUE
        );

        selectRadio(0);

        return 0;
    }

    case WM_COMMAND: {

        int command = LOWORD(wParam);

        // Built-in sample selection
        if (command >= ID_SAMPLE_FIRST &&
            command < ID_SAMPLE_FIRST + 5) {

            int index = command - ID_SAMPLE_FIRST;

            selectRadio(index);

            wstring text =
                L"Selected: " +
                to_wstring(TEST_SIZES[index]) +
                L" customers.\r\n"
                L"Choose an algorithm to run.";

            SetWindowTextW(
                resultBox,
                text.c_str()
            );

            return 0;
        }

        if (command == ID_EXTERNAL) {

            selectRadio(5);

            SetWindowTextW(
                resultBox,
                L"Select an external file using Browse."
            );

            return 0;
        }

        if (command == ID_BROWSE) {

            browseExternal(window);
            return 0;
        }

        if (command == ID_GREEDY) {

            startAlgorithm(
                window,
                Algorithm::Greedy
            );

            return 0;
        }

        if (command == ID_BRANCH) {

            startAlgorithm(
                window,
                Algorithm::BranchAndBound
            );

            return 0;
        }

        if (command == ID_RANDOM100) {

            startAlgorithm(
                window,
                Algorithm::Randomized100
            );

            return 0;
        }

        if (command == ID_RANDOM500) {

            startAlgorithm(
                window,
                Algorithm::Randomized500
            );

            return 0;
        }

        break;
    }

    case WM_TIMER: {

        if (wParam == ID_TIMER &&
            isRunning &&
            runningTask.valid()) {

            if (runningTask.wait_for(
                chrono::milliseconds(0)
            ) == future_status::ready) {

                KillTimer(window, ID_TIMER);

                wstring results;

                try {
                    results = runningTask.get();
                }
                catch (...) {
                    results =
                        L"An error occurred while "
                        L"running the algorithm.";
                }

                SetWindowTextW(
                    resultBox,
                    results.c_str()
                );

                setRunning(false);
            }
        }

        return 0;
    }

    case WM_SIZE: {

        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        if (resultBox) {

            MoveWindow(
                resultBox,
                16, 390,
                std::max(400, width - 32),
                std::max(120, height - 406),
                TRUE
            );
        }

        if (fileBox) {

            MoveWindow(
                fileBox,
                16, 258,
                std::max(200, width - 188),
                28, TRUE
            );

            MoveWindow(
                browseButton,
                width - 151, 258,
                135, 28, TRUE
            );
        }

        return 0;
    }

    case WM_CLOSE: {

        if (isRunning) {

            MessageBoxW(
                window,
                L"Please let the algorithm finish "
                L"before closing.",
                L"Algorithm Running",
                MB_OK | MB_ICONINFORMATION
            );

            return 0;
        }

        DestroyWindow(window);
        return 0;
    }

    case WM_DESTROY:

        KillTimer(window, ID_TIMER);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(
        window,
        message,
        wParam,
        lParam
    );
}

// -------------------------------------
// MAIN WINDOWS APPLICATION
// -------------------------------------

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow
) {
    const wchar_t CLASS_NAME[] =
        L"SmartDeliveryApplication";

    WNDCLASSW wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    wc.hbrBackground =
        (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc))
        return 1;

    HWND window = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Smart Delivery - Route Optimization",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        900, 760,
        nullptr, nullptr,
        hInstance, nullptr
    );

    if (!window)
        return 1;

    ShowWindow(window, nCmdShow);
    UpdateWindow(window);

    MSG msg = {};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
