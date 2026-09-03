// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/domain.hpp"
#include "sister/urt/governance.hpp"
#include "sister/urt/repository.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void print_help() {
    std::cout << "SisTer-URT CLI · Gerenciamento de Unidades de Referência Tecnológica\n\n"
              << "Comandos:\n"
              << "  list [--inst X] [--tipo X] [--stat X]  Lista as URTs cadastradas\n"
              << "  show <id>                              Exibe ficha completa da URT por Camadas\n"
              << "  metrics                                Exibe indicadores globais de cobertura\n"
              << "  validate <id> <status> <autor> <mot>   Executa transição governada de validação\n"
              << "  export <caminho>                       Exporta base para arquivo JSON\n\n"
              << "Exemplo:\n"
              << "  ./build/sister-urt-cli list\n"
              << "  ./build/sister-urt-cli show urt-rs-001\n"
              << "  ./build/sister-urt-cli metrics\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    sister::urt::UrtRepository repo;

    const std::string_view cmd{argv[1]};

    if (cmd == "--help" || cmd == "-h" || cmd == "help") {
        print_help();
        return 0;
    }

    if (cmd == "list") {
        sister::urt::FiltroUrt filtro;
        for (int i = 2; i < argc; ++i) {
            std::string_view arg{argv[i]};
            if (arg == "--inst" && i + 1 < argc) filtro.instituicao = argv[++i];
            else if (arg == "--tipo" && i + 1 < argc) filtro.tipo_sistema = argv[++i];
            else if (arg == "--stat" && i + 1 < argc) filtro.status_validacao = argv[++i];
        }

        const auto lista = repo.listar(filtro);
        std::cout << "\n=== URTs Cadastradas (" << lista.size() << " encontradas) ===\n\n";
        std::cout << std::left << std::setw(14) << "CÓDIGO"
                  << std::setw(34) << "NOME LOCAL"
                  << std::setw(16) << "INSTITUIÇÃO"
                  << std::setw(20) << "MUNICÍPIO/UF"
                  << std::setw(8) << "TIPO"
                  << std::setw(14) << "VALIDAÇÃO"
                  << std::setw(10) << "CAMADA B\n";
        std::cout << std::string(116, '-') << '\n';

        for (const auto& u : lista) {
            std::string muni_uf = u.camada_a.municipio + "/" + u.camada_a.uf;
            int pct_b = static_cast<int>(u.camada_b.grau_completude() * 100);
            std::cout << std::left << std::setw(14) << u.camada_a.codigo_urt
                      << std::setw(34) << (u.camada_a.nome_local.size() > 32 ? u.camada_a.nome_local.substr(0, 29) + "..." : u.camada_a.nome_local)
                      << std::setw(16) << u.camada_a.instituicao_referencia
                      << std::setw(20) << muni_uf
                      << std::setw(8) << sister::urt::to_string(u.camada_b.tipo_sistema)
                      << std::setw(14) << sister::urt::to_string(u.status_validacao)
                      << std::setw(10) << (std::to_string(pct_b) + "%") << '\n';
        }
        std::cout << '\n';
        return 0;
    }

    if (cmd == "show" && argc >= 3) {
        const std::string id{argv[2]};
        const auto urt_opt = repo.buscar_por_id(id);
        if (!urt_opt) {
            std::cerr << "Erro: URT com ID '" << id << "' não encontrada.\n";
            return 1;
        }

        const auto& u = *urt_opt;
        std::cout << "\n======================================================================\n";
        std::cout << " FICHA DA URT: " << u.camada_a.codigo_urt << " · " << u.camada_a.nome_local << '\n';
        std::cout << " ID: " << u.id << " | Versão: v" << u.versao
                  << " | Status: [" << sister::urt::to_string(u.status_validacao) << "]\n";
        std::cout << "======================================================================\n\n";

        std::cout << "--- [ CAMADA A: REGISTRO ESSENCIAL ] ---\n";
        std::cout << "  • Instituição: " << u.camada_a.instituicao_referencia << '\n';
        std::cout << "  • Município/UF: " << u.camada_a.municipio << " - " << u.camada_a.uf << '\n';
        std::cout << "  • Localidade: " << u.camada_a.localidade_referencia << '\n';
        if (u.camada_a.coordenadas.tem_coordenadas_validas()) {
            std::cout << "  • Coordenadas: " << u.camada_a.coordenadas.latitude << ", " << u.camada_a.coordenadas.longitude << " (Válidas)\n";
        } else {
            std::cout << "  • Coordenadas: [Pendente ou Não Informado]\n";
        }
        std::cout << "  • Responsável Técnico: " << u.camada_a.responsavel_tecnico.nome
                  << " (" << u.camada_a.responsavel_tecnico.instituicao << ") - "
                  << u.camada_a.responsavel_tecnico.contato << '\n';
        std::cout << "  • Propriedade: " << u.camada_a.propriedade.nome
                  << " | Responsável: " << u.camada_a.propriedade.proprietario_responsavel
                  << " | Área Total: " << u.camada_a.propriedade.area_total_ha << " ha\n\n";

        std::cout << "--- [ CAMADA B: CARACTERIZAÇÃO TÉCNICA ] ---\n";
        std::cout << "  • Tipo de Sistema: " << sister::urt::to_string(u.camada_b.tipo_sistema)
                  << " | Área da URT: " << u.camada_b.area_urt_ha << " ha\n";
        std::cout << "  • Situação Atual: " << sister::urt::to_string(u.camada_b.situacao_atual)
                  << " | Implantação: " << u.camada_b.data_implantacao << '\n';
        std::cout << "  • Espécies Arbóreas: " << u.camada_b.especies_arboreas << '\n';
        std::cout << "  • Componente Forrageiro: " << u.camada_b.pastagem_forrageira << '\n';
        std::cout << "  • Componente Animal: " << u.camada_b.componente_animal << '\n';
        std::cout << "  • Arranjo Espacial: " << sister::urt::to_string(u.camada_b.arranjo_espacial)
                  << " | Espaçamento: " << u.camada_b.espacamento << '\n';
        std::cout << "  • Manejo Atual: " << u.camada_b.manejo_atual << '\n';
        std::cout << "  • Grau de Completude Camada B: " << static_cast<int>(u.camada_b.grau_completude() * 100) << "%\n\n";

        std::cout << "--- [ CAMADA C: EVIDÊNCIAS E HISTÓRICO ] ---\n";
        std::cout << "  • Fonte de Informação: " << u.camada_c.fonte_informacao << '\n';
        std::cout << "  • Última Atualização: " << u.camada_c.data_ultima_atualizacao << '\n';
        std::cout << "  • Observações: " << u.camada_c.observacoes << '\n';
        std::cout << "  • Registros Documentais: " << u.camada_c.registros_documentais.size() << " itens anexados\n\n";

        std::cout << "--- [ TRILHA DE GOVERNANÇA E AUDITORIA ] ---\n";
        if (u.historico_transicoes.empty()) {
            std::cout << "  (Nenhuma transição registrada - versão inicial)\n";
        } else {
            for (const auto& rec : u.historico_transicoes) {
                std::cout << "  [" << rec.timestamp << "] "
                          << sister::urt::to_string(rec.de_status) << " -> " << sister::urt::to_string(rec.para_status)
                          << " | Autoridade: " << rec.autoridade << '\n'
                          << "    Linhagem: " << rec.predecessor_state_id << " -> " << rec.successor_state_id << '\n'
                          << "    Motivo: " << rec.motivo << "\n\n";
            }
        }
        return 0;
    }

    if (cmd == "metrics") {
        const auto m = repo.obter_metricas();
        std::cout << "\n=== INDICADORES GLOBAIS DO CADASTRO DE URTs ===\n\n";
        std::cout << "  • Total de URTs cadastradas: " << m.total_cadastros << '\n';
        std::cout << "  • Com coordenadas válidas: " << m.com_coordenadas_validas
                  << " (" << std::fixed << std::setprecision(1) << (m.taxa_coordenadas_validas * 100) << "%)\n";
        std::cout << "  • Com responsável técnico: " << m.com_responsavel_tecnico
                  << " (" << (m.taxa_responsavel_tecnico * 100) << "%)\n";
        std::cout << "  • Com Camada B completa: " << m.com_camada_b_completa
                  << " (" << (m.taxa_camada_b_completa * 100) << "%)\n\n";

        std::cout << "--- Distribuição por Instituição ---\n";
        for (const auto& [inst, count] : m.por_instituicao) {
            std::cout << "  - " << std::left << std::setw(18) << inst << ": " << count << '\n';
        }
        std::cout << "\n--- Distribuição por Tipo de Sistema ---\n";
        for (const auto& [tipo, count] : m.por_tipo_sistema) {
            std::cout << "  - " << std::left << std::setw(18) << tipo << ": " << count << '\n';
        }
        std::cout << "\n--- Distribuição por Status de Validação ---\n";
        for (const auto& [stat, count] : m.por_status_validacao) {
            std::cout << "  - " << std::left << std::setw(18) << stat << ": " << count << '\n';
        }
        std::cout << '\n';
        return 0;
    }

    if (cmd == "validate" && argc >= 6) {
        const std::string id{argv[2]};
        const auto novo_status = sister::urt::parse_status_validacao(argv[3]);
        const std::string autoridade{argv[4]};
        const std::string motivo{argv[5]};

        const auto res = repo.transicionar(id, novo_status, autoridade, motivo);
        if (!res.sucesso) {
            std::cerr << "Falha na transição: " << res.erro << '\n';
            return 1;
        }

        std::cout << "Transição efetuada com sucesso!\n";
        std::cout << "Recibo ID: " << res.recibo.id << '\n';
        std::cout << "Linhagem: " << res.recibo.predecessor_state_id << " -> " << res.recibo.successor_state_id << '\n';
        return 0;
    }

    print_help();
    return 0;
}
