// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sister/urt/types.hpp"

#include <string>
#include <string_view>

namespace sister::urt {

struct TransitionContext {
    std::string autoridade;
    std::string motivo;
    std::string role_or_scope;
};

class StateMachine {
public:
    [[nodiscard]] static bool is_transition_allowed(
        StatusValidacao from,
        StatusValidacao to,
        const TransitionContext& ctx,
        std::string* error_reason = nullptr) noexcept;
};

} // namespace sister::urt
