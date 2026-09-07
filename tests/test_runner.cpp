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
    bool ExpectRace = false;
    std::set<std::string> ExpectedRaceVars;
    // Ako je ExpectRace true, ovo je ocekivana ozbiljnost ("MUST" ili "MAY").
    // Prazan string = ne proveravaj ozbiljnost (za stare testove bez ovog polja).
    std::string ExpectedSeverity = "";
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
    bool HasDeadlock = false;
    std::set<std::pair<std::string,std::string>> Edges;
    bool HasRace = false;
    std::set<std::string> RaceVars;
    // Skup svih ozbiljnosti vidjenih u izlazu (moze imati i "MUST" i "MAY"
    // ako ima vise race parova razlicite ozbiljnosti).
    std::set<std::string> RaceSeverities;
};

// Parsira izlaz analyzer-a (--quiet mod):
// "RACE\nx|6|create_line_12|MUST\nx|13|main|MUST\nDEADLOCK\nm1->m2,m2->m1\n"
// ili "NO_RACE\nSAFE\n" (i sve kombinacije izmedju).
static ActualResult ParseAnalyzerOutput(const std::string &Output) {
    ActualResult Res;

    std::istringstream Stream(Output);
    std::string Line;

    // --- RACE / NO_RACE sekcija (uvek prva) ---
    if (!std::getline(Stream, Line)) return Res;

    if (Line == "RACE") {
        Res.HasRace = true;
        // Cita RACE parove dok ne naidje na DEADLOCK/SAFE liniju (pocetak
        // sledece sekcije), koju onda mora da "vrati" na obradu ispod.
        while (std::getline(Stream, Line)) {
            if (Line == "DEADLOCK" || Line == "SAFE") {
                break;
            }
            if (Line.empty()) continue;

            // Format: promenljiva|linija|thread|ozbiljnost
            size_t Sep1 = Line.find('|');
            if (Sep1 == std::string::npos) continue;
            Res.RaceVars.insert(Line.substr(0, Sep1));

            size_t Sep2 = Line.find('|', Sep1 + 1);
            size_t Sep3 = (Sep2 == std::string::npos) ? std::string::npos
                                                        : Line.find('|', Sep2 + 1);
            if (Sep3 != std::string::npos) {
                Res.RaceSeverities.insert(Line.substr(Sep3 + 1));
            }
        }
        // "Line" sad sadrzi DEADLOCK/SAFE liniju (ili je stream prazan) -
        // nastavljamo obradu od nje dole, bez ponovnog getline.
    } else if (Line == "NO_RACE") {
        Res.HasRace = false;
        if (!std::getline(Stream, Line)) return Res;
    } else {
        // Neocekivan format (npr. stari analyzer bez race izlaza) -
        // tretiraj ovu liniju kao pocetak DEADLOCK/SAFE sekcije direktno,
        // da stariji testovi i dalje rade bez izmene.
    }

    // --- DEADLOCK / SAFE sekcija ---
    if (Line == "DEADLOCK") {
        Res.HasDeadlock = true;
        std::string EdgeLine;
        while (std::getline(Stream, EdgeLine)) {
            if (EdgeLine.empty()) continue;
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

static void PrintStringSet(const std::set<std::string> &S) {
    std::cout << "{";
    bool First = true;
    for (const auto &V : S) {
        if (!First) std::cout << ", ";
        std::cout << V;
        First = false;
    }
    std::cout << "}";
}

int main(int argc, char** argv) {
    std::vector<TestCase> Tests = {
        {"tests/deadlock/interprocedural_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/safe/common_lock_safe.c", false, {}},
        {"tests/deadlock/branching_deadlock.c", true, {{"m2","m3"}, {"m3","m2"}}},
        {"tests/safe/deadcode_safe.c", false, {}},
        {"tests/deadlock/wrapper_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/safe/thread_loop_safe.c", false, {}},
        {"tests/deadlock/thread_loop_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/trylock_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/array_locks_deadlock.c", true, {{"locks[0]","locks[1]"}, {"locks[1]","locks[0]"}}},
        {"tests/deadlock/struct_locks_deadlock.c", true, {{"res.lock1","res.lock2"}, {"res.lock2","res.lock1"}}},
        {"tests/deadlock/must_lockset_gap.c", true, {{"m2","m3"}, {"m3","m2"}}},
        {"tests/deadlock/nested_wrapper_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/switch_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/disconnected_clusters_deadlock.c", true, {{"a1","a2"}, {"a2","a1"}, {"b1","b2"}, {"b2","b1"}}},
        {"tests/deadlock/conditional_unlock_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/loop_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/three_way_cycle_deadlock.c", true, {{"m1","m2"}, {"m2","m3"}, {"m3","m1"}}},
        {"tests/safe/sequential_calls_safe.c", false, {}},
        {"tests/deadlock/rwlock_write_deadlock.c", true, {{"rw1","rw2"}, {"rw2","rw1"}}},
        {"tests/safe/rwlock_readers_safe.c", false, {}},
        {"tests/deadlock/rwlock_shared_must_not_protect_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/deref_dot_normalization_deadlock.c", true, {{"p.lock1","p.lock2"}, {"p.lock2","p.lock1"}}},   
        {"tests/safe/join_removes_false_deadlock_safe.c", false, {}},
        {"tests/deadlock/create_without_join_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/all_same_thread_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/safe/join_before_branch_still_safe.c", false, {}},
        {"tests/deadlock/test_loop_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/nested_call_same_block_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/race/basic_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race/must_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race/protected_no_race.c", false, {}, false, {}},
        {"tests/race/may_race.c", false, {}, true, {"x"}, "MAY"},
        {"tests/race/loop_race.c", false, {}, true, {"x"}, "MAY"},
        {"tests/safe/same_thread_safe.c", false, {}},
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

        bool DeadlockMatch = (Actual.HasDeadlock == Test.ExpectDeadlock);
        if (DeadlockMatch && Test.ExpectDeadlock) {
            DeadlockMatch = (Actual.Edges == Test.ExpectedEdges);
        }

        bool RaceMatch = (Actual.HasRace == Test.ExpectRace);
        if (RaceMatch && Test.ExpectRace) {
            RaceMatch = (Actual.RaceVars == Test.ExpectedRaceVars);
            if (RaceMatch && !Test.ExpectedSeverity.empty()) {
                RaceMatch = Actual.RaceSeverities.count(Test.ExpectedSeverity) > 0;
            }
        }

        bool Match = DeadlockMatch && RaceMatch;

        if (Match) {
            std::cout << "[PASS] " << Test.FilePath << "\n";
            std::cout << "  Deadlock: " << (Actual.HasDeadlock ? "DEADLOCK, ciklus " : "SAFE");
            if (Actual.HasDeadlock) PrintEdgeSet(Actual.Edges);
            std::cout << "\n";
            std::cout << "  Race: " << (Actual.HasRace ? "RACE, promenljive " : "NO_RACE");
            if (Actual.HasRace) {
                PrintStringSet(Actual.RaceVars);
                std::cout << " (";
                PrintStringSet(Actual.RaceSeverities);
                std::cout << ")";
            }
            std::cout << "\n";
            Passed++;
        } else {
            std::cout << "[FAIL] " << Test.FilePath << "\n";
            std::cout << "  Dobijeno  - Deadlock: " << (Actual.HasDeadlock ? "DEADLOCK, ciklus " : "SAFE");
            if (Actual.HasDeadlock) PrintEdgeSet(Actual.Edges);
            std::cout << "\n";
            std::cout << "  Ocekivano - Deadlock: " << (Test.ExpectDeadlock ? "DEADLOCK, ciklus " : "SAFE");
            if (Test.ExpectDeadlock) PrintEdgeSet(Test.ExpectedEdges);
            std::cout << "\n";
            std::cout << "  Dobijeno  - Race: " << (Actual.HasRace ? "RACE, promenljive " : "NO_RACE");
            if (Actual.HasRace) {
                PrintStringSet(Actual.RaceVars);
                std::cout << " (";
                PrintStringSet(Actual.RaceSeverities);
                std::cout << ")";
            }
            std::cout << "\n";
            std::cout << "  Ocekivano - Race: " << (Test.ExpectRace ? "RACE, promenljive " : "NO_RACE");
            if (Test.ExpectRace) {
                PrintStringSet(Test.ExpectedRaceVars);
                if (!Test.ExpectedSeverity.empty()) {
                    std::cout << " (" << Test.ExpectedSeverity << ")";
                }
            }
            std::cout << "\n";
        }
    }

    std::cout << "\n" << Passed << "/" << Total << " testova proslo\n";
    return (Passed == Total) ? 0 : 1;
}