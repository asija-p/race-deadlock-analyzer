cmake_minimum_required(VERSION 3.20)
project(RaceDeadlockAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Found Clang: ${CLANG_INSTALL_PREFIX}")

add_executable(analyzer
    src/main.cpp
    src/frontend/CallVisitor.cpp
    src/frontend/CFGPrinter.cpp
    src/frontend/ASTConsumer.cpp
    src/analysis/LockOrderAnalyzer.cpp
)

target_include_directories(analyzer PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
    src
    src/frontend
    src/analysis
)

target_link_libraries(analyzer PRIVATE clang-cpp LLVM)