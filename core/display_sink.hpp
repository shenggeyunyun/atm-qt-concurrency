#pragma once
#include <functional>
#include <string>

namespace atm_core {
    using display_sink = std::function<void(std::string const&)>;
}
