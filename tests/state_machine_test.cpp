// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/state_machine.hpp"
#include "sister/urt/governance.hpp"

#include <cassert>
#include <iostream>

using namespace sister::urt;

void test_urt_state_01_matrix() {
    TransitionContext ctx{
        .autoridade = "Dr. Moacir Medrado (CNPF)",
        .motivo = "Auditoria técnica de conformidade realizada em campo.",
        .role_or_scope = "domain_evaluator"
    };

    std::string err;

    // Mesmo status -> proibido
    assert(!sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::Preliminar,
        sister::urt::StatusValidacao::Preliminar,
        ctx, &err));

    // Preliminar -> RequerRevisao (OK)
    assert(sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::Preliminar,
        sister::urt::StatusValidacao::RequerRevisao,
        ctx, &err));

    // Preliminar -> Validado (OK)
    assert(sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::Preliminar,
        sister::urt::StatusValidacao::Validado,
        ctx, &err));

    // Preliminar -> Arquivado (OK)
    assert(sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::Preliminar,
        sister::urt::StatusValidacao::Arquivado,
        ctx, &err));

    // RequerRevisao -> Validado (OK)
    assert(sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::RequerRevisao,
        sister::urt::StatusValidacao::Validado,
        ctx, &err));

    // RequerRevisao -> Preliminar (OK)
    assert(sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::RequerRevisao,
        sister::urt::StatusValidacao::Preliminar,
        ctx, &err));

    // Validado -> RequerRevisao (OK)
    assert(sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::Validado,
        sister::urt::StatusValidacao::RequerRevisao,
        ctx, &err));

    // Validado -> Preliminar (Proibido direto sem revisão)
    assert(!sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::Validado,
        sister::urt::StatusValidacao::Preliminar,
        ctx, &err));

    // Arquivado -> Preliminar (OK - reabertura)
    assert(sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::Arquivado,
        sister::urt::StatusValidacao::Preliminar,
        ctx, &err));

    // Arquivado -> Validado (Proibido)
    assert(!sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::Arquivado,
        sister::urt::StatusValidacao::Validado,
        ctx, &err));
}

void test_urt_auth_01_fail_closed_autoridade() {
    TransitionContext ctx_sem_aut{
        .autoridade = "",
        .motivo = "Justificativa presente",
        .role_or_scope = ""
    };
    std::string err;
    assert(!sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::Preliminar,
        sister::urt::StatusValidacao::Validado,
        ctx_sem_aut, &err));
    assert(!err.empty());
}

void test_urt_auth_02_fail_closed_motivo() {
    TransitionContext ctx_sem_mot{
        .autoridade = "Helio Tonini (CPPSul)",
        .motivo = "",
        .role_or_scope = ""
    };
    std::string err;
    assert(!sister::urt::StateMachine::is_transition_allowed(
        sister::urt::StatusValidacao::Preliminar,
        sister::urt::StatusValidacao::Validado,
        ctx_sem_mot, &err));
    assert(!err.empty());
}

int main() {
    test_urt_state_01_matrix();
    test_urt_auth_01_fail_closed_autoridade();
    test_urt_auth_02_fail_closed_motivo();
    std::cout << "[PASS] URT-STATE-01, URT-AUTH-01, URT-AUTH-02 passaram com sucesso.\n";
    return 0;
}
