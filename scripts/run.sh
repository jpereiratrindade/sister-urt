#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${PROJECT_DIR}"

PORT="${1:-8094}"
echo "==> Iniciando SisTer-URT HTTP Server na porta ${PORT}..."
./build/sister-urt-http --bind 127.0.0.1 --port "${PORT}" --web-index web/index.html --store data/authoritative_store.json --seed data/pilot_urts.json
