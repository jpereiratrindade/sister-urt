// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace sister::urt {

enum class SistemaTipo {
    SP,   // Sistema Silvipastoril
    ASP,  // Sistema Agrossilvipastoril
    SAF,  // Sistema Agroflorestal
    Desconhecido
};

enum class SituacaoAtual {
    Ativa,
    Inativa,
    Descaracterizada,
    Desconhecida
};

enum class StatusValidacao {
    Preliminar,
    RequerRevisao,
    Validado
};

enum class ArranjoEspacial {
    LinhasSimples,
    LinhasDuplas,
    Faixas,
    Renques,
    Bosquetes,
    Outro,
    NaoInformado
};

struct Coordenadas {
    double latitude{0.0};
    double longitude{0.0};
    bool valida{false};

    [[nodiscard]] bool tem_coordenadas_validas() const noexcept;
};

struct ResponsavelTecnico {
    std::string nome;
    std::string instituicao;
    std::string contato;

    [[nodiscard]] bool esta_identificado() const noexcept;
};

struct Propriedade {
    std::string nome;
    std::string proprietario_responsavel;
    double area_total_ha{0.0};
    std::string atividade_principal;
};

// Camada A — Registro Essencial
struct CamadaA {
    std::string codigo_urt;
    std::string nome_local;
    std::string instituicao_referencia;
    std::string municipio;
    std::string uf;
    Coordenadas coordenadas;
    std::string localidade_referencia;
    ResponsavelTecnico responsavel_tecnico;
    Propriedade propriedade;

    [[nodiscard]] bool esta_valida() const noexcept;
};

// Camada B — Caracterização Técnica
struct CamadaB {
    SistemaTipo tipo_sistema{SistemaTipo::Desconhecido};
    double area_urt_ha{0.0};
    std::string data_implantacao;
    SituacaoAtual situacao_atual{SituacaoAtual::Desconhecida};
    std::string especies_arboreas;
    std::string pastagem_forrageira;
    std::string componente_animal;
    ArranjoEspacial arranjo_espacial{ArranjoEspacial::NaoInformado};
    std::string espacamento;
    std::string manejo_atual;

    [[nodiscard]] double grau_completude() const noexcept;
    [[nodiscard]] bool esta_completa() const noexcept;
};

// Camada C — Evidências e Histórico
struct CamadaC {
    std::string fonte_informacao;
    std::vector<std::string> registros_documentais;
    std::string data_ultima_atualizacao;
    std::string observacoes;
};

// Recibo Epistêmico de Transição
struct TransitoryReceipt {
    std::string id;
    std::string timestamp;
    StatusValidacao de_status{StatusValidacao::Preliminar};
    StatusValidacao para_status{StatusValidacao::Preliminar};
    std::string autoridade;
    std::string motivo;
    std::string predecessor_state_id;
    std::string successor_state_id;
};

// Registro Completo da URT
struct UrtRecord {
    std::string id;
    int versao{1};
    StatusValidacao status_validacao{StatusValidacao::Preliminar};
    CamadaA camada_a;
    CamadaB camada_b;
    CamadaC camada_c;
    std::vector<TransitoryReceipt> historico_transicoes;
};

[[nodiscard]] std::string_view to_string(SistemaTipo tipo) noexcept;
[[nodiscard]] SistemaTipo parse_sistema_tipo(std::string_view str) noexcept;

[[nodiscard]] std::string_view to_string(SituacaoAtual situacao) noexcept;
[[nodiscard]] SituacaoAtual parse_situacao_atual(std::string_view str) noexcept;

[[nodiscard]] std::string_view to_string(StatusValidacao status) noexcept;
[[nodiscard]] StatusValidacao parse_status_validacao(std::string_view str) noexcept;

[[nodiscard]] std::string_view to_string(ArranjoEspacial arranjo) noexcept;
[[nodiscard]] ArranjoEspacial parse_arranjo_espacial(std::string_view str) noexcept;

} // namespace sister::urt
