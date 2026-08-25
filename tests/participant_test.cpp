// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/participant.hpp"
#include "sister/urt/repository.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

namespace {

std::filesystem::path get_test_store_path(std::string_view name) {
    auto p = std::filesystem::temp_directory_path() / ("sister_urt_part_" + std::string{name} + ".json");
    std::error_code ec;
    std::filesystem::remove(p, ec);
    return p;
}

} // namespace

void test_urt_participant_01_semantic_boundary() {
    auto test_path = get_test_store_path("sem_01");
    sister::urt::UrtRepository repo(test_path, std::nullopt);
    sister::urt::UrtParticipant part(repo, "contracts/system_manifest.json");

    // who_are_you()
    auto id = part.who_are_you();
    assert(id.system_id == "sister_urt");
    assert(id.functional_role == "domain");
    assert(!id.domain_authority.empty());
    assert(id.history_model == "append_only_transition_history");

    // what_can_you_do()
    auto caps = part.what_can_you_do();
    assert(caps.size() >= 5);
    bool has_read = false, has_transition = false;
    for (const auto& c : caps) {
        if (c.id == "urt.cadastro.read") has_read = true;
        if (c.id == "urt.validation.transition") has_transition = true;
    }
    assert(has_read);
    assert(has_transition);
    (void)has_read;
    (void)has_transition;

    // what_do_you_own()
    auto boundary = part.what_do_you_own();
    assert(boundary.domain_name == "unidades_referencia_tecnologica");
    assert(boundary.exclusive_authority);

    // what_are_your_limits()
    auto limits = part.what_are_your_limits();
    assert(!limits.empty());

    // what_roles_can_you_play()
    auto roles = part.what_roles_can_you_play();
    assert(!roles.empty());
    assert(roles[0] == "domain");

    std::error_code ec;
    std::filesystem::remove(test_path, ec);
}

void test_urt_manifest_01_canonical_and_digest() {
    auto test_path = get_test_store_path("man_01");
    sister::urt::UrtRepository repo(test_path, std::nullopt);
    sister::urt::UrtParticipant part(repo, "contracts/system_manifest.json");

    auto manifest = part.get_canonical_manifest_json();
    assert(manifest.find("\"system_id\": \"sister_urt\"") != std::string::npos);
    assert(manifest.find("\"schema\": \"sister.subsystem.manifest/1.0.0\"") != std::string::npos);

    auto digest = part.get_manifest_digest();
    assert(digest.rfind("sha256:", 0) == 0);
    assert(digest.size() > 10);

    std::error_code ec;
    std::filesystem::remove(test_path, ec);
}

void test_urt_ready_01_observed_readiness() {
    auto test_path = get_test_store_path("ready_01");
    sister::urt::UrtRepository repo(test_path, std::nullopt);
    sister::urt::UrtParticipant part(repo, "contracts/system_manifest.json");

    auto ready_state = part.what_is_your_state();
    assert(ready_state.status == "ready");
    assert(ready_state.storage_ready);
    assert(ready_state.repository_ready);
    assert(ready_state.governance_active);
    assert(!ready_state.manifest_digest.empty());

    std::error_code ec;
    std::filesystem::remove(test_path, ec);
}

void test_urt_capability_01_transport_neutral_invocation() {
    auto test_path = get_test_store_path("cap_01");
    sister::urt::UrtRepository repo(test_path, std::nullopt);
    sister::urt::UrtParticipant part(repo, "contracts/system_manifest.json");

    // Invocação de metrics via capability
    auto r1 = part.invoke_capability("urt.metrics.evaluate");
    assert(r1.success);
    assert(r1.status_code == 200);
    assert(r1.payload.find("\"total_cadastros\": 0") != std::string::npos);

    // Invocação de cadastro via capability
    const std::string new_urt_json = R"json({
  "codigo_urt": "URT-CAP-01",
  "nome_local": "URT Criada via Capability",
  "instituicao_referencia": "EPAGRI",
  "municipio": "Lages",
  "uf": "SC",
  "latitude": -27.81,
  "longitude": -50.32,
  "tipo_sistema": "SP"
})json";

    auto r2 = part.invoke_capability("urt.cadastro.create", new_urt_json);
    assert(r2.success);
    assert(r2.status_code == 201);
    assert(repo.total() == 1);

    // Invocação de export via capability
    auto r3 = part.invoke_capability("urt.dataset.export");
    assert(r3.success);
    assert(r3.payload.find("URT-CAP-01") != std::string::npos);

    std::error_code ec;
    std::filesystem::remove(test_path, ec);
}

int main() {
    test_urt_participant_01_semantic_boundary();
    test_urt_manifest_01_canonical_and_digest();
    test_urt_ready_01_observed_readiness();
    test_urt_capability_01_transport_neutral_invocation();
    std::cout << "[PASS] URT-PARTICIPANT-01, URT-MANIFEST-01, URT-READY-01, URT-CAPABILITY-01 passaram com sucesso.\n";
    return 0;
}
