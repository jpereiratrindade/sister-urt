// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/repository.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace sister::urt {

namespace {

std::string escape_json_str(std::string_view str) {
    std::string out;
    out.reserve(str.size() + 8);
    for (char c : str) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

std::string to_lower_case(std::string_view str) {
    std::string res;
    res.reserve(str.size());
    for (char c : str) {
        res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return res;
}

// Helpers para parser JSON minimalista
std::string extrair_campo_string(std::string_view json, std::string_view chave) {
    const std::string pattern = "\"" + std::string{chave} + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string_view::npos) return "";

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string_view::npos) return "";

    // Pula espaços
    pos = json.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == std::string_view::npos || json[pos] != '"') return "";

    auto end = json.find('"', pos + 1);
    while (end != std::string_view::npos && json[end - 1] == '\\') {
        end = json.find('"', end + 1);
    }
    if (end == std::string_view::npos) return "";

    std::string raw{json.substr(pos + 1, end - pos - 1)};
    // Desescapar caracteres simples
    std::string unescaped;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            char next = raw[++i];
            if (next == 'n') unescaped += '\n';
            else if (next == 'r') unescaped += '\r';
            else if (next == 't') unescaped += '\t';
            else if (next == '"') unescaped += '"';
            else if (next == '\\') unescaped += '\\';
            else unescaped += next;
        } else {
            unescaped += raw[i];
        }
    }
    return unescaped;
}

double extrair_campo_double(std::string_view json, std::string_view chave, double padrao = 0.0) {
    const std::string pattern = "\"" + std::string{chave} + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string_view::npos) return padrao;

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string_view::npos) return padrao;

    pos = json.find_first_not_of(" \t\r\n\"", pos + 1);
    if (pos == std::string_view::npos) return padrao;

    auto end = json.find_first_of(",}\" \t\r\n", pos);
    std::string val_str{json.substr(pos, end - pos)};
    try {
        return std::stod(val_str);
    } catch (...) {
        return padrao;
    }
}

int extrair_campo_int(std::string_view json, std::string_view chave, int padrao = 0) {
    return static_cast<int>(extrair_campo_double(json, chave, static_cast<double>(padrao)));
}

bool extrair_campo_bool(std::string_view json, std::string_view chave, bool padrao = false) {
    const std::string pattern = "\"" + std::string{chave} + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string_view::npos) return padrao;

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string_view::npos) return padrao;

    pos = json.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == std::string_view::npos) return padrao;

    if (json.substr(pos, 4) == "true") return true;
    if (json.substr(pos, 5) == "false") return false;
    return padrao;
}

std::vector<std::string> extrair_campo_array_strings(std::string_view json, std::string_view chave) {
    std::vector<std::string> resultado;
    const std::string pattern = "\"" + std::string{chave} + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string_view::npos) return resultado;

    pos = json.find('[', pos + pattern.size());
    if (pos == std::string_view::npos) return resultado;

    auto end_array = json.find(']', pos);
    if (end_array == std::string_view::npos) return resultado;

    std::string_view array_content = json.substr(pos + 1, end_array - pos - 1);
    std::size_t cur = 0;
    while (cur < array_content.size()) {
        auto str_start = array_content.find('"', cur);
        if (str_start == std::string_view::npos) break;
        auto str_end = array_content.find('"', str_start + 1);
        while (str_end != std::string_view::npos && array_content[str_end - 1] == '\\') {
            str_end = array_content.find('"', str_end + 1);
        }
        if (str_end == std::string_view::npos) break;
        resultado.push_back(std::string{array_content.substr(str_start + 1, str_end - str_start - 1)});
        cur = str_end + 1;
    }
    return resultado;
}

std::string_view extrair_objeto(std::string_view json, std::string_view chave) {
    const std::string pattern = "\"" + std::string{chave} + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string_view::npos) return "";

    pos = json.find('{', pos + pattern.size());
    if (pos == std::string_view::npos) return "";

    int profundidade = 1;
    std::size_t end = pos + 1;
    while (end < json.size() && profundidade > 0) {
        if (json[end] == '{') ++profundidade;
        else if (json[end] == '}') --profundidade;
        else if (json[end] == '"') {
            ++end;
            while (end < json.size() && json[end] != '"') {
                if (json[end] == '\\') ++end;
                ++end;
            }
        }
        ++end;
    }
    if (profundidade == 0) {
        return json.substr(pos, end - pos);
    }
    return "";
}

} // namespace

std::string to_json(const TransitoryReceipt& receipt) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"id\": \"" << escape_json_str(receipt.id) << "\",\n"
       << "  \"timestamp\": \"" << escape_json_str(receipt.timestamp) << "\",\n"
       << "  \"de_status\": \"" << escape_json_str(to_string(receipt.de_status)) << "\",\n"
       << "  \"para_status\": \"" << escape_json_str(to_string(receipt.para_status)) << "\",\n"
       << "  \"autoridade\": \"" << escape_json_str(receipt.autoridade) << "\",\n"
       << "  \"motivo\": \"" << escape_json_str(receipt.motivo) << "\",\n"
       << "  \"predecessor_state_id\": \"" << escape_json_str(receipt.predecessor_state_id) << "\",\n"
       << "  \"successor_state_id\": \"" << escape_json_str(receipt.successor_state_id) << "\"\n"
       << "}";
    return ss.str();
}

std::string to_json(const UrtRecord& urt) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "{\n"
       << "  \"id\": \"" << escape_json_str(urt.id) << "\",\n"
       << "  \"versao\": " << urt.versao << ",\n"
       << "  \"status_validacao\": \"" << escape_json_str(to_string(urt.status_validacao)) << "\",\n"
       << "  \"grau_completude_camada_b\": " << std::setprecision(2) << urt.camada_b.grau_completude() << ",\n"
       << "  \"camada_a\": {\n"
       << "    \"codigo_urt\": \"" << escape_json_str(urt.camada_a.codigo_urt) << "\",\n"
       << "    \"nome_local\": \"" << escape_json_str(urt.camada_a.nome_local) << "\",\n"
       << "    \"instituicao_referencia\": \"" << escape_json_str(urt.camada_a.instituicao_referencia) << "\",\n"
       << "    \"municipio\": \"" << escape_json_str(urt.camada_a.municipio) << "\",\n"
       << "    \"uf\": \"" << escape_json_str(urt.camada_a.uf) << "\",\n"
       << "    \"localidade_referencia\": \"" << escape_json_str(urt.camada_a.localidade_referencia) << "\",\n"
       << "    \"coordenadas\": {\n"
       << "      \"latitude\": " << std::setprecision(6) << urt.camada_a.coordenadas.latitude << ",\n"
       << "      \"longitude\": " << std::setprecision(6) << urt.camada_a.coordenadas.longitude << ",\n"
       << "      \"valida\": " << (urt.camada_a.coordenadas.tem_coordenadas_validas() ? "true" : "false") << "\n"
       << "    },\n"
       << "    \"responsavel_tecnico\": {\n"
       << "      \"nome\": \"" << escape_json_str(urt.camada_a.responsavel_tecnico.nome) << "\",\n"
       << "      \"instituicao\": \"" << escape_json_str(urt.camada_a.responsavel_tecnico.instituicao) << "\",\n"
       << "      \"contato\": \"" << escape_json_str(urt.camada_a.responsavel_tecnico.contato) << "\"\n"
       << "    },\n"
       << "    \"propriedade\": {\n"
       << "      \"nome\": \"" << escape_json_str(urt.camada_a.propriedade.nome) << "\",\n"
       << "      \"proprietario_responsavel\": \"" << escape_json_str(urt.camada_a.propriedade.proprietario_responsavel) << "\",\n"
       << "      \"area_total_ha\": " << std::setprecision(2) << urt.camada_a.propriedade.area_total_ha << ",\n"
       << "      \"atividade_principal\": \"" << escape_json_str(urt.camada_a.propriedade.atividade_principal) << "\"\n"
       << "    }\n"
       << "  },\n"
       << "  \"camada_b\": {\n"
       << "    \"tipo_sistema\": \"" << escape_json_str(to_string(urt.camada_b.tipo_sistema)) << "\",\n"
       << "    \"area_urt_ha\": " << std::setprecision(2) << urt.camada_b.area_urt_ha << ",\n"
       << "    \"data_implantacao\": \"" << escape_json_str(urt.camada_b.data_implantacao) << "\",\n"
       << "    \"situacao_atual\": \"" << escape_json_str(to_string(urt.camada_b.situacao_atual)) << "\",\n"
       << "    \"especies_arboreas\": \"" << escape_json_str(urt.camada_b.especies_arboreas) << "\",\n"
       << "    \"pastagem_forrageira\": \"" << escape_json_str(urt.camada_b.pastagem_forrageira) << "\",\n"
       << "    \"componente_animal\": \"" << escape_json_str(urt.camada_b.componente_animal) << "\",\n"
       << "    \"arranjo_espacial\": \"" << escape_json_str(to_string(urt.camada_b.arranjo_espacial)) << "\",\n"
       << "    \"espacamento\": \"" << escape_json_str(urt.camada_b.espacamento) << "\",\n"
       << "    \"manejo_atual\": \"" << escape_json_str(urt.camada_b.manejo_atual) << "\"\n"
       << "  },\n"
       << "  \"camada_c\": {\n"
       << "    \"fonte_informacao\": \"" << escape_json_str(urt.camada_c.fonte_informacao) << "\",\n"
       << "    \"data_ultima_atualizacao\": \"" << escape_json_str(urt.camada_c.data_ultima_atualizacao) << "\",\n"
       << "    \"observacoes\": \"" << escape_json_str(urt.camada_c.observacoes) << "\",\n"
       << "    \"registros_documentais\": [";

    for (std::size_t i = 0; i < urt.camada_c.registros_documentais.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << escape_json_str(urt.camada_c.registros_documentais[i]) << "\"";
    }
    ss << "]\n"
       << "  },\n"
       << "  \"historico_transicoes\": [";

    for (std::size_t i = 0; i < urt.historico_transicoes.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << to_json(urt.historico_transicoes[i]);
    }
    ss << "]\n"
       << "}";
    return ss.str();
}

std::string to_json(const std::vector<UrtRecord>& lista) {
    std::ostringstream ss;
    ss << "[\n";
    for (std::size_t i = 0; i < lista.size(); ++i) {
        if (i > 0) ss << ",\n";
        ss << to_json(lista[i]);
    }
    ss << "\n]";
    return ss.str();
}

std::string to_json(const UrtMetrics& metrics) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4);
    ss << "{\n"
       << "  \"total_cadastros\": " << metrics.total_cadastros << ",\n"
       << "  \"com_coordenadas_validas\": " << metrics.com_coordenadas_validas << ",\n"
       << "  \"com_responsavel_tecnico\": " << metrics.com_responsavel_tecnico << ",\n"
       << "  \"com_camada_b_completa\": " << metrics.com_camada_b_completa << ",\n"
       << "  \"taxa_coordenadas_validas\": " << metrics.taxa_coordenadas_validas << ",\n"
       << "  \"taxa_responsavel_tecnico\": " << metrics.taxa_responsavel_tecnico << ",\n"
       << "  \"taxa_camada_b_completa\": " << metrics.taxa_camada_b_completa << ",\n"
       << "  \"por_instituicao\": {";

    bool first = true;
    for (const auto& [inst, count] : metrics.por_instituicao) {
        if (!first) ss << ", ";
        ss << "\"" << escape_json_str(inst) << "\": " << count;
        first = false;
    }
    ss << "},\n  \"por_tipo_sistema\": {";

    first = true;
    for (const auto& [tipo, count] : metrics.por_tipo_sistema) {
        if (!first) ss << ", ";
        ss << "\"" << escape_json_str(tipo) << "\": " << count;
        first = false;
    }
    ss << "},\n  \"por_situacao\": {";

    first = true;
    for (const auto& [sit, count] : metrics.por_situacao) {
        if (!first) ss << ", ";
        ss << "\"" << escape_json_str(sit) << "\": " << count;
        first = false;
    }
    ss << "},\n  \"por_status_validacao\": {";

    first = true;
    for (const auto& [stat, count] : metrics.por_status_validacao) {
        if (!first) ss << ", ";
        ss << "\"" << escape_json_str(stat) << "\": " << count;
        first = false;
    }
    ss << "},\n  \"por_uf\": {";

    first = true;
    for (const auto& [uf, count] : metrics.por_uf) {
        if (!first) ss << ", ";
        ss << "\"" << escape_json_str(uf) << "\": " << count;
        first = false;
    }
    ss << "}\n}";
    return ss.str();
}

std::optional<UrtRecord> parse_urt_json(std::string_view json_str) {
    if (json_str.empty()) return std::nullopt;

    UrtRecord urt;
    urt.id = extrair_campo_string(json_str, "id");
    urt.versao = extrair_campo_int(json_str, "versao", 1);
    urt.status_validacao = parse_status_validacao(extrair_campo_string(json_str, "status_validacao"));

    auto camada_a_obj = extrair_objeto(json_str, "camada_a");
    if (!camada_a_obj.empty()) {
        urt.camada_a.codigo_urt = extrair_campo_string(camada_a_obj, "codigo_urt");
        urt.camada_a.nome_local = extrair_campo_string(camada_a_obj, "nome_local");
        urt.camada_a.instituicao_referencia = extrair_campo_string(camada_a_obj, "instituicao_referencia");
        urt.camada_a.municipio = extrair_campo_string(camada_a_obj, "municipio");
        urt.camada_a.uf = extrair_campo_string(camada_a_obj, "uf");
        urt.camada_a.localidade_referencia = extrair_campo_string(camada_a_obj, "localidade_referencia");

        auto coord_obj = extrair_objeto(camada_a_obj, "coordenadas");
        if (!coord_obj.empty()) {
            urt.camada_a.coordenadas.latitude = extrair_campo_double(coord_obj, "latitude", 0.0);
            urt.camada_a.coordenadas.longitude = extrair_campo_double(coord_obj, "longitude", 0.0);
            urt.camada_a.coordenadas.valida = extrair_campo_bool(coord_obj, "valida", true);
        }

        auto resp_obj = extrair_objeto(camada_a_obj, "responsavel_tecnico");
        if (!resp_obj.empty()) {
            urt.camada_a.responsavel_tecnico.nome = extrair_campo_string(resp_obj, "nome");
            urt.camada_a.responsavel_tecnico.instituicao = extrair_campo_string(resp_obj, "instituicao");
            urt.camada_a.responsavel_tecnico.contato = extrair_campo_string(resp_obj, "contato");
        }

        auto prop_obj = extrair_objeto(camada_a_obj, "propriedade");
        if (!prop_obj.empty()) {
            urt.camada_a.propriedade.nome = extrair_campo_string(prop_obj, "nome");
            urt.camada_a.propriedade.proprietario_responsavel = extrair_campo_string(prop_obj, "proprietario_responsavel");
            urt.camada_a.propriedade.area_total_ha = extrair_campo_double(prop_obj, "area_total_ha", 0.0);
            urt.camada_a.propriedade.atividade_principal = extrair_campo_string(prop_obj, "atividade_principal");
        }
    } else {
        // Formato flat (caso venha do formulário)
        urt.camada_a.codigo_urt = extrair_campo_string(json_str, "codigo_urt");
        urt.camada_a.nome_local = extrair_campo_string(json_str, "nome_local");
        urt.camada_a.instituicao_referencia = extrair_campo_string(json_str, "instituicao_referencia");
        urt.camada_a.municipio = extrair_campo_string(json_str, "municipio");
        urt.camada_a.uf = extrair_campo_string(json_str, "uf");
        urt.camada_a.localidade_referencia = extrair_campo_string(json_str, "localidade_referencia");
        urt.camada_a.coordenadas.latitude = extrair_campo_double(json_str, "latitude", 0.0);
        urt.camada_a.coordenadas.longitude = extrair_campo_double(json_str, "longitude", 0.0);
        urt.camada_a.coordenadas.valida = urt.camada_a.coordenadas.tem_coordenadas_validas();
        urt.camada_a.responsavel_tecnico.nome = extrair_campo_string(json_str, "responsavel_nome");
        urt.camada_a.responsavel_tecnico.instituicao = extrair_campo_string(json_str, "responsavel_instituicao");
        urt.camada_a.responsavel_tecnico.contato = extrair_campo_string(json_str, "responsavel_contato");
        urt.camada_a.propriedade.nome = extrair_campo_string(json_str, "propriedade_nome");
        urt.camada_a.propriedade.proprietario_responsavel = extrair_campo_string(json_str, "proprietario");
        urt.camada_a.propriedade.area_total_ha = extrair_campo_double(json_str, "propriedade_area_total_ha", 0.0);
        urt.camada_a.propriedade.atividade_principal = extrair_campo_string(json_str, "atividade_principal");
    }

    auto camada_b_obj = extrair_objeto(json_str, "camada_b");
    if (!camada_b_obj.empty()) {
        urt.camada_b.tipo_sistema = parse_sistema_tipo(extrair_campo_string(camada_b_obj, "tipo_sistema"));
        urt.camada_b.area_urt_ha = extrair_campo_double(camada_b_obj, "area_urt_ha", 0.0);
        urt.camada_b.data_implantacao = extrair_campo_string(camada_b_obj, "data_implantacao");
        urt.camada_b.situacao_atual = parse_situacao_atual(extrair_campo_string(camada_b_obj, "situacao_atual"));
        urt.camada_b.especies_arboreas = extrair_campo_string(camada_b_obj, "especies_arboreas");
        urt.camada_b.pastagem_forrageira = extrair_campo_string(camada_b_obj, "pastagem_forrageira");
        urt.camada_b.componente_animal = extrair_campo_string(camada_b_obj, "componente_animal");
        urt.camada_b.arranjo_espacial = parse_arranjo_espacial(extrair_campo_string(camada_b_obj, "arranjo_espacial"));
        urt.camada_b.espacamento = extrair_campo_string(camada_b_obj, "espacamento");
        urt.camada_b.manejo_atual = extrair_campo_string(camada_b_obj, "manejo_atual");
    } else {
        urt.camada_b.tipo_sistema = parse_sistema_tipo(extrair_campo_string(json_str, "tipo_sistema"));
        urt.camada_b.area_urt_ha = extrair_campo_double(json_str, "area_urt_ha", 0.0);
        urt.camada_b.data_implantacao = extrair_campo_string(json_str, "data_implantacao");
        urt.camada_b.situacao_atual = parse_situacao_atual(extrair_campo_string(json_str, "situacao_atual"));
        urt.camada_b.especies_arboreas = extrair_campo_string(json_str, "especies_arboreas");
        urt.camada_b.pastagem_forrageira = extrair_campo_string(json_str, "pastagem_forrageira");
        urt.camada_b.componente_animal = extrair_campo_string(json_str, "componente_animal");
        urt.camada_b.arranjo_espacial = parse_arranjo_espacial(extrair_campo_string(json_str, "arranjo_espacial"));
        urt.camada_b.espacamento = extrair_campo_string(json_str, "espacamento");
        urt.camada_b.manejo_atual = extrair_campo_string(json_str, "manejo_atual");
    }

    auto camada_c_obj = extrair_objeto(json_str, "camada_c");
    if (!camada_c_obj.empty()) {
        urt.camada_c.fonte_informacao = extrair_campo_string(camada_c_obj, "fonte_informacao");
        urt.camada_c.data_ultima_atualizacao = extrair_campo_string(camada_c_obj, "data_ultima_atualizacao");
        urt.camada_c.observacoes = extrair_campo_string(camada_c_obj, "observacoes");
        urt.camada_c.registros_documentais = extrair_campo_array_strings(camada_c_obj, "registros_documentais");
    } else {
        urt.camada_c.fonte_informacao = extrair_campo_string(json_str, "fonte_informacao");
        urt.camada_c.data_ultima_atualizacao = extrair_campo_string(json_str, "data_ultima_atualizacao");
        urt.camada_c.observacoes = extrair_campo_string(json_str, "observacoes");
    }

    // Parse histórico de transições
    auto hist_pos = json_str.find("\"historico_transicoes\"");
    if (hist_pos != std::string_view::npos) {
        auto arr_start = json_str.find('[', hist_pos);
        if (arr_start != std::string_view::npos) {
            std::size_t cur = arr_start + 1;
            while (cur < json_str.size()) {
                auto item_start = json_str.find('{', cur);
                auto arr_end = json_str.find(']', cur);
                if (item_start == std::string_view::npos || (arr_end != std::string_view::npos && item_start > arr_end)) {
                    break;
                }

                int depth = 1;
                std::size_t end = item_start + 1;
                while (end < json_str.size() && depth > 0) {
                    if (json_str[end] == '{') ++depth;
                    else if (json_str[end] == '}') --depth;
                    else if (json_str[end] == '"') {
                        ++end;
                        while (end < json_str.size() && json_str[end] != '"') {
                            if (json_str[end] == '\\') ++end;
                            ++end;
                        }
                    }
                    ++end;
                }

                if (depth == 0) {
                    std::string_view rec_json = json_str.substr(item_start, end - item_start);
                    TransitoryReceipt rec;
                    rec.id = extrair_campo_string(rec_json, "id");
                    rec.timestamp = extrair_campo_string(rec_json, "timestamp");
                    rec.de_status = parse_status_validacao(extrair_campo_string(rec_json, "de_status"));
                    rec.para_status = parse_status_validacao(extrair_campo_string(rec_json, "para_status"));
                    rec.autoridade = extrair_campo_string(rec_json, "autoridade");
                    rec.motivo = extrair_campo_string(rec_json, "motivo");
                    rec.predecessor_state_id = extrair_campo_string(rec_json, "predecessor_state_id");
                    rec.successor_state_id = extrair_campo_string(rec_json, "successor_state_id");
                    urt.historico_transicoes.push_back(std::move(rec));
                    cur = end;
                } else {
                    break;
                }
            }
        }
    }

    if (urt.id.empty()) {
        if (!urt.camada_a.codigo_urt.empty()) {
            urt.id = to_lower_case(urt.camada_a.codigo_urt);
            std::replace(urt.id.begin(), urt.id.end(), ' ', '-');
        }
    }

    return urt;
}

std::vector<UrtRecord> parse_urts_lista_json(std::string_view json_str) {
    std::vector<UrtRecord> lista;
    auto start_arr = json_str.find('[');
    if (start_arr == std::string_view::npos) return lista;

    std::size_t pos = start_arr + 1;
    while (pos < json_str.size()) {
        auto obj_start = json_str.find('{', pos);
        if (obj_start == std::string_view::npos) break;

        int profundidade = 1;
        std::size_t end = obj_start + 1;
        while (end < json_str.size() && profundidade > 0) {
            if (json_str[end] == '{') ++profundidade;
            else if (json_str[end] == '}') --profundidade;
            else if (json_str[end] == '"') {
                ++end;
                while (end < json_str.size() && json_str[end] != '"') {
                    if (json_str[end] == '\\') ++end;
                    ++end;
                }
            }
            ++end;
        }

        if (profundidade == 0) {
            std::string_view obj_json = json_str.substr(obj_start, end - obj_start);
            auto parsed = parse_urt_json(obj_json);
            if (parsed) {
                lista.push_back(std::move(*parsed));
            }
            pos = end;
        } else {
            break;
        }
    }
    return lista;
}

UrtRepository::UrtRepository(std::filesystem::path store_path, std::optional<std::filesystem::path> seed_path)
    : storage_{std::move(store_path)}, seed_path_{std::move(seed_path)} {
    inicializar();
}

bool UrtRepository::inicializar() {
    std::unique_lock lock(mutex_);
    dados_.clear();
    ordem_insercao_.clear();

    auto records = storage_.load(seed_path_);
    for (auto& r : records) {
        if (!r.id.empty() && dados_.find(r.id) == dados_.end()) {
            ordem_insercao_.push_back(r.id);
            dados_[r.id] = std::move(r);
        }
    }
    return true;
}

bool UrtRepository::persist_all_unlocked(std::string* erro) {
    std::vector<UrtRecord> lista;
    lista.reserve(ordem_insercao_.size());
    for (const auto& id : ordem_insercao_) {
        auto it = dados_.find(id);
        if (it != dados_.end()) {
            lista.push_back(it->second);
        }
    }
    return storage_.save_atomic(lista, erro);
}

bool UrtRepository::is_storage_healthy() const noexcept {
    return storage_.is_healthy();
}

bool UrtRepository::carregar_arquivo_json(const std::string& caminho) {
    std::ifstream file(caminho);
    if (!file.is_open()) return false;

    std::stringstream ss;
    ss << file.rdbuf();
    return carregar_string_json(ss.str());
}

bool UrtRepository::carregar_string_json(std::string_view conteudo) {
    std::unique_lock lock(mutex_);
    auto lista = parse_urts_lista_json(conteudo);
    if (lista.empty()) return false;

    dados_.clear();
    ordem_insercao_.clear();
    for (auto& urt : lista) {
        if (!urt.id.empty() && dados_.find(urt.id) == dados_.end()) {
            ordem_insercao_.push_back(urt.id);
            dados_[urt.id] = std::move(urt);
        }
    }
    persist_all_unlocked();
    return true;
}

std::string UrtRepository::exportar_json() const {
    std::shared_lock lock(mutex_);
    std::vector<UrtRecord> lista;
    lista.reserve(ordem_insercao_.size());
    for (const auto& id : ordem_insercao_) {
        auto it = dados_.find(id);
        if (it != dados_.end()) {
            lista.push_back(it->second);
        }
    }
    return to_json(lista);
}

std::vector<UrtRecord> UrtRepository::listar(const FiltroUrt& filtro) const {
    std::shared_lock lock(mutex_);
    std::vector<UrtRecord> resultado;

    const std::string filtro_inst = to_lower_case(filtro.instituicao);
    const std::string filtro_muni = to_lower_case(filtro.municipio);
    const std::string filtro_uf = to_lower_case(filtro.uf);
    const std::string filtro_tipo = to_lower_case(filtro.tipo_sistema);
    const std::string filtro_situ = to_lower_case(filtro.situacao);
    const std::string filtro_stat = to_lower_case(filtro.status_validacao);
    const std::string filtro_busca = to_lower_case(filtro.busca_texto);

    for (const auto& id : ordem_insercao_) {
        auto it = dados_.find(id);
        if (it == dados_.end()) continue;
        const auto& urt = it->second;

        if (!filtro_inst.empty() && to_lower_case(urt.camada_a.instituicao_referencia) != filtro_inst) {
            continue;
        }
        if (!filtro_muni.empty() && to_lower_case(urt.camada_a.municipio) != filtro_muni) {
            continue;
        }
        if (!filtro_uf.empty() && to_lower_case(urt.camada_a.uf) != filtro_uf) {
            continue;
        }
        if (!filtro_tipo.empty() && to_lower_case(to_string(urt.camada_b.tipo_sistema)) != filtro_tipo) {
            continue;
        }
        if (!filtro_situ.empty() && to_lower_case(to_string(urt.camada_b.situacao_atual)) != filtro_situ) {
            continue;
        }
        if (!filtro_stat.empty() && to_lower_case(to_string(urt.status_validacao)) != filtro_stat) {
            continue;
        }
        if (!filtro_busca.empty()) {
            const std::string hay = to_lower_case(urt.camada_a.codigo_urt + " " +
                                                  urt.camada_a.nome_local + " " +
                                                  urt.camada_a.municipio + " " +
                                                  urt.camada_a.instituicao_referencia + " " +
                                                  urt.camada_b.especies_arboreas + " " +
                                                  urt.camada_b.pastagem_forrageira);
            if (hay.find(filtro_busca) == std::string::npos) {
                continue;
            }
        }

        resultado.push_back(urt);
    }
    return resultado;
}

std::optional<UrtRecord> UrtRepository::buscar_por_id(std::string_view id) const {
    std::shared_lock lock(mutex_);
    const auto it = dados_.find(std::string{id});
    if (it != dados_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool UrtRepository::adicionar(UrtRecord urt, std::string* erro) {
    std::unique_lock lock(mutex_);

    auto validacao = validar_invariantes(urt);
    if (!validacao.valido) {
        if (erro != nullptr && !validacao.erros.empty()) {
            *erro = validacao.erros.front();
        }
        return false;
    }

    if (dados_.find(urt.id) != dados_.end()) {
        if (erro != nullptr) *erro = "URT com ID '" + urt.id + "' já existe.";
        return false;
    }

    const std::string id = urt.id;
    ordem_insercao_.push_back(id);
    dados_[id] = std::move(urt);

    if (!persist_all_unlocked(erro)) {
        return false;
    }

    return true;
}

bool UrtRepository::atualizar(UrtRecord urt, std::string* erro) {
    std::unique_lock lock(mutex_);

    auto it = dados_.find(urt.id);
    if (it == dados_.end()) {
        if (erro != nullptr) *erro = "URT não encontrada para atualização.";
        return false;
    }

    // Preservação estrita de história append-only:
    // Atualização normal NUNCA pode apagar ou reescrever recibos existentes de transição
    auto prev_history = it->second.historico_transicoes;
    if (urt.historico_transicoes.empty()) {
        urt.historico_transicoes = std::move(prev_history);
    } else {
        for (const auto& r : prev_history) {
            bool exists = false;
            for (const auto& nr : urt.historico_transicoes) {
                if (nr.id == r.id) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                urt.historico_transicoes.push_back(r);
            }
        }
    }

    auto validacao = validar_invariantes(urt);
    if (!validacao.valido) {
        if (erro != nullptr && !validacao.erros.empty()) {
            *erro = validacao.erros.front();
        }
        return false;
    }

    it->second = std::move(urt);

    if (!persist_all_unlocked(erro)) {
        return false;
    }

    return true;
}

TransitionResult UrtRepository::transicionar(
    std::string_view id,
    StatusValidacao novo_status,
    std::string_view autoridade,
    std::string_view motivo) {

    std::unique_lock lock(mutex_);
    auto it = dados_.find(std::string{id});
    if (it == dados_.end()) {
        return TransitionResult{
            .sucesso = false,
            .erro = "URT não encontrada para transição.",
            .recibo = {},
        };
    }

    TransitionRequest request{
        .urt_id = std::string{id},
        .novo_status = novo_status,
        .autoridade = std::string{autoridade},
        .motivo = std::string{motivo},
        .timestamp = "",
    };

    auto res = executar_transicao_governada(it->second, request);
    if (res.sucesso) {
        persist_all_unlocked(&res.erro);
    }
    return res;
}

UrtMetrics UrtRepository::obter_metricas() const {
    std::shared_lock lock(mutex_);
    std::vector<UrtRecord> lista;
    lista.reserve(ordem_insercao_.size());
    for (const auto& id : ordem_insercao_) {
        auto it = dados_.find(id);
        if (it != dados_.end()) {
            lista.push_back(it->second);
        }
    }
    return calcular_metricas(lista);
}

std::size_t UrtRepository::total() const {
    std::shared_lock lock(mutex_);
    return dados_.size();
}

} // namespace sister::urt
