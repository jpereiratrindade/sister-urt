// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/http.hpp"

#include <algorithm>
#include <sstream>

namespace sister::urt::http {

namespace {

std::string url_decode(std::string_view in) {
    std::string out;
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            auto from_hex = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            int high = from_hex(in[i + 1]);
            int low = from_hex(in[i + 2]);
            if (high >= 0 && low >= 0) {
                out += static_cast<char>((high << 4) | low);
                i += 2;
                continue;
            }
        } else if (in[i] == '+') {
            out += ' ';
            continue;
        }
        out += in[i];
    }
    return out;
}

} // namespace

std::string Request::get_query_param(std::string_view key) const {
    if (query_string.empty()) return "";
    std::string needle = std::string{key} + "=";
    auto pos = query_string.find(needle);
    if (pos == std::string::npos) return "";

    auto val_start = pos + needle.size();
    auto val_end = query_string.find('&', val_start);
    if (val_end == std::string::npos) {
        return url_decode(query_string.substr(val_start));
    }
    return url_decode(query_string.substr(val_start, val_end - val_start));
}

std::string reason_phrase(int status) noexcept {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 422: return "Unprocessable Entity";
        case 500: return "Internal Server Error";
        default:  return "OK";
    }
}

std::string serialize_response(const Response& response) {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << response.status << ' ' << reason_phrase(response.status) << "\r\n"
       << "Content-Type: " << response.content_type << "\r\n"
       << "Content-Length: " << response.body.size() << "\r\n"
       << "Access-Control-Allow-Origin: *\r\n"
       << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
       << "Access-Control-Allow-Headers: Content-Type\r\n"
       << "Connection: close\r\n\r\n"
       << response.body;
    return ss.str();
}

Request parse_raw_http_request(std::string_view raw_data) {
    Request req;
    auto line_end = raw_data.find("\r\n");
    if (line_end == std::string_view::npos) {
        line_end = raw_data.find('\n');
        if (line_end == std::string_view::npos) return req;
    }

    std::string_view req_line = raw_data.substr(0, line_end);
    auto sp1 = req_line.find(' ');
    if (sp1 == std::string_view::npos) return req;
    auto sp2 = req_line.find(' ', sp1 + 1);
    if (sp2 == std::string_view::npos) return req;

    req.method = std::string{req_line.substr(0, sp1)};
    std::string_view full_path = req_line.substr(sp1 + 1, sp2 - sp1 - 1);

    auto q_pos = full_path.find('?');
    if (q_pos != std::string_view::npos) {
        req.path = std::string{full_path.substr(0, q_pos)};
        req.query_string = std::string{full_path.substr(q_pos + 1)};
    } else {
        req.path = std::string{full_path};
    }

    // Procura o corpo (após \r\n\r\n ou \n\n)
    auto body_pos = raw_data.find("\r\n\r\n");
    if (body_pos != std::string_view::npos) {
        req.body = std::string{raw_data.substr(body_pos + 4)};
    } else {
        body_pos = raw_data.find("\n\n");
        if (body_pos != std::string_view::npos) {
            req.body = std::string{raw_data.substr(body_pos + 2)};
        }
    }

    return req;
}

Application::Application(UrtRepository& repository, std::string index_html) noexcept
    : repository_{repository}, index_html_{std::move(index_html)} {}

Response Application::handle(const Request& request) const {
    if (request.method == "OPTIONS") {
        return Response{.status = 204, .content_type = "text/plain", .body = ""};
    }

    if (request.method == "GET") {
        if (request.path == "/" || request.path == "/index.html") {
            if (!index_html_.empty()) {
                return Response{
                    .status = 200,
                    .content_type = "text/html; charset=utf-8",
                    .body = index_html_,
                };
            }
            return Response{.status = 200, .content_type = "text/plain", .body = "SisTer-URT Web Server"};
        }
        if (request.path == "/api/health" || request.path == "/health") {
            return handle_get_health();
        }
        if (request.path == "/api/metrics") {
            return handle_get_metrics();
        }
        if (request.path == "/api/export") {
            return Response{
                .status = 200,
                .content_type = "application/json; charset=utf-8",
                .body = repository_.exportar_json(),
            };
        }
        if (request.path == "/_sister/manifest") {
            return handle_get_sister_manifest();
        }
        if (request.path == "/_sister/health") {
            return handle_get_sister_health();
        }
        if (request.path == "/_sister/ready") {
            return handle_get_sister_ready();
        }
        if (request.path == "/_sister/capabilities") {
            return handle_get_sister_capabilities();
        }
        if (request.path == "/_sister/identity") {
            return handle_get_sister_identity();
        }
        if (request.path == "/api/urts" || request.path == "/api/urts/") {
            return handle_get_urts(request);
        }
        if (request.path.rfind("/api/urts/", 0) == 0) {
            std::string_view id = std::string_view{request.path}.substr(10);
            return handle_get_urt_by_id(id);
        }
    }

    if (request.method == "POST") {
        if (request.path == "/api/urts" || request.path == "/api/urts/") {
            return handle_post_urt(request);
        }
        // POST /api/urts/{id}/validate
        const std::string_view prefix = "/api/urts/";
        if (request.path.rfind(prefix, 0) == 0) {
            auto suffix_pos = request.path.find("/validate", prefix.size());
            if (suffix_pos != std::string_view::npos) {
                std::string id = request.path.substr(prefix.size(), suffix_pos - prefix.size());
                return handle_post_validate(id, request);
            }
        }
    }

    return Response{
        .status = 404,
        .content_type = "application/json; charset=utf-8",
        .body = R"({"error":"not_found","message":"Recurso não encontrado"})",
    };
}

Response Application::handle_get_health() const {
    const auto total = repository_.total();
    std::ostringstream ss;
    ss << "{\n"
       << "  \"status\": \"ready\",\n"
       << "  \"system\": \"sister_urt\",\n"
       << "  \"version\": \"0.1.0\",\n"
       << "  \"total_cadastros\": " << total << ",\n"
       << "  \"core\": \"ready\",\n"
       << "  \"governance\": \"active\"\n"
       << "}";
    return Response{
        .status = 200,
        .content_type = "application/json; charset=utf-8",
        .body = ss.str(),
    };
}

Response Application::handle_get_urts(const Request& req) const {
    FiltroUrt filtro;
    filtro.instituicao = req.get_query_param("instituicao");
    filtro.municipio = req.get_query_param("municipio");
    filtro.uf = req.get_query_param("uf");
    filtro.tipo_sistema = req.get_query_param("tipo");
    filtro.situacao = req.get_query_param("situacao");
    filtro.status_validacao = req.get_query_param("status");
    filtro.busca_texto = req.get_query_param("q");

    auto lista = repository_.listar(filtro);
    return Response{
        .status = 200,
        .content_type = "application/json; charset=utf-8",
        .body = to_json(lista),
    };
}

Response Application::handle_get_urt_by_id(std::string_view id) const {
    auto urt = repository_.buscar_por_id(id);
    if (!urt) {
        return Response{
            .status = 404,
            .content_type = "application/json; charset=utf-8",
            .body = R"({"error":"not_found","message":"URT não encontrada"})",
        };
    }
    return Response{
        .status = 200,
        .content_type = "application/json; charset=utf-8",
        .body = to_json(*urt),
    };
}

Response Application::handle_post_urt(const Request& req) const {
    auto parsed = parse_urt_json(req.body);
    if (!parsed) {
        return Response{
            .status = 400,
            .content_type = "application/json; charset=utf-8",
            .body = R"({"error":"bad_request","message":"Corpo JSON inválido"})",
        };
    }

    std::string erro;
    if (!repository_.adicionar(*parsed, &erro)) {
        return Response{
            .status = 422,
            .content_type = "application/json; charset=utf-8",
            .body = R"({"error":"validation_error","message":")" + erro + R"("})",
        };
    }

    return Response{
        .status = 201,
        .content_type = "application/json; charset=utf-8",
        .body = to_json(*parsed),
    };
}

Response Application::handle_post_validate(std::string_view id, const Request& req) const {
    // Extrai novo_status, autoridade, motivo do corpo JSON
    std::string status_str = "validado";
    std::string autoridade;
    std::string motivo;

    auto pos_stat = req.body.find("\"novo_status\"");
    if (pos_stat != std::string::npos) {
        auto pos_colon = req.body.find(':', pos_stat);
        auto pos_q1 = req.body.find('"', pos_colon);
        auto pos_q2 = req.body.find('"', pos_q1 + 1);
        if (pos_q1 != std::string::npos && pos_q2 != std::string::npos) {
            status_str = req.body.substr(pos_q1 + 1, pos_q2 - pos_q1 - 1);
        }
    }

    auto pos_aut = req.body.find("\"autoridade\"");
    if (pos_aut != std::string::npos) {
        auto pos_colon = req.body.find(':', pos_aut);
        auto pos_q1 = req.body.find('"', pos_colon);
        auto pos_q2 = req.body.find('"', pos_q1 + 1);
        if (pos_q1 != std::string::npos && pos_q2 != std::string::npos) {
            autoridade = req.body.substr(pos_q1 + 1, pos_q2 - pos_q1 - 1);
        }
    }

    auto pos_mot = req.body.find("\"motivo\"");
    if (pos_mot != std::string::npos) {
        auto pos_colon = req.body.find(':', pos_mot);
        auto pos_q1 = req.body.find('"', pos_colon);
        auto pos_q2 = req.body.find('"', pos_q1 + 1);
        if (pos_q1 != std::string::npos && pos_q2 != std::string::npos) {
            motivo = req.body.substr(pos_q1 + 1, pos_q2 - pos_q1 - 1);
        }
    }

    if (autoridade.empty()) autoridade = "Técnico Responsável";
    if (motivo.empty()) motivo = "Validação cadastral registrada na interface";

    auto res = repository_.transicionar(id, parse_status_validacao(status_str), autoridade, motivo);
    if (!res.sucesso) {
        return Response{
            .status = 400,
            .content_type = "application/json; charset=utf-8",
            .body = R"({"error":"transition_error","message":")" + res.erro + R"("})",
        };
    }

    return Response{
        .status = 200,
        .content_type = "application/json; charset=utf-8",
        .body = to_json(res.recibo),
    };
}

Response Application::handle_get_metrics() const {
    const auto metrics = repository_.obter_metricas();
    return Response{
        .status = 200,
        .content_type = "application/json; charset=utf-8",
        .body = to_json(metrics),
    };
}

Response Application::handle_get_sister_manifest() const {
    const std::string manifest_json = R"json({
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
    return Response{
        .status = 200,
        .content_type = "application/json; charset=utf-8",
        .body = manifest_json,
    };
}

Response Application::handle_get_sister_health() const {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"schema\": \"sister.subsystem.health/1.0.0\",\n"
       << "  \"system_id\": \"sister_urt\",\n"
       << "  \"status\": \"ok\",\n"
       << "  \"checked_at\": \"" << gerar_timestamp_iso8601() << "\"\n"
       << "}";
    return Response{
        .status = 200,
        .content_type = "application/json; charset=utf-8",
        .body = ss.str(),
    };
}

Response Application::handle_get_sister_ready() const {
    const auto total = repository_.total();
    std::ostringstream ss;
    ss << "{\n"
       << "  \"schema\": \"sister.subsystem.readiness/1.0.0\",\n"
       << "  \"system_id\": \"sister_urt\",\n"
       << "  \"status\": \"ready\",\n"
       << "  \"contract_version\": \"1.0.0\",\n"
       << "  \"total_cadastros\": " << total << ",\n"
       << "  \"dependencies\": {\n"
       << "    \"domain_core\": \"ready\",\n"
       << "    \"repository\": \"ready\",\n"
       << "    \"governance\": \"active\"\n"
       << "  }\n"
       << "}";
    return Response{
        .status = 200,
        .content_type = "application/json; charset=utf-8",
        .body = ss.str(),
    };
}

Response Application::handle_get_sister_capabilities() const {
    const std::string caps_json = R"json({
  "schema": "sister.subsystem.capabilities/1.0.0",
  "system_id": "sister_urt",
  "contract": "sister.subsystem/1.0.0",
  "capabilities": [
    {"id": "urt.cadastro.read", "description": "Consultar catálogo e ficha de URTs.", "risk": "low"},
    {"id": "urt.cadastro.create", "description": "Cadastrar nova Unidade de Referência Tecnológica.", "risk": "medium"},
    {"id": "urt.validation.transition", "description": "Executar transição governada de validação epistêmica.", "risk": "high"},
    {"id": "urt.metrics.evaluate", "description": "Calcular indicadores de cobertura e qualidade do cadastro.", "risk": "low"},
    {"id": "urt.dataset.export", "description": "Exportar base observacional consolidada em JSON.", "risk": "low"}
  ]
})json";
    return Response{
        .status = 200,
        .content_type = "application/json; charset=utf-8",
        .body = caps_json,
    };
}

Response Application::handle_get_sister_identity() const {
    const std::string id_json = R"json({
  "schema": "sister.subsystem.identity/1.0.0",
  "system_id": "sister_urt",
  "name": "SisTer-URT",
  "version": "0.1.0",
  "functional_role": "domain",
  "domain_authority": "Unidades de Referência Tecnológica (Sistemas Silvipastoris, ASP e SAF)",
  "governance_model": "SisTer-Praxis Epistemic Transitions",
  "partner_institutions": [
    "CRSul",
    "CNPF",
    "EMATER-RS",
    "EPAGRI",
    "EMATER-PR"
  ],
  "boundaries": {
    "state_authority": "exclusive",
    "immutable_receipts": true,
    "provenance_tracking": true
  }
})json";
    return Response{
        .status = 200,
        .content_type = "application/json; charset=utf-8",
        .body = id_json,
    };
}

} // namespace sister::urt::http
