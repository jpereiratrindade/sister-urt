// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sister/urt/types.hpp"

#include <string>
#include <string_view>

namespace sister::urt {

struct TransitionRequest {
    std::string urt_id;
    StatusValidacao novo_status{StatusValidacao::Preliminar};
    std::string autoridade;
    std::string motivo;
    std::string timestamp; // Se vazio, gerado automaticamente
};

struct TransitionResult {
    bool sucesso{false};
    std::string erro;
    TransitoryReceipt recibo;
};

[[nodiscard]] TransitionResult executar_transicao_governada(
    UrtRecord& urt,
    const TransitionRequest& request) noexcept;

[[nodiscard]] std::string gerar_timestamp_iso8601() noexcept;

} // namespace sister::urt
