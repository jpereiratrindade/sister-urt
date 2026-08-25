// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/http.hpp"
#include "sister/urt/repository.hpp"

#include <iostream>

namespace {

sister::urt::http::Request make_req(std::string method, std::string path, std::string body = "") {
    return sister::urt::http::Request{
        .method = std::move(method),
        .path = std::move(path),
        .query_string = "",
        .headers = {},
        .body = std::move(body),
    };
}

} // namespace

void test_http_health_e_rotas() {
    auto test_path = std::filesystem::temp_directory_path() / "sister_urt_http_test.json";
    std::error_code ec;
    std::filesystem::remove(test_path, ec);
    sister::urt::UrtRepository repo(test_path, std::nullopt);

    sister::urt::UrtRecord u;
    u.id = "urt-test";
    u.camada_a.codigo_urt = "URT-TEST";
    u.camada_a.nome_local = "Unidade Teste";
    u.camada_a.instituicao_referencia = "EPAGRI";
    u.camada_a.municipio = "Lages";
    u.camada_a.uf = "SC";
    repo.adicionar(u);

    sister::urt::http::Application app{repo, "<html>Test</html>"};

    // Teste GET /
    auto r_root = app.handle(make_req("GET", "/"));
    if (r_root.status != 200 || r_root.body != "<html>Test</html>") {
        std::exit(1);
    }

    // Teste GET /api/health
    auto r_health = app.handle(make_req("GET", "/api/health"));
    if (r_health.status != 200 || r_health.body.find("\"status\": \"ready\"") == std::string::npos ||
        r_health.body.find("\"total_cadastros\": 1") == std::string::npos) {
        std::exit(1);
    }

    // Teste GET /api/urts
    auto r_list = app.handle(make_req("GET", "/api/urts"));
    if (r_list.status != 200 || r_list.body.find("URT-TEST") == std::string::npos) {
        std::exit(1);
    }

    // Teste GET /api/urts/{id}
    auto r_item = app.handle(make_req("GET", "/api/urts/urt-test"));
    if (r_item.status != 200 || r_item.body.find("Unidade Teste") == std::string::npos) {
        std::exit(1);
    }

    // Teste GET /api/metrics
    auto r_met = app.handle(make_req("GET", "/api/metrics"));
    if (r_met.status != 200 || r_met.body.find("\"total_cadastros\": 1") == std::string::npos) {
        std::exit(1);
    }

    // Teste GET /_sister/manifest
    auto r_manifest = app.handle(make_req("GET", "/_sister/manifest"));
    if (r_manifest.status != 200 || r_manifest.body.find("sister.subsystem.manifest/1.0.0") == std::string::npos) {
        std::exit(1);
    }

    // Teste GET /_sister/health
    auto r_s_health = app.handle(make_req("GET", "/_sister/health"));
    if (r_s_health.status != 200 || r_s_health.body.find("sister.subsystem.health/1.0.0") == std::string::npos) {
        std::exit(1);
    }

    // Teste GET /_sister/ready
    auto r_ready = app.handle(make_req("GET", "/_sister/ready"));
    if (r_ready.status != 200 || r_ready.body.find("sister.subsystem.readiness/1.0.0") == std::string::npos) {
        std::exit(1);
    }

    // Teste GET /_sister/capabilities
    auto r_caps = app.handle(make_req("GET", "/_sister/capabilities"));
    if (r_caps.status != 200 || r_caps.body.find("urt.cadastro.read") == std::string::npos) {
        std::exit(1);
    }

    // Teste GET /_sister/identity
    auto r_ident = app.handle(make_req("GET", "/_sister/identity"));
    if (r_ident.status != 200 || r_ident.body.find("\"functional_role\": \"domain\"") == std::string::npos) {
        std::exit(1);
    }

    // Teste 404
    auto r_404 = app.handle(make_req("GET", "/api/desconhecido"));
    if (r_404.status != 404) {
        std::exit(1);
    }

    // Teste POST /api/urts/{id}/validate sem autoridade (deve falhar fechado com 400)
    const std::string bad_val_aut = R"({"novo_status":"validado","autoridade":"","motivo":"teste"})";
    auto r_val_no_aut = app.handle(make_req("POST", "/api/urts/urt-test/validate", bad_val_aut));
    if (r_val_no_aut.status != 400 || r_val_no_aut.body.find("fail-closed") == std::string::npos) {
        std::cerr << "Erro: Transição sem autoridade não falhou fechada no HTTP adapter!\n";
        std::exit(1);
    }

    // Teste POST /api/urts/{id}/validate sem motivo (deve falhar fechado com 400)
    const std::string bad_val_mot = R"({"novo_status":"validado","autoridade":"Auditor Teste","motivo":""})";
    auto r_val_no_mot = app.handle(make_req("POST", "/api/urts/urt-test/validate", bad_val_mot));
    if (r_val_no_mot.status != 400 || r_val_no_mot.body.find("fail-closed") == std::string::npos) {
        std::cerr << "Erro: Transição sem motivo não falhou fechada no HTTP adapter!\n";
        std::exit(1);
    }

    // Teste POST /api/urts/{id}/validate com parâmetros válidos (deve ter sucesso 200)
    const std::string ok_val = R"json({"novo_status":"validado","autoridade":"Dr. Moacir Medrado (CNPF)","motivo":"Conformidade técnica verificada"})json";
    auto r_val_ok = app.handle(make_req("POST", "/api/urts/urt-test/validate", ok_val));
    if (r_val_ok.status != 200 || r_val_ok.body.find("rcpt-urt-test") == std::string::npos) {
        std::cerr << "Erro: Transição válida falhou no HTTP adapter!\n";
        std::exit(1);
    }

    std::filesystem::remove(test_path, ec);
}

int main() {
    test_http_health_e_rotas();
    std::cout << "[PASS] Todos os testes HTTP (incluindo fail-closed de autoridade e motivo) passaram com sucesso.\n";
    return 0;
}
