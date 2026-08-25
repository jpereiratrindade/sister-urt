// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/governance.hpp"
#include "sister/urt/types.hpp"

#include <cassert>
#include <iostream>

void test_transicao_governada() {
    sister::urt::UrtRecord urt;
    urt.id = "urt-01";
    urt.versao = 1;
    urt.status_validacao = sister::urt::StatusValidacao::Preliminar;

    sister::urt::TransitionRequest req{
        .urt_id = "urt-01",
        .novo_status = sister::urt::StatusValidacao::Validado,
        .autoridade = "Eng. Agrônomo Validador",
        .motivo = "Revisão e conformidade técnica atestadas",
        .timestamp = "2026-04-10T12:00:00Z",
    };

    auto res = sister::urt::executar_transicao_governada(urt, req);
    if (!res.sucesso) {
        std::cerr << "Falha na transicao\n";
        std::exit(1);
    }
    if (urt.versao != 2 || urt.status_validacao != sister::urt::StatusValidacao::Validado) {
        std::exit(1);
    }
    if (urt.historico_transicoes.empty()) {
        std::exit(1);
    }

    const auto& receipt = urt.historico_transicoes.front();
    if (receipt.de_status != sister::urt::StatusValidacao::Preliminar ||
        receipt.para_status != sister::urt::StatusValidacao::Validado ||
        receipt.autoridade != "Eng. Agrônomo Validador" ||
        receipt.predecessor_state_id != "urt-01@v1" ||
        receipt.successor_state_id != "urt-01@v2") {
        std::exit(1);
    }

    // Tentativa de transição para o mesmo status deve falhar
    auto res_dupl = sister::urt::executar_transicao_governada(urt, req);
    if (res_dupl.sucesso) {
        std::exit(1);
    }
}

void test_transicao_sem_motivo_ou_autoridade() {
    sister::urt::UrtRecord urt;
    urt.id = "urt-02";
    urt.versao = 1;
    urt.status_validacao = sister::urt::StatusValidacao::Preliminar;

    sister::urt::TransitionRequest req_sem_aut{
        .urt_id = "urt-02",
        .novo_status = sister::urt::StatusValidacao::Validado,
        .autoridade = "",
        .motivo = "Motivo qualquer",
        .timestamp = "",
    };
    auto res1 = sister::urt::executar_transicao_governada(urt, req_sem_aut);
    if (res1.sucesso) {
        std::exit(1);
    }

    sister::urt::TransitionRequest req_sem_mot{
        .urt_id = "urt-02",
        .novo_status = sister::urt::StatusValidacao::Validado,
        .autoridade = "Autoridade",
        .motivo = "",
        .timestamp = "",
    };
    auto res2 = sister::urt::executar_transicao_governada(urt, req_sem_mot);
    if (res2.sucesso) {
        std::exit(1);
    }
}

int main() {
    test_transicao_governada();
    test_transicao_sem_motivo_ou_autoridade();
    std::cout << "[PASS] Todos os testes de governança passaram com sucesso.\n";
    return 0;
}
