// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sister/urt/domain.hpp"
#include "sister/urt/governance.hpp"
#include "sister/urt/types.hpp"

#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sister::urt {

struct FiltroUrt {
    std::string instituicao;
    std::string municipio;
    std::string uf;
    std::string tipo_sistema;
    std::string situacao;
    std::string status_validacao;
    std::string busca_texto;
};

class UrtRepository {
public:
    UrtRepository() = default;

    bool carregar_arquivo_json(const std::string& caminho);
    bool carregar_string_json(std::string_view conteudo);
    [[nodiscard]] std::string exportar_json() const;

    [[nodiscard]] std::vector<UrtRecord> listar(const FiltroUrt& filtro = {}) const;
    [[nodiscard]] std::optional<UrtRecord> buscar_por_id(std::string_view id) const;

    bool adicionar(UrtRecord urt, std::string* erro = nullptr);
    bool atualizar(UrtRecord urt, std::string* erro = nullptr);

    TransitionResult transicionar(
        std::string_view id,
        StatusValidacao novo_status,
        std::string_view autoridade,
        std::string_view motivo);

    [[nodiscard]] UrtMetrics obter_metricas() const;
    [[nodiscard]] std::size_t total() const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, UrtRecord> dados_;
    std::vector<std::string> ordem_insercao_;
};

// Funções utilitárias de serialização JSON
[[nodiscard]] std::string to_json(const UrtRecord& urt);
[[nodiscard]] std::string to_json(const std::vector<UrtRecord>& lista);
[[nodiscard]] std::string to_json(const UrtMetrics& metrics);
[[nodiscard]] std::string to_json(const TransitoryReceipt& receipt);

[[nodiscard]] std::optional<UrtRecord> parse_urt_json(std::string_view json_str);
[[nodiscard]] std::vector<UrtRecord> parse_urts_lista_json(std::string_view json_str);

} // namespace sister::urt
