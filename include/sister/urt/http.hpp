// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sister/urt/repository.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace sister::urt::http {

struct Request {
    std::string method;
    std::string path;
    std::string query_string;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    [[nodiscard]] std::string get_query_param(std::string_view key) const;
};

struct Response {
    int status{200};
    std::string content_type{"application/json; charset=utf-8"};
    std::string body;
};

class Application {
public:
    explicit Application(UrtRepository& repository, std::string index_html = {}) noexcept;

    [[nodiscard]] Response handle(const Request& request) const;

private:
    UrtRepository& repository_;
    std::string index_html_;

    [[nodiscard]] Response handle_get_health() const;
    [[nodiscard]] Response handle_get_urts(const Request& req) const;
    [[nodiscard]] Response handle_get_urt_by_id(std::string_view id) const;
    [[nodiscard]] Response handle_post_urt(const Request& req) const;
    [[nodiscard]] Response handle_post_validate(std::string_view id, const Request& req) const;
    [[nodiscard]] Response handle_get_metrics() const;

    // Endpoints técnicos do ecossistema SisTer
    [[nodiscard]] Response handle_get_sister_manifest() const;
    [[nodiscard]] Response handle_get_sister_health() const;
    [[nodiscard]] Response handle_get_sister_ready() const;
    [[nodiscard]] Response handle_get_sister_capabilities() const;
    [[nodiscard]] Response handle_get_sister_identity() const;
};

[[nodiscard]] std::string reason_phrase(int status) noexcept;
[[nodiscard]] std::string serialize_response(const Response& response);
[[nodiscard]] Request parse_raw_http_request(std::string_view raw_data);

} // namespace sister::urt::http
