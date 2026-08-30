#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <utility>
#include <sstream>
#include <cstdio>
#include <memory>
#include <array>

struct TestCase {
    std::string FilePath;
    bool ExpectDeadlock;
    std::set<std::pair<std::string,std::string>> ExpectedEdges;
};

// Pokrece komandu kao subprocess, vraca njen ceo stdout izlaz kao string
static std::string RunCommand(const std::string &Command) {
    std::array<char, 256> Buffer;
    std::string Result;
    std::unique_ptr<FILE, decltype(&pclose)> Pipe(popen(Command.c_str(), "r"), pclose);
    if (!Pipe) {
        return "";
    }
    while (fgets(Buffer.data(), Buffer.size(), Pipe.get()) != nullptr) {
        Result += Buffer.data();
    }
    return Result;
}

struct ActualResult {
    bool HasDeadlock;
    std::set<std::pair<std::string,std::string>> Edges;
};

// Parsira izlaz analyzer-a (--quiet mod): "DEADLOCK\nm1->m2,m2->m1\n" ili "SAFE\n"
static ActualResult ParseAnalyzerOutput(const std::string &Output) {
    ActualResult Res;
    Res.HasDeadlock = false;

    std::istringstream Stream(Output);
    std::string FirstLine;
    std::getline(Stream, FirstLine);

    if (FirstLine == "DEADLOCK") {
        Res.HasDeadlock = true;
        std::string EdgeLine;
        std::getline(Stream, EdgeLine);

        std::istringstream EdgeStream(EdgeLine);
        std::string OnePair;
        while (std::getline(EdgeStream, OnePair, ',')) {
            size_t ArrowPos = OnePair.find("->");
            if (ArrowPos != std::string::npos) {
                std::string From = OnePair.substr(0, ArrowPos);
                std::string To = OnePair.substr(ArrowPos + 2);
                Res.Edges.insert({From, To});
            }
        }
    }

    return Res;
}

static void PrintEdgeSet(const std::set<std::pair<std::string,std::string>> &S) {
    std::cout << "{";
    bool First = true;
    for (const auto &Edge : S) {
        if (!First) std::cout << ", ";
        std::cout << Edge.first << "->" << Edge.second;
        First = false;
    }
    std::cout << "}";
}

int main(int argc, char** argv) {
    std::vector<TestCase> Tests = {
        {"tests/interprocedular_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/common_lock_example.c", false, {}},
        {"tests/branching_deadlock.c", true, {{"m2","m3"}, {"m3","m2"}}},
        {"tests/deadcode_example.c", false, {}},
        {"tests/wrapper_example.c", true, {{"m1","m2"}, {"m2","m1"}}},
    };

    std::string Filter;
    if (argc >= 2) {
        Filter = argv[1];
    }

    std::vector<TestCase> TestsToRun;
    if (Filter.empty() || Filter == "all") {
        TestsToRun = Tests;
    } else {
        for (const TestCase &Test : Tests) {
            // Poklapanje ako se Filter nalazi negde u putanji (npr. "wrapper" pronadje wrapper_example.c)
            if (Test.FilePath.find(Filter) != std::string::npos) {
                TestsToRun.push_back(Test);
            }
        }
        if (TestsToRun.empty()) {
            std::cout << "Nijedan test se ne poklapa sa: " << Filter << "\n";
            return 1;
        }
    }

    int Passed = 0;
    int Total = TestsToRun.size();

    for (const TestCase &Test : TestsToRun) {
        std::string Command = "./build/analyzer --quiet " + Test.FilePath;
        std::string Output = RunCommand(Command);
        ActualResult Actual = ParseAnalyzerOutput(Output);

        bool Match = (Actual.HasDeadlock == Test.ExpectDeadlock);
        if (Match && Test.ExpectDeadlock) {
            Match = (Actual.Edges == Test.ExpectedEdges);
        }

        if (Match) {
            std::cout << "[PASS] " << Test.FilePath << "\n";
            std::cout << "  Dobijeno: " << (Actual.HasDeadlock ? "DEADLOCK, ciklus " : "SAFE");
            if (Actual.HasDeadlock) PrintEdgeSet(Actual.Edges);
            std::cout << "\n";
            Passed++;
        } else {
            std::cout << "[FAIL] " << Test.FilePath << "\n";
            std::cout << "  Dobijeno:  " << (Actual.HasDeadlock ? "DEADLOCK, ciklus " : "SAFE");
            if (Actual.HasDeadlock) PrintEdgeSet(Actual.Edges);
            std::cout << "\n";
            std::cout << "  Ocekivano: " << (Test.ExpectDeadlock ? "DEADLOCK, ciklus " : "SAFE");
            if (Test.ExpectDeadlock) PrintEdgeSet(Test.ExpectedEdges);
            std::cout << "\n";
        }
    }

    std::cout << "\n" << Passed << "/" << Total << " testova proslo\n";
    return (Passed == Total) ? 0 : 1;
}