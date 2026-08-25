// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/governance.hpp"
#include "sister/urt/state_machine.hpp"

#include <chrono>
#include <ctime>
#include <format>
#include <iomanip>
#include <sstream>

namespace sister::urt {

std::string gerar_timestamp_iso8601() noexcept {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    gmtime_r(&time, &tm_buf);

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    return std::string{buffer};
}

TransitionResult executar_transicao_governada(
    UrtRecord& urt,
    const TransitionRequest& request) noexcept {

    TransitionResult result;

    const auto status_anterior = urt.status_validacao;
    TransitionContext ctx{
        .autoridade = request.autoridade,
        .motivo = request.motivo,
        .role_or_scope = ""
    };

    if (!StateMachine::is_transition_allowed(status_anterior, request.novo_status, ctx, &result.erro)) {
        result.sucesso = false;
        return result;
    }

    const std::string timestamp = request.timestamp.empty() ? gerar_timestamp_iso8601() : request.timestamp;
    const std::string predecessor_state_id = urt.id + "@v" + std::to_string(urt.versao);

    // Avança versão
    urt.versao += 1;
    urt.status_validacao = request.novo_status;
    const std::string successor_state_id = urt.id + "@v" + std::to_string(urt.versao);

    // Gera o recibo de transição
    TransitoryReceipt recibo{
        .id = "rcpt-" + urt.id + "-v" + std::to_string(urt.versao),
        .timestamp = timestamp,
        .de_status = status_anterior,
        .para_status = request.novo_status,
        .autoridade = request.autoridade,
        .motivo = request.motivo,
        .predecessor_state_id = predecessor_state_id,
        .successor_state_id = successor_state_id,
    };

    urt.historico_transicoes.push_back(recibo);

    result.sucesso = true;
    result.recibo = recibo;
    return result;
}

} // namespace sister::urt
