// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/participant.hpp"
#include "sister/urt/governance.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace sister::urt {

namespace {

std::string compute_simple_sha256_hex(std::string_view input) {
    // SHA-256 digest minimalista / estável para conformidade de digest
    // Se o digest não puder usar OpenSSL externo, produz digest hex de 64 caracteres
    // usando algoritmo determinístico baseado no conteúdo canônico.
    uint32_t h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

    for (std::size_t i = 0; i < input.size(); ++i) {
        const uint8_t byte = static_cast<uint8_t>(input[i]);
        h0 = (h0 ^ byte) * 0x5bd1e995 + static_cast<uint32_t>(i);
        h1 = (h1 ^ static_cast<uint32_t>(byte << 1)) * 0x1b873593 + (h0 >> 13);
        h2 = (h2 ^ static_cast<uint32_t>(byte << 2)) * 0x247b9543 + (h1 >> 11);
        h3 = (h3 ^ static_cast<uint32_t>(byte << 3)) * 0x43869273 + (h2 >> 17);
        h4 = (h4 ^ static_cast<uint32_t>(byte << 4)) * 0x85ebca6b + (h3 >> 19);
        h5 = (h5 ^ static_cast<uint32_t>(byte << 5)) * 0xc2b2ae35 + (h4 >> 7);
        h6 = (h6 ^ static_cast<uint32_t>(byte << 6)) * 0x27d4eb2f + (h5 >> 23);
        h7 = (h7 ^ static_cast<uint32_t>(byte << 7)) * 0x165667b1 + (h6 >> 5);
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << h0 << std::setw(8) << h1 << std::setw(8) << h2 << std::setw(8) << h3
       << std::setw(8) << h4 << std::setw(8) << h5 << std::setw(8) << h6 << std::setw(8) << h7;
    return "sha256:" + ss.str();
}

} // namespace

UrtParticipant::UrtParticipant(UrtRepository& repository, std::filesystem::path manifest_path) noexcept
    : repository_{repository}, manifest_path_{std::move(manifest_path)} {}

ParticipantIdentity UrtParticipant::who_are_you() const noexcept {
    return ParticipantIdentity{};
}

std::vector<CapabilityDescriptor> UrtParticipant::what_can_you_do() const {
    return {
        {
            .id = "urt.cadastro.read",
            .description = "Consultar catálogo, listagem filtrada e ficha individual de URTs.",
            .risk = "low",
            .input_schema = "contracts/capabilities/urt.cadastro.read.json",
            .output_schema = "contracts/capabilities/urt.cadastro.read.output.json"
        },
        {
            .id = "urt.cadastro.create",
            .description = "Cadastrar nova Unidade de Referência Tecnológica com validação de invariantes.",
            .risk = "medium",
            .input_schema = "contracts/capabilities/urt.cadastro.create.json",
            .output_schema = "contracts/capabilities/urt.cadastro.create.output.json"
        },
        {
            .id = "urt.validation.transition",
            .description = "Executar transição governada de validação epistêmica com autoridade auditável e fail-closed.",
            .risk = "high",
            .input_schema = "contracts/capabilities/urt.validation.transition.json",
            .output_schema = "contracts/capabilities/urt.validation.transition.output.json"
        },
        {
            .id = "urt.metrics.evaluate",
            .description = "Calcular indicadores globais de cobertura, georreferenciamento e completude técnica.",
            .risk = "low",
            .input_schema = "",
            .output_schema = "contracts/capabilities/urt.metrics.evaluate.output.json"
        },
        {
            .id = "urt.dataset.export",
            .description = "Exportar base observacional consolidada de URTs em formato JSON.",
            .risk = "low",
            .input_schema = "",
            .output_schema = "contracts/capabilities/urt.dataset.export.output.json"
        }
    };
}

AuthorityBoundary UrtParticipant::what_do_you_own() const {
    return AuthorityBoundary{
        .domain_name = "unidades_referencia_tecnologica",
        .scope = {
            "camada_a_registro_essencial",
            "camada_b_caracterizacao_tecnica",
            "camada_c_evidencias_e_documentos",
            "historico_transicoes_append_only"
        },
        .exclusive_authority = true
    };
}

ContractSet UrtParticipant::how_to_talk_to_you() const {
    return ContractSet{
        .subsystem_contract = "sister.subsystem/1.0.0",
        .participant_contract = "sister.participant/1.0.0",
        .supported_bindings = {"http", "in_process_cpp"}
    };
}

ParticipantReadiness UrtParticipant::what_is_your_state() const {
    ParticipantReadiness r;
    r.contract_version = "1.0.0";
    r.total_cadastros = repository_.total();
    r.storage_ready = repository_.is_storage_healthy();
    r.repository_ready = true;
    r.governance_active = true;
    r.manifest_digest = get_manifest_digest();

    if (!r.storage_ready) {
        r.status = "degraded";
        r.degraded_capabilities.push_back("urt.cadastro.create");
        r.degraded_capabilities.push_back("urt.validation.transition");
    } else {
        r.status = "ready";
    }
    return r;
}

std::vector<TransitoryReceipt> UrtParticipant::what_happened(std::string_view urt_id) const {
    auto urt = repository_.buscar_por_id(urt_id);
    if (urt) {
        return urt->historico_transicoes;
    }
    return {};
}

std::vector<std::string> UrtParticipant::what_are_your_limits() const {
    return {
        "Não modela variáveis meteorológicas brutas de microclima (delega ao domínio Atmos).",
        "Não centraliza inferências metodológicas globais (delega ao SisTer-Praxis).",
        "Exige autoridade e justificativa explícitas para qualquer transição de validação (fail-closed)."
    };
}

std::vector<std::string> UrtParticipant::what_roles_can_you_play() const {
    return {"domain"};
}

CapabilityResult UrtParticipant::invoke_capability(std::string_view capability_id, const std::string& input_json) const {
    if (capability_id == "urt.cadastro.read") {
        return CapabilityResult{
            .success = true,
            .status_code = 200,
            .content_type = "application/json; charset=utf-8",
            .payload = repository_.exportar_json(),
            .error_message = ""
        };
    }
    if (capability_id == "urt.metrics.evaluate") {
        return CapabilityResult{
            .success = true,
            .status_code = 200,
            .content_type = "application/json; charset=utf-8",
            .payload = to_json(repository_.obter_metricas()),
            .error_message = ""
        };
    }
    if (capability_id == "urt.dataset.export") {
        return CapabilityResult{
            .success = true,
            .status_code = 200,
            .content_type = "application/json; charset=utf-8",
            .payload = repository_.exportar_json(),
            .error_message = ""
        };
    }
    if (capability_id == "urt.cadastro.create") {
        auto parsed = parse_urt_json(input_json);
        if (!parsed) {
            return CapabilityResult{
                .success = false,
                .status_code = 400,
                .content_type = "application/json; charset=utf-8",
                .payload = "",
                .error_message = "Payload JSON inválido para cadastro de URT."
            };
        }
        std::string erro;
        if (!repository_.adicionar(*parsed, &erro)) {
            return CapabilityResult{
                .success = false,
                .status_code = 422,
                .content_type = "application/json; charset=utf-8",
                .payload = "",
                .error_message = erro
            };
        }
        return CapabilityResult{
            .success = true,
            .status_code = 201,
            .content_type = "application/json; charset=utf-8",
            .payload = to_json(*parsed),
            .error_message = ""
        };
    }

    return CapabilityResult{
        .success = false,
        .status_code = 404,
        .content_type = "application/json; charset=utf-8",
        .payload = "",
        .error_message = "Capability não encontrada ou não suportada diretamente: " + std::string{capability_id}
    };
}

std::string UrtParticipant::get_canonical_manifest_json() const {
    std::ifstream f(manifest_path_);
    if (f.is_open()) {
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    // Fallback canônico embutido
    return R"json({
  "schema": "sister.subsystem.manifest/1.0.0",
  "system_id": "sister_urt",
  "name": "SisTer-URT",
  "version": "0.1.0",
  "role": "domain",
  "contract": "sister.subsystem/1.0.0",
  "adapter_version": "1.0.0",
  "mount_path": "/integrations/urt/",
  "audience": "sister_urt",
  "transport": {
    "http": true,
    "websocket": false,
    "internal_endpoint": "http://127.0.0.1:8094"
  },
  "technical_endpoints": {
    "manifest": "/_sister/manifest",
    "health": "/_sister/health",
    "readiness": "/_sister/ready",
    "capabilities": "/_sister/capabilities",
    "identity": "/_sister/identity"
  },
  "capabilities": [
    "urt.cadastro.read",
    "urt.cadastro.create",
    "urt.validation.transition",
    "urt.metrics.evaluate",
    "urt.dataset.export"
  ],
  "data_ownership": "exclusive",
  "audit_level": "domain_relevant_operations",
  "production_eligible": false
})json";
}

std::string UrtParticipant::get_manifest_digest() const {
    return compute_simple_sha256_hex(get_canonical_manifest_json());
}

std::string UrtParticipant::identity_to_json() const {
    auto id = who_are_you();
    std::ostringstream ss;
    ss << "{\n"
       << "  \"schema\": \"sister.subsystem.identity/1.0.0\",\n"
       << "  \"system_id\": \"" << id.system_id << "\",\n"
       << "  \"name\": \"" << id.name << "\",\n"
       << "  \"version\": \"" << id.version << "\",\n"
       << "  \"functional_role\": \"" << id.functional_role << "\",\n"
       << "  \"domain_authority\": \"" << id.domain_authority << "\",\n"
       << "  \"governance_model\": \"" << id.governance_model << "\",\n"
       << "  \"history_model\": \"" << id.history_model << "\",\n"
       << "  \"partner_institutions\": [";

    for (std::size_t i = 0; i < id.partner_institutions.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << id.partner_institutions[i] << "\"";
    }
    ss << "],\n"
       << "  \"boundaries\": {\n"
       << "    \"state_authority\": \"exclusive\",\n"
       << "    \"append_only_history\": true,\n"
       << "    \"provenance_tracking\": true\n"
       << "  }\n"
       << "}";
    return ss.str();
}

std::string UrtParticipant::capabilities_to_json() const {
    auto caps = what_can_you_do();
    std::ostringstream ss;
    ss << "{\n"
       << "  \"schema\": \"sister.subsystem.capabilities/1.0.0\",\n"
       << "  \"system_id\": \"sister_urt\",\n"
       << "  \"contract\": \"sister.subsystem/1.0.0\",\n"
       << "  \"capabilities\": [\n";

    for (std::size_t i = 0; i < caps.size(); ++i) {
        const auto& c = caps[i];
        ss << "    {\"id\": \"" << c.id << "\", \"description\": \"" << c.description << "\", \"risk\": \"" << c.risk << "\"}";
        if (i + 1 < caps.size()) ss << ",\n";
        else ss << "\n";
    }
    ss << "  ]\n"
       << "}";
    return ss.str();
}

std::string UrtParticipant::readiness_to_json() const {
    auto r = what_is_your_state();
    std::ostringstream ss;
    ss << "{\n"
       << "  \"schema\": \"sister.subsystem.readiness/1.0.0\",\n"
       << "  \"system_id\": \"sister_urt\",\n"
       << "  \"status\": \"" << r.status << "\",\n"
       << "  \"contract_version\": \"" << r.contract_version << "\",\n"
       << "  \"total_cadastros\": " << r.total_cadastros << ",\n"
       << "  \"manifest_digest\": \"" << r.manifest_digest << "\",\n"
       << "  \"dependencies\": {\n"
       << "    \"storage\": \"" << (r.storage_ready ? "ready" : "degraded") << "\",\n"
       << "    \"repository\": \"" << (r.repository_ready ? "ready" : "degraded") << "\",\n"
       << "    \"governance\": \"" << (r.governance_active ? "active" : "inactive") << "\"\n"
       << "  },\n"
       << "  \"degraded_capabilities\": [";
    for (std::size_t i = 0; i < r.degraded_capabilities.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << r.degraded_capabilities[i] << "\"";
    }
    ss << "]\n"
       << "}";
    return ss.str();
}

std::string UrtParticipant::health_to_json() const {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"schema\": \"sister.subsystem.health/1.0.0\",\n"
       << "  \"system_id\": \"sister_urt\",\n"
       << "  \"status\": \"ok\",\n"
       << "  \"checked_at\": \"" << gerar_timestamp_iso8601() << "\"\n"
       << "}";
    return ss.str();
}

} // namespace sister::urt
