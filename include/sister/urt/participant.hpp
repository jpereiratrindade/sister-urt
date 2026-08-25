// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sister/urt/repository.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sister::urt {

struct ParticipantIdentity {
    std::string system_id{"sister_urt"};
    std::string name{"SisTer-URT"};
    std::string version{"0.1.0"};
    std::string functional_role{"domain"};
    std::string domain_authority{"Unidades de Referência Tecnológica (Sistemas Silvipastoris, ASP e SAF)"};
    std::string governance_model{"local_governance_v1 (SisTer-Praxis assessment compatible)"};
    std::string history_model{"append_only_transition_history"};
    std::vector<std::string> partner_institutions{"CPPSul", "CNPF", "EMATER-RS", "EPAGRI", "EMATER-PR"};
};

struct CapabilityDescriptor {
    std::string id;
    std::string description;
    std::string risk; // low, medium, high
    std::string input_schema;
    std::string output_schema;
};

struct AuthorityBoundary {
    std::string domain_name{"unidades_referencia_tecnologica"};
    std::vector<std::string> scope;
    bool exclusive_authority{true};
};

struct ContractSet {
    std::string subsystem_contract{"sister.subsystem/1.0.0"};
    std::string participant_contract{"sister.participant/1.0.0"};
    std::vector<std::string> supported_bindings{"http", "in_process_cpp"};
};

struct ParticipantReadiness {
    std::string status{"ready"}; // ready, degraded, not_ready
    std::string contract_version{"1.0.0"};
    std::size_t total_cadastros{0};
    std::string manifest_digest;
    std::vector<std::string> degraded_capabilities;
    bool storage_ready{true};
    bool repository_ready{true};
    bool governance_active{true};
};

struct CapabilityResult {
    bool success{false};
    int status_code{200};
    std::string content_type{"application/json; charset=utf-8"};
    std::string payload;
    std::string error_message;
};

class UrtParticipant {
public:
    explicit UrtParticipant(UrtRepository& repository,
                            std::filesystem::path manifest_path = "contracts/system_manifest.json") noexcept;

    // Fronteira Semântica do Participante (Seção 7.3 do Documento Fundacional)
    [[nodiscard]] ParticipantIdentity who_are_you() const noexcept;
    [[nodiscard]] std::vector<CapabilityDescriptor> what_can_you_do() const;
    [[nodiscard]] AuthorityBoundary what_do_you_own() const;
    [[nodiscard]] ContractSet how_to_talk_to_you() const;
    [[nodiscard]] ParticipantReadiness what_is_your_state() const;
    [[nodiscard]] std::vector<TransitoryReceipt> what_happened(std::string_view urt_id) const;
    [[nodiscard]] std::vector<std::string> what_are_your_limits() const;
    [[nodiscard]] std::vector<std::string> what_roles_can_you_play() const;

    // Despacho transport-neutral de capabilities
    [[nodiscard]] CapabilityResult invoke_capability(std::string_view capability_id, const std::string& input_json = "") const;

    // Manifesto Canônico e Digest SHA-256
    [[nodiscard]] std::string get_canonical_manifest_json() const;
    [[nodiscard]] std::string get_manifest_digest() const;

    // JSON Serializers para a fronteira semântica
    [[nodiscard]] std::string identity_to_json() const;
    [[nodiscard]] std::string capabilities_to_json() const;
    [[nodiscard]] std::string readiness_to_json() const;
    [[nodiscard]] std::string health_to_json() const;

private:
    UrtRepository& repository_;
    std::filesystem::path manifest_path_;
};

} // namespace sister::urt
