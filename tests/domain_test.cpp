// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/domain.hpp"
#include "sister/urt/types.hpp"

#include <cassert>
#include <iostream>
#include <vector>

void test_camada_a_validacao() {
    sister::urt::CamadaA a;
    assert(!a.esta_valida());

    a.codigo_urt = "URT-TEST-01";
    a.nome_local = "Unidade Teste";
    a.instituicao_referencia = "EPAGRI";
    a.municipio = "Lages";
    a.uf = "SC";
    assert(a.esta_valida());
}

void test_coordenadas_validacao() {
    sister::urt::Coordenadas c;
    assert(!c.tem_coordenadas_validas());

    c.latitude = -27.8159;
    c.longitude = -50.3261;
    c.valida = true;
    assert(c.tem_coordenadas_validas());

    c.latitude = 120.0; // Inválida
    assert(!c.tem_coordenadas_validas());
}

void test_camada_b_completude() {
    sister::urt::CamadaB b;
    assert(b.grau_completude() == 0.0);
    assert(!b.esta_completa());

    b.tipo_sistema = sister::urt::SistemaTipo::SP;
    b.area_urt_ha = 15.0;
    b.data_implantacao = "2020-01-01";
    b.situacao_atual = sister::urt::SituacaoAtual::Ativa;
    b.especies_arboreas = "Pinus taeda";
    b.pastagem_forrageira = "Azevém";
    b.componente_animal = "Bovinos";
    b.arranjo_espacial = sister::urt::ArranjoEspacial::LinhasDuplas;
    b.espacamento = "12m x 3m";

    assert(b.grau_completude() == 1.0);
    assert(b.esta_completa());
}

void test_metricas_globais() {
    std::vector<sister::urt::UrtRecord> lista;

    sister::urt::UrtRecord u1;
    u1.id = "u1";
    u1.camada_a.codigo_urt = "URT-01";
    u1.camada_a.nome_local = "URT 1";
    u1.camada_a.instituicao_referencia = "EPAGRI";
    u1.camada_a.municipio = "Lages";
    u1.camada_a.uf = "SC";
    u1.camada_a.coordenadas = {-27.0, -50.0, true};
    u1.camada_a.responsavel_tecnico = {"Fulano", "EPAGRI", "contato"};
    u1.camada_b.tipo_sistema = sister::urt::SistemaTipo::SP;
    u1.camada_b.area_urt_ha = 10.0;
    u1.camada_b.data_implantacao = "2020";
    u1.camada_b.situacao_atual = sister::urt::SituacaoAtual::Ativa;
    u1.camada_b.especies_arboreas = "Eucalipto";
    u1.camada_b.pastagem_forrageira = "Tifton";
    u1.camada_b.componente_animal = "Gado";
    u1.camada_b.arranjo_espacial = sister::urt::ArranjoEspacial::LinhasSimples;
    u1.camada_b.espacamento = "10x2";
    u1.status_validacao = sister::urt::StatusValidacao::Validado;

    sister::urt::UrtRecord u2;
    u2.id = "u2";
    u2.camada_a.codigo_urt = "URT-02";
    u2.camada_a.nome_local = "URT 2";
    u2.camada_a.instituicao_referencia = "EMATER-RS";
    u2.camada_a.municipio = "Bagé";
    u2.camada_a.uf = "RS";
    u2.status_validacao = sister::urt::StatusValidacao::Preliminar;

    lista.push_back(u1);
    lista.push_back(u2);

    auto m = sister::urt::calcular_metricas(lista);
    assert(m.total_cadastros == 2);
    assert(m.com_coordenadas_validas == 1);
    assert(m.com_responsavel_tecnico == 1);
    assert(m.com_camada_b_completa == 1);
    assert(m.taxa_coordenadas_validas == 0.5);
    assert(m.por_instituicao["EPAGRI"] == 1);
    assert(m.por_instituicao["EMATER-RS"] == 1);
    assert(m.por_status_validacao["validado"] == 1);
    assert(m.por_status_validacao["preliminar"] == 1);
}

int main() {
    test_camada_a_validacao();
    test_coordenadas_validacao();
    test_camada_b_completude();
    test_metricas_globais();
    std::cout << "[PASS] Todos os testes de domínio passaram com sucesso.\n";
    return 0;
}
