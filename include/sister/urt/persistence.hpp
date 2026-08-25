// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sister/urt/types.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sister::urt {

class AuthoritativeStorage {
public:
    explicit AuthoritativeStorage(std::filesystem::path store_path = "data/authoritative_store.json") noexcept;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return store_path_; }

    // Carrega registros da persistência autoritativa local.
    // Se o arquivo autoritativo não existir e seed_path for fornecido, importa os dados piloto/seed.
    [[nodiscard]] std::vector<UrtRecord> load(std::optional<std::filesystem::path> seed_path = std::nullopt);

    // Salva atomicamente a lista de registros.
    [[nodiscard]] bool save_atomic(const std::vector<UrtRecord>& records, std::string* error_msg = nullptr) const noexcept;

    // Checa se o armazenamento está acessível e funcional.
    [[nodiscard]] bool is_healthy() const noexcept;

private:
    std::filesystem::path store_path_;
};

} // namespace sister::urt
