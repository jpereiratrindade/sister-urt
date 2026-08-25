// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/persistence.hpp"
#include "sister/urt/repository.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <system_error>
#include <unistd.h>

namespace sister::urt {

AuthoritativeStorage::AuthoritativeStorage(std::filesystem::path store_path) noexcept
    : store_path_{std::move(store_path)} {}

std::vector<UrtRecord> AuthoritativeStorage::load(std::optional<std::filesystem::path> seed_path) {
    std::error_code ec;

    // Se o arquivo autoritativo existe, carrega dele exclusivamente
    if (std::filesystem::exists(store_path_, ec) && std::filesystem::is_regular_file(store_path_, ec)) {
        std::ifstream file(store_path_);
        if (file.is_open()) {
            std::stringstream ss;
            ss << file.rdbuf();
            return parse_urts_lista_json(ss.str());
        }
    }

    // Se o arquivo autoritativo não existe mas temos um seed_path (ex: data/pilot_urts.json)
    if (seed_path.has_value() && std::filesystem::exists(*seed_path, ec)) {
        std::ifstream file(*seed_path);
        if (file.is_open()) {
            std::stringstream ss;
            ss << file.rdbuf();
            auto records = parse_urts_lista_json(ss.str());
            // Salva na persistência autoritativa para bootstrapar
            if (!records.empty()) {
                (void)save_atomic(records);
            }
            return records;
        }
    }

    return {};
}

bool AuthoritativeStorage::save_atomic(const std::vector<UrtRecord>& records, std::string* error_msg) const noexcept {
    try {
        std::error_code ec;
        auto parent = store_path_.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                if (error_msg) *error_msg = "Falha ao criar diretório: " + ec.message();
                return false;
            }
        }

        const auto now_ns = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto pid = static_cast<long>(getpid());
        const auto tmp_path = store_path_.string() + ".tmp." + std::to_string(pid) + "." + std::to_string(now_ns);

        {
            std::ofstream out(tmp_path, std::ios::trunc | std::ios::out);
            if (!out.is_open()) {
                if (error_msg) *error_msg = "Falha ao abrir arquivo temporário para escrita: " + tmp_path;
                return false;
            }
            out << to_json(records);
            out.flush();
            if (!out.good()) {
                if (error_msg) *error_msg = "Falha de I/O ao gravar registros.";
                return false;
            }
        }

        // Renomeação atômica
        std::filesystem::rename(tmp_path, store_path_, ec);
        if (ec) {
            if (error_msg) *error_msg = "Falha no rename atômico da persistência: " + ec.message();
            std::filesystem::remove(tmp_path, ec);
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        if (error_msg) *error_msg = std::string("Exceção na persistência: ") + e.what();
        return false;
    }
}

bool AuthoritativeStorage::is_healthy() const noexcept {
    try {
        std::error_code ec;
        if (std::filesystem::exists(store_path_, ec)) {
            return std::filesystem::is_regular_file(store_path_, ec);
        }
        auto parent = store_path_.parent_path();
        if (parent.empty()) return true;
        return !std::filesystem::exists(parent, ec) || std::filesystem::is_directory(parent, ec);
    } catch (...) {
        return false;
    }
}

} // namespace sister::urt
