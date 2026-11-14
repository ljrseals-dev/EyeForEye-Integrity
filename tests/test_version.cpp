#include "eye_for_eye/version.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

int main() {
    const auto actual = eye_for_eye::version();
    if (actual != std::string_view{"0.1.0"}) {
        std::cerr << "unexpected version: " << actual << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
