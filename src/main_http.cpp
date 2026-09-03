// SPDX-License-Identifier: GPL-3.0-or-later
#include "sister/urt/http.hpp"
#include "sister/urt/repository.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

namespace {

volatile std::sig_atomic_t g_stop = 0;

void handle_signal(int) {
    g_stop = 1;
}

struct Options {
    std::string bind_address{"127.0.0.1"};
    unsigned short port{8094};
    std::string web_index{"web/index.html"};
    std::filesystem::path store_file{"data/authoritative_store.json"};
    std::optional<std::filesystem::path> seed_file{std::nullopt};
};

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "--bind" && i + 1 < argc) {
            options.bind_address = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            const auto val = std::stoul(argv[++i]);
            if (val == 0 || val > 65535) {
                throw std::invalid_argument("Porta fora do intervalo permitido (1-65535)");
            }
            options.port = static_cast<unsigned short>(val);
        } else if (arg == "--web-index" && i + 1 < argc) {
            options.web_index = argv[++i];
        } else if (arg == "--store" && i + 1 < argc) {
            options.store_file = argv[++i];
        } else if (arg == "--seed" && i + 1 < argc) {
            options.seed_file = std::filesystem::path{argv[++i]};
        } else if (arg == "--no-seed") {
            options.seed_file = std::nullopt;
        } else if (arg == "--data" && i + 1 < argc) {
            // Compatibilidade transitória: --data passa a significar seed inicial,
            // nunca uma fonte a ser reaplicada sobre o store autoritativo.
            options.seed_file = std::filesystem::path{argv[++i]};
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Uso: sister-urt-http [--bind 127.0.0.1] [--port 8094] "
                         "[--web-index web/index.html] [--store data/authoritative_store.json] "
                         "[--seed <arquivo>|--no-seed]\n";
            std::exit(0);
        }
    }
    return options;
}

std::string read_text_file(const std::string& path) {
    std::ifstream input{path, std::ios::binary};
    if (!input) {
        return "";
    }
    return std::string{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

void write_all(int fd, const std::string& payload) {
    std::size_t written = 0;
    while (written < payload.size()) {
        const auto rc = ::send(fd, payload.data() + written, payload.size() - written, MSG_NOSIGNAL);
        if (rc < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::string{"Erro no send: "} + std::strerror(errno));
        }
        written += static_cast<std::size_t>(rc);
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse_options(argc, argv);

        sister::urt::UrtRepository repo{options.store_file, options.seed_file};
        std::cout << "[SisTer-URT] Store autoritativo: '" << options.store_file.string()
                  << "' | registros: " << repo.total();
        if (options.seed_file.has_value()) {
            std::cout << " | seed inicial: '" << options.seed_file->string() << "'";
        } else {
            std::cout << " | seed inicial: desabilitado";
        }
        std::cout << '\n';

        const auto web_html = read_text_file(options.web_index);
        const sister::urt::http::Application app{repo, web_html};

        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw std::runtime_error(std::string{"Falha no socket: "} + std::strerror(errno));
        }

        const int yes = 1;
        if (::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
            ::close(server_fd);
            throw std::runtime_error(std::string{"Falha no setsockopt: "} + std::strerror(errno));
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(options.port);
        if (::inet_pton(AF_INET, options.bind_address.c_str(), &address.sin_addr) != 1) {
            ::close(server_fd);
            throw std::invalid_argument("Endereço de bind inválido");
        }

        if (::bind(server_fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0) {
            const auto msg = std::string{"Falha no bind: "} + std::strerror(errno);
            ::close(server_fd);
            throw std::runtime_error(msg);
        }

        if (::listen(server_fd, 32) < 0) {
            const auto msg = std::string{"Falha no listen: "} + std::strerror(errno);
            ::close(server_fd);
            throw std::runtime_error(msg);
        }

        std::cout << "========================================================\n";
        std::cout << "  SisTer-URT · Cadastro e Caracterização de URTs\n";
        std::cout << "  Interface Web e API REST disponíveis em:\n";
        std::cout << "  http://" << options.bind_address << ':' << options.port << "/\n";
        std::cout << "========================================================\n";
        std::cout.flush();

        while (g_stop == 0) {
            pollfd ready{.fd = server_fd, .events = POLLIN, .revents = 0};
            const int poll_rc = ::poll(&ready, 1, 250);
            if (poll_rc < 0) {
                if (errno == EINTR) continue;
                throw std::runtime_error(std::string{"Falha no poll: "} + std::strerror(errno));
            }
            if (poll_rc == 0) continue;

            const int client_fd = ::accept(server_fd, nullptr, nullptr);
            if (client_fd < 0) {
                if (errno == EINTR) continue;
                throw std::runtime_error(std::string{"Falha no accept: "} + std::strerror(errno));
            }

            try {
                std::string raw_request;
                raw_request.resize(32768U);
                const auto received = ::recv(client_fd, raw_request.data(), raw_request.size(), 0);
                if (received > 0) {
                    raw_request.resize(static_cast<std::size_t>(received));
                    const auto req = sister::urt::http::parse_raw_http_request(raw_request);
                    const auto resp = app.handle(req);
                    write_all(client_fd, sister::urt::http::serialize_response(resp));
                }
            } catch (const std::exception& e) {
                const sister::urt::http::Response err_resp{
                    .status = 500,
                    .body = R"({"error":"internal_error","message":")" + std::string{e.what()} + R"("})",
                };
                try {
                    write_all(client_fd, sister::urt::http::serialize_response(err_resp));
                } catch (...) {}
            }
            ::close(client_fd);
        }

        ::close(server_fd);
        std::cout << "\n[SisTer-URT] Servidor finalizado com sucesso.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Erro fatal: " << e.what() << '\n';
        return 1;
    }
}
