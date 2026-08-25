// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/types.hpp"

#include <cmath>

namespace sister::urt {

bool Coordenadas::tem_coordenadas_validas() const noexcept {
    return (latitude >= -90.0 && latitude <= 90.0) &&
           (longitude >= -180.0 && longitude <= 180.0) &&
           (std::abs(latitude) > 0.0001 || std::abs(longitude) > 0.0001);
}

bool ResponsavelTecnico::esta_identificado() const noexcept {
    return !nome.empty() && (!instituicao.empty() || !contato.empty());
}

bool CamadaA::esta_valida() const noexcept {
    return !codigo_urt.empty() &&
           !nome_local.empty() &&
           !instituicao_referencia.empty() &&
           !municipio.empty() &&
           !uf.empty();
}

double CamadaB::grau_completude() const noexcept {
    int total_campos = 9;
    int preenchidos = 0;

    if (tipo_sistema != SistemaTipo::Desconhecido) ++preenchidos;
    if (area_urt_ha > 0.0) ++preenchidos;
    if (!data_implantacao.empty()) ++preenchidos;
    if (situacao_atual != SituacaoAtual::Desconhecida) ++preenchidos;
    if (!especies_arboreas.empty()) ++preenchidos;
    if (!pastagem_forrageira.empty()) ++preenchidos;
    if (!componente_animal.empty()) ++preenchidos;
    if (arranjo_espacial != ArranjoEspacial::NaoInformado) ++preenchidos;
    if (!espacamento.empty()) ++preenchidos;

    return static_cast<double>(preenchidos) / static_cast<double>(total_campos);
}

bool CamadaB::esta_completa() const noexcept {
    return grau_completude() >= 0.77; // Pelo menos 7 dos 9 campos principais preenchidos
}

std::string_view to_string(SistemaTipo tipo) noexcept {
    switch (tipo) {
        case SistemaTipo::SP: return "SP";
        case SistemaTipo::ASP: return "ASP";
        case SistemaTipo::SAF: return "SAF";
        case SistemaTipo::Desconhecido: return "Desconhecido";
    }
    return "Desconhecido";
}

SistemaTipo parse_sistema_tipo(std::string_view str) noexcept {
    if (str == "SP" || str == "sp" || str == "Silvipastoril") return SistemaTipo::SP;
    if (str == "ASP" || str == "asp" || str == "Agrossilvipastoril") return SistemaTipo::ASP;
    if (str == "SAF" || str == "saf" || str == "Agroflorestal") return SistemaTipo::SAF;
    return SistemaTipo::Desconhecido;
}

std::string_view to_string(SituacaoAtual situacao) noexcept {
    switch (situacao) {
        case SituacaoAtual::Ativa: return "Ativa";
        case SituacaoAtual::Inativa: return "Inativa";
        case SituacaoAtual::Descaracterizada: return "Descaracterizada";
        case SituacaoAtual::Desconhecida: return "Desconhecida";
    }
    return "Desconhecida";
}

SituacaoAtual parse_situacao_atual(std::string_view str) noexcept {
    if (str == "Ativa" || str == "ativa" || str == "active") return SituacaoAtual::Ativa;
    if (str == "Inativa" || str == "inativa" || str == "inactive") return SituacaoAtual::Inativa;
    if (str == "Descaracterizada" || str == "descaracterizada") return SituacaoAtual::Descaracterizada;
    return SituacaoAtual::Desconhecida;
}

std::string_view to_string(StatusValidacao status) noexcept {
    switch (status) {
        case StatusValidacao::Preliminar: return "preliminar";
        case StatusValidacao::RequerRevisao: return "requer_revisao";
        case StatusValidacao::Validado: return "validado";
        case StatusValidacao::Arquivado: return "arquivado";
    }
    return "preliminar";
}

StatusValidacao parse_status_validacao(std::string_view str) noexcept {
    if (str == "validado" || str == "Validado" || str == "validated") return StatusValidacao::Validado;
    if (str == "requer_revisao" || str == "RequerRevisao" || str == "revisao") return StatusValidacao::RequerRevisao;
    if (str == "arquivado" || str == "Arquivado" || str == "archived") return StatusValidacao::Arquivado;
    return StatusValidacao::Preliminar;
}

std::string_view to_string(ArranjoEspacial arranjo) noexcept {
    switch (arranjo) {
        case ArranjoEspacial::LinhasSimples: return "Linhas Simples";
        case ArranjoEspacial::LinhasDuplas: return "Linhas Duplas";
        case ArranjoEspacial::Faixas: return "Faixas";
        case ArranjoEspacial::Renques: return "Renques";
        case ArranjoEspacial::Bosquetes: return "Bosquetes";
        case ArranjoEspacial::Outro: return "Outro";
        case ArranjoEspacial::NaoInformado: return "Não informado";
    }
    return "Não informado";
}

ArranjoEspacial parse_arranjo_espacial(std::string_view str) noexcept {
    if (str == "Linhas Simples" || str == "linhas_simples" || str == "simples") return ArranjoEspacial::LinhasSimples;
    if (str == "Linhas Duplas" || str == "linhas_duplas" || str == "duplas") return ArranjoEspacial::LinhasDuplas;
    if (str == "Faixas" || str == "faixas") return ArranjoEspacial::Faixas;
    if (str == "Renques" || str == "renques") return ArranjoEspacial::Renques;
    if (str == "Bosquetes" || str == "bosquetes") return ArranjoEspacial::Bosquetes;
    if (str == "Outro" || str == "outro") return ArranjoEspacial::Outro;
    return ArranjoEspacial::NaoInformado;
}

} // namespace sister::urt
