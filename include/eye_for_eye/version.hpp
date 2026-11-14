#pragma once

#include <string_view>

namespace eye_for_eye {

/// Return the semantic version of this repository baseline.
std::string_view version() noexcept;

} // namespace eye_for_eye
