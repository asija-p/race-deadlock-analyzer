#include <clang/Basic/Version.h>
#include <iostream>

int main() {
    std::cout << "Clang version: " << clang::getClangFullVersion() << std::endl;
    return 0;
}