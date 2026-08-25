// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sister/urt/types.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace sister::urt {

struct UrtMetrics {
    std::size_t total_cadastros{0};
    std::size_t com_coordenadas_validas{0};
    std::size_t com_responsavel_tecnico{0};
    std::size_t com_camada_b_completa{0};

    double taxa_coordenadas_validas{0.0};
    double taxa_responsavel_tecnico{0.0};
    double taxa_camada_b_completa{0.0};

    std::unordered_map<std::string, std::size_t> por_instituicao;
    std::unordered_map<std::string, std::size_t> por_tipo_sistema;
    std::unordered_map<std::string, std::size_t> por_situacao;
    std::unordered_map<std::string, std::size_t> por_status_validacao;
    std::unordered_map<std::string, std::size_t> por_uf;
};

struct InvariantValidationResult {
    bool valido{true};
    std::vector<std::string> erros;
    std::vector<std::string> pendencias; // avisos para qualificação posterior
};

[[nodiscard]] InvariantValidationResult validar_invariantes(const UrtRecord& urt) noexcept;

[[nodiscard]] UrtMetrics calcular_metricas(const std::vector<UrtRecord>& lista) noexcept;

} // namespace sister::urt
