#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${PROJECT_DIR}"

PORT="${1:-8094}"
URT_STORE_PATH="${URT_STORE_PATH:-data/authoritative_store.json}"
URT_SEED_PATH="${URT_SEED_PATH:-}"

server_args=(
  --bind 127.0.0.1
  --port "${PORT}"
  --web-index web/index.html
  --store "${URT_STORE_PATH}"
)

if [[ -n "${URT_SEED_PATH}" ]]; then
  server_args+=(--seed "${URT_SEED_PATH}")
else
  server_args+=(--no-seed)
fi

echo "==> Iniciando SisTer-URT HTTP Server na porta ${PORT}..."
./build/sister-urt-http "${server_args[@]}"
