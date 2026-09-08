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
    std::string ExpectedSeverity = "";
};

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
    std::set<std::string> RaceSeverities;
};

static ActualResult ParseAnalyzerOutput(const std::string &Output) {
    ActualResult Res;

    std::istringstream Stream(Output);
    std::string Line;

    // --- RACE / NO_RACE sekcija (uvek prva) ---
    if (!std::getline(Stream, Line)) return Res;

    if (Line == "RACE") {
        Res.HasRace = true;

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
    } else if (Line == "NO_RACE") {
        Res.HasRace = false;
        if (!std::getline(Stream, Line)) return Res;
    } else {
        // Neocekivan format (npr. stari analyzer bez race izlaza) -
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
        {"tests/deadlock/recursive_fixed_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/rwlock_shared_must_not_protect_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/rwlock_write_deadlock.c", true, {{"rw1","rw2"}, {"rw2","rw1"}}},
        {"tests/deadlock/all_same_thread_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/array_locks_deadlock.c", true, {{"locks[0]","locks[1]"}, {"locks[1]","locks[0]"}}},
        {"tests/deadlock/basic_deadlock.c", true, {{"lock1","lock2"}, {"lock2","lock1"}}},
        {"tests/deadlock/branching_deadlock.c", true, {{"m2","m3"}, {"m3","m2"}}},
        {"tests/deadlock_safe/single_thread_sequential_no_deadlock.c", false, {}},
        {"tests/deadlock/conditional_unlock_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/create_without_join_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/deref_dot_normalization_deadlock.c", true, {{"p.lock1","p.lock2"}, {"p.lock2","p.lock1"}}},
        {"tests/deadlock/disconnected_clusters_deadlock.c", true, {{"a1","a2"}, {"a2","a1"}, {"b1","b2"}, {"b2","b1"}}},
        {"tests/deadlock/interprocedural_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/loop_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/no_value_analysis_false_branch_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/must_lockset_gap.c", true, {{"m2","m3"}, {"m3","m2"}}},
        {"tests/deadlock/nested_call_same_block_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/nested_wrapper_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/test_loop_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock_safe/main_before_create_no_deadlock.c", false, {}},
        {"tests/deadlock/three_way_cycle_deadlock.c", true, {{"m1","m2"}, {"m2","m3"}, {"m3","m1"}}},
        {"tests/deadlock/trylock_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/wrapper_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock/struct_locks_deadlock.c", true, {{"res.lock1","res.lock2"}, {"res.lock2","res.lock1"}}},
        {"tests/deadlock/switch_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock_safe/main_before_create_in_loop_no_deadlock.c", false, {}},
        {"tests/deadlock/creation_order_through_interprocedural_call_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/race/basic_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race/must_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race/may_race.c", false, {}, true, {"x"}, "MAY"},
        {"tests/race/loop_race.c", false, {}, true, {"x"}, "MAY"},
        {"tests/race/siblings_threads_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race/loop_join_still_race.c", false, {}, true, {"x"}, "MAY"},
        {"tests/race_safe/protected_no_race.c", false, {}, false, {}},
        {"tests/race_safe/main_before_create_race.c", false, {}, false, {}},
        {"tests/race_safe/join_removes_race.c", false, {}, false, {}},
        {"tests/race/write_under_rdlock_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race/different_locks_still_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race/compound_assignment_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race/grandchild_thread_race.c", false, {}, true, {"x"}, "MAY"},
        {"tests/race_safe/shadowed_local_no_race.c", false, {}, false, {}},
        {"tests/race/write_read_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race/read_in_condition_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race/read_as_call_arg_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race_safe/protected_read_write_no_race.c", false, {}, false, {}},
        {"tests/race_safe/both_only_read_no_race.c", false, {}, false, {}},
        {"tests/race/conditional_creation_same_branch_must_race.c", false, {}, true, {"x"}, "MUST"},
        {"tests/race_safe/different_vars_no_race.c", false, {}, false, {}},
        {"tests/race_safe/correct_rwlock_usage_no_race.c", false, {}, false, {}},
        {"tests/race/struct_field_race.c", false, {}, true, {"d.val"}, "MUST"},
        {"tests/race_safe/shadowed_local_struct_no_race.c", false, {}, false, {}},
        {"tests/race/conditional_lock_hidden_race.c", false, {}, true, {"x"}, "MAY"},
        {"tests/deadlock/handle_merge_lost_deadlock.c", true, {{"m1","m2"}, {"m2","m1"}}},
        {"tests/deadlock_safe/local_mutex_name_collision_no_deadlock.c", false, {}},
        {"tests/deadlock_safe/rwlock_both_read_no_deadlock.c", false, {}},
        {"tests/combined/kitchen_sink_combined.c", true, {{"a1","a2"}, {"a2","a1"}, {"b1","b2"}, {"b2","b3"}, {"b3","b1"}}, true, {"x","y"}, "MUST"},
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