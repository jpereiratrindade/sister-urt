#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Stable installed-runtime boundary for sister-infra/workstation.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

URT_ADDRESS="${URT_ADDRESS:-127.0.0.1}"
URT_PORT="${URT_PORT:-8094}"
URT_STATE_DIR="${URT_STATE_DIR:-${XDG_STATE_HOME:-${HOME}/.local/state}/sister/workstation/urt}"
URT_STORE_PATH="${URT_STORE_PATH:-${URT_STATE_DIR}/authoritative_store.json}"
URT_SEED_PATH="${URT_SEED_PATH:-${PROJECT_DIR}/data/pilot_urts.json}"
URT_WEB_INDEX="${URT_WEB_INDEX:-${PROJECT_DIR}/web/index.html}"
URT_BINARY="${URT_BINARY:-${PROJECT_DIR}/build/sister-urt-http}"

mkdir -p "${URT_STATE_DIR}"

[[ -x "${URT_BINARY}" ]] || {
  printf '[FAIL] sister-urt-http não encontrado/executável: %s\n' "${URT_BINARY}" >&2
  exit 1
}

exec "${URT_BINARY}" \
  --bind "${URT_ADDRESS}" \
  --port "${URT_PORT}" \
  --web-index "${URT_WEB_INDEX}" \
  --store "${URT_STORE_PATH}" \
  --seed "${URT_SEED_PATH}"
