// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/domain.hpp"

namespace sister::urt {

InvariantValidationResult validar_invariantes(const UrtRecord& urt) noexcept {
    InvariantValidationResult result;

    if (urt.id.empty()) {
        result.erros.push_back("Identificador único (id) é obrigatório");
    }

    // Validação da Camada A
    if (urt.camada_a.codigo_urt.empty()) {
        result.erros.push_back("Código da URT é obrigatório na Camada A");
    }
    if (urt.camada_a.nome_local.empty()) {
        result.erros.push_back("Nome local da URT é obrigatório na Camada A");
    }
    if (urt.camada_a.instituicao_referencia.empty()) {
        result.erros.push_back("Instituição de referência é obrigatória na Camada A");
    }
    if (urt.camada_a.municipio.empty() || urt.camada_a.uf.empty()) {
        result.erros.push_back("Município e UF são obrigatórios na Camada A");
    }

    // Pendências (qualificação progressiva sem invalidar o cadastro preliminar)
    if (!urt.camada_a.coordenadas.tem_coordenadas_validas()) {
        result.pendencias.push_back("Coordenadas geográficas pendentes ou fora do intervalo válido");
    }
    if (!urt.camada_a.responsavel_tecnico.esta_identificado()) {
        result.pendencias.push_back("Responsável técnico ainda não identificado");
    }
    if (!urt.camada_b.esta_completa()) {
        result.pendencias.push_back("Camada B incompleta (grau de completude: " +
                                    std::to_string(static_cast<int>(urt.camada_b.grau_completude() * 100)) + "%)");
    }
    if (urt.camada_c.fonte_informacao.empty()) {
        result.pendencias.push_back("Fonte da informação na Camada C não informada");
    }

    result.valido = result.erros.empty();
    return result;
}

UrtMetrics calcular_metricas(const std::vector<UrtRecord>& lista) noexcept {
    UrtMetrics metrics;
    metrics.total_cadastros = lista.size();

    if (lista.empty()) {
        return metrics;
    }

    for (const auto& urt : lista) {
        if (urt.camada_a.coordenadas.tem_coordenadas_validas()) {
            metrics.com_coordenadas_validas++;
        }
        if (urt.camada_a.responsavel_tecnico.esta_identificado()) {
            metrics.com_responsavel_tecnico++;
        }
        if (urt.camada_b.esta_completa()) {
            metrics.com_camada_b_completa++;
        }

        // Agrupamentos
        const std::string inst = urt.camada_a.instituicao_referencia.empty() ? "Não informada" : urt.camada_a.instituicao_referencia;
        metrics.por_instituicao[inst]++;

        metrics.por_tipo_sistema[std::string{to_string(urt.camada_b.tipo_sistema)}]++;
        metrics.por_situacao[std::string{to_string(urt.camada_b.situacao_atual)}]++;
        metrics.por_status_validacao[std::string{to_string(urt.status_validacao)}]++;

        const std::string uf = urt.camada_a.uf.empty() ? "N/I" : urt.camada_a.uf;
        metrics.por_uf[uf]++;
    }

    const auto total = static_cast<double>(metrics.total_cadastros);
    metrics.taxa_coordenadas_validas = static_cast<double>(metrics.com_coordenadas_validas) / total;
    metrics.taxa_responsavel_tecnico = static_cast<double>(metrics.com_responsavel_tecnico) / total;
    metrics.taxa_camada_b_completa = static_cast<double>(metrics.com_camada_b_completa) / total;

    return metrics;
}

} // namespace sister::urt
