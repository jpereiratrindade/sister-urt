// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/state_machine.hpp"

namespace sister::urt {

bool StateMachine::is_transition_allowed(
    StatusValidacao from,
    StatusValidacao to,
    const TransitionContext& ctx,
    std::string* error_reason) noexcept {

    if (from == to) {
        if (error_reason) *error_reason = "Transição redundante para o mesmo status de validação.";
        return false;
    }

    if (ctx.autoridade.empty()) {
        if (error_reason) *error_reason = "Autoridade responsável é estritamente obrigatória para transição (fail-closed).";
        return false;
    }

    if (ctx.motivo.empty()) {
        if (error_reason) *error_reason = "Motivo/justificativa técnica é estritamente obrigatório para transição (fail-closed).";
        return false;
    }

    switch (from) {
        case StatusValidacao::Preliminar:
            // De Preliminar é permitido ir para RequerRevisao, Validado ou Arquivado
            if (to == StatusValidacao::RequerRevisao ||
                to == StatusValidacao::Validado ||
                to == StatusValidacao::Arquivado) {
                return true;
            }
            break;

        case StatusValidacao::RequerRevisao:
            // De RequerRevisao é permitido ir para Validado, Preliminar ou Arquivado
            if (to == StatusValidacao::Validado ||
                to == StatusValidacao::Preliminar ||
                to == StatusValidacao::Arquivado) {
                return true;
            }
            break;

        case StatusValidacao::Validado:
            // De Validado é permitido ir para RequerRevisao ou Arquivado
            if (to == StatusValidacao::RequerRevisao ||
                to == StatusValidacao::Arquivado) {
                return true;
            }
            if (to == StatusValidacao::Preliminar) {
                if (error_reason) *error_reason = "Status validado não pode retroceder direto para preliminar sem passar por revisão.";
                return false;
            }
            break;

        case StatusValidacao::Arquivado:
            // De Arquivado só é permitido reabrir para Preliminar
            if (to == StatusValidacao::Preliminar) {
                return true;
            }
            if (error_reason) *error_reason = "Status arquivado deve ser reaberto primeiro como preliminar para nova caracterização.";
            return false;
    }

    if (error_reason) *error_reason = "Transição de validação não permitida pela política de governança de domínio.";
    return false;
}

} // namespace sister::urt
