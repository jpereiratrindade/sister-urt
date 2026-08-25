// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/persistence.hpp"
#include "sister/urt/repository.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

namespace {

std::filesystem::path get_test_store_path(std::string_view name) {
    auto p = std::filesystem::temp_directory_path() / ("sister_urt_test_" + std::string{name} + ".json");
    std::error_code ec;
    std::filesystem::remove(p, ec);
    return p;
}

} // namespace

void test_urt_persist_01_atomic_save_and_load() {
    auto test_path = get_test_store_path("persist_01");

    {
        sister::urt::UrtRepository repo(test_path, std::nullopt);
        assert(repo.total() == 0);

        sister::urt::UrtRecord u1;
        u1.id = "urt-p1";
        u1.camada_a.codigo_urt = "URT-P1";
        u1.camada_a.nome_local = "Unidade Persistente 1";
        u1.camada_a.instituicao_referencia = "EPAGRI";
        u1.camada_a.municipio = "Lages";
        u1.camada_a.uf = "SC";
        u1.camada_a.coordenadas.latitude = -27.8159;
        u1.camada_a.coordenadas.longitude = -50.3261;
        u1.camada_a.coordenadas.valida = true;

        std::string err;
        bool ok = repo.adicionar(u1, &err);
        assert(ok);
        (void)ok;
        assert(repo.total() == 1);
    }

    // Instância destruída. Agora recria o repositório a partir do arquivo persistido.
    {
        sister::urt::UrtRepository repo_reloaded(test_path, std::nullopt);
        assert(repo_reloaded.total() == 1);

        auto found = repo_reloaded.buscar_por_id("urt-p1");
        assert(found.has_value());
        assert(found->camada_a.codigo_urt == "URT-P1");
        assert(found->camada_a.municipio == "Lages");
        assert(found->camada_a.coordenadas.valida);
    }

    std::error_code ec;
    std::filesystem::remove(test_path, ec);
}

void test_urt_restart_01_lifecycle_and_transitions() {
    auto test_path = get_test_store_path("restart_01");

    {
        sister::urt::UrtRepository repo(test_path, std::nullopt);
        sister::urt::UrtRecord u;
        u.id = "urt-restart";
        u.versao = 1;
        u.status_validacao = sister::urt::StatusValidacao::Preliminar;
        u.camada_a.codigo_urt = "URT-RST";
        u.camada_a.nome_local = "URT Ciclo de Vida";
        u.camada_a.instituicao_referencia = "EMBRAPA-CPPSUL";
        u.camada_a.municipio = "Bagé";
        u.camada_a.uf = "RS";
        repo.adicionar(u);

        // Executa transição governada
        auto res = repo.transicionar("urt-restart", sister::urt::StatusValidacao::Validado,
                                     "Dr. Helio Tonini (CPPSul)", "Validação técnica de campo aprovada");
        assert(res.sucesso);
        assert(res.recibo.para_status == sister::urt::StatusValidacao::Validado);
    }

    // Simula restart do processo
    {
        sister::urt::UrtRepository repo_after_restart(test_path, std::nullopt);
        auto found = repo_after_restart.buscar_por_id("urt-restart");
        assert(found.has_value());
        assert(found->versao == 2);
        assert(found->status_validacao == sister::urt::StatusValidacao::Validado);
        assert(found->historico_transicoes.size() == 1);
        assert(found->historico_transicoes[0].autoridade == "Dr. Helio Tonini (CPPSul)");
    }

    std::error_code ec;
    std::filesystem::remove(test_path, ec);
}

void test_urt_hist_01_append_only_history_preservation() {
    auto test_path = get_test_store_path("hist_01");

    sister::urt::UrtRepository repo(test_path, std::nullopt);
    sister::urt::UrtRecord u;
    u.id = "urt-hist";
    u.camada_a.codigo_urt = "URT-HIST";
    u.camada_a.nome_local = "URT Teste Histórico";
    u.camada_a.instituicao_referencia = "EMATER-RS";
    u.camada_a.municipio = "Passo Fundo";
    u.camada_a.uf = "RS";
    repo.adicionar(u);

    // Transição 1: Preliminar -> RequerRevisao
    auto t1 = repo.transicionar("urt-hist", sister::urt::StatusValidacao::RequerRevisao,
                                "Auditor A", "Ajustar dados de pastagem");
    assert(t1.sucesso);

    // Transição 2: RequerRevisao -> Validado
    auto t2 = repo.transicionar("urt-hist", sister::urt::StatusValidacao::Validado,
                                "Auditor B", "Dados corrigidos e validados");
    assert(t2.sucesso);

    // Agora tenta fazer uma atualização normal (PUT) passando um UrtRecord sem histórico
    auto current = *repo.buscar_por_id("urt-hist");
    assert(current.historico_transicoes.size() == 2);

    current.camada_a.nome_local = "URT Teste Histórico Renomeada";
    current.historico_transicoes.clear(); // Tentativa de apagar o histórico na atualização

    std::string err;
    bool ok = repo.atualizar(current, &err);
    assert(ok);
    (void)ok;

    // Verifica que o repositório PRESERVOU os 2 recibos de forma append-only
    auto updated = *repo.buscar_por_id("urt-hist");
    assert(updated.camada_a.nome_local == "URT Teste Histórico Renomeada");
    assert(updated.historico_transicoes.size() == 2);
    assert(updated.historico_transicoes[0].autoridade == "Auditor A");
    assert(updated.historico_transicoes[1].autoridade == "Auditor B");

    std::error_code ec;
    std::filesystem::remove(test_path, ec);
}

int main() {
    test_urt_persist_01_atomic_save_and_load();
    test_urt_restart_01_lifecycle_and_transitions();
    test_urt_hist_01_append_only_history_preservation();
    std::cout << "[PASS] URT-PERSIST-01, URT-RESTART-01, URT-HIST-01 passaram com sucesso.\n";
    return 0;
}
