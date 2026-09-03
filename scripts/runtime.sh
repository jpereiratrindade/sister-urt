#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Stable installed-runtime boundary for sister.component/sister.runtime.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

ACTION="${1:-run}"

load_deployment_binding() {
  local resolved="${SISTER_RESOLVED_DEPLOYMENT_FILE:-}"
  [[ -n "${resolved}" ]] || return 0
  [[ -f "${resolved}" ]] || {
    printf '[FAIL] deployment resolvido ausente: %s\n' "${resolved}" >&2
    return 1
  }
  command -v jq >/dev/null 2>&1 || {
    printf '[FAIL] jq é necessário para consumir deployment resolvido.\n' >&2
    return 1
  }

  local system_id transport
  system_id="$(jq -er '.system_id' "${PROJECT_DIR}/.sister/component.json")"
  transport="$(jq -er --arg id "${system_id}" \
    '.components[] | select(.system_id == $id) | .runtime.transport' \
    "${resolved}")"
  [[ "${transport}" == "tcp" ]] || {
    printf '[FAIL] runtime URT ainda requer binding TCP.\n' >&2
    return 1
  }
  export URT_ADDRESS
  export URT_PORT
  URT_ADDRESS="$(jq -er --arg id "${system_id}" \
    '.components[] | select(.system_id == $id) | .runtime.listen' \
    "${resolved}")"
  URT_PORT="$(jq -er --arg id "${system_id}" \
    '.components[] | select(.system_id == $id) | .runtime.port' \
    "${resolved}")"
}

load_deployment_binding

URT_ADDRESS="${URT_ADDRESS:-127.0.0.1}"
URT_PORT="${URT_PORT:-8094}"
URT_STATE_DIR="${URT_STATE_DIR:-${SISTER_RUNTIME_STATE_DIR:-${XDG_STATE_HOME:-${HOME}/.local/state}/sister/workstation/urt}}"
URT_RUNTIME_DIR="${URT_RUNTIME_DIR:-${SISTER_RUNTIME_RUN_DIR:-${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}/sister/urt}}"
export SISTER_RUNTIME_STATE_DIR="${URT_STATE_DIR}"
export SISTER_RUNTIME_RUN_DIR="${URT_RUNTIME_DIR}"

URT_STORE_PATH="${URT_STORE_PATH:-${URT_STATE_DIR}/authoritative_store.json}"
URT_SEED_PATH="${URT_SEED_PATH:-}"
URT_PILOT_SNAPSHOT="${PROJECT_DIR}/data/fixtures/pilot_authoritative_store.snapshot.json"
URT_WEB_INDEX="${URT_WEB_INDEX:-${PROJECT_DIR}/web/index.html}"
URT_BINARY="${URT_BINARY:-${PROJECT_DIR}/build/sister-urt-http}"
URT_PID_FILE="${URT_PID_FILE:-${URT_RUNTIME_DIR}/sister-urt.pid}"
URT_LOG_PATH="${URT_LOG_PATH:-${URT_RUNTIME_DIR}/sister-urt.log}"

mkdir -p "${URT_STATE_DIR}" "${URT_RUNTIME_DIR}"

require_binary() {
  [[ -x "${URT_BINARY}" ]] || {
    printf '[FAIL] sister-urt-http não encontrado/executável: %s\n' "${URT_BINARY}" >&2
    return 1
  }
}

read_pid() {
  [[ -r "${URT_PID_FILE}" ]] || return 1

  local pid
  IFS= read -r pid < "${URT_PID_FILE}"

  [[ "${pid}" =~ ^[0-9]+$ ]] || return 1
  printf '%s\n' "${pid}"
}

pid_is_alive() {
  local pid="$1"
  kill -0 "${pid}" 2>/dev/null
}

pid_matches_runtime() {
  local pid="$1"
  local cmdline="/proc/${pid}/cmdline"

  [[ -r "${cmdline}" ]] || return 1

  tr '\0' '\n' < "${cmdline}" \
    | grep -Fqx -- "${URT_BINARY}"
}

running_pid() {
  local pid

  pid="$(read_pid)" || return 1

  if ! pid_is_alive "${pid}"; then
    rm -f "${URT_PID_FILE}"
    return 1
  fi

  if ! pid_matches_runtime "${pid}"; then
    printf '[FAIL] PID %s não pertence ao runtime URT esperado; recusando operar.\n' "${pid}" >&2
    return 2
  fi

  printf '%s\n' "${pid}"
}

server_args=(
  --bind "${URT_ADDRESS}"
  --port "${URT_PORT}"
  --web-index "${URT_WEB_INDEX}"
  --store "${URT_STORE_PATH}"
)

if [[ -n "${URT_SEED_PATH}" ]]; then
  server_args+=(--seed "${URT_SEED_PATH}")
else
  server_args+=(--no-seed)
fi

quarantine_legacy_pilot_store() {
  [[ -f "${URT_STORE_PATH}" ]] || return 0

  local known_payload
  for known_payload in \
    "${URT_PILOT_SNAPSHOT}" \
    "${PROJECT_DIR}/data/pilot_urts.json"
  do
    [[ -f "${known_payload}" ]] || continue
    if cmp -s -- "${URT_STORE_PATH}" "${known_payload}"; then
      local stamp quarantine_path
      stamp="$(date -u +%Y%m%dT%H%M%SZ)"
      quarantine_path="${URT_STORE_PATH}.pilot-quarantined.${stamp}"
      mv -- "${URT_STORE_PATH}" "${quarantine_path}"
      printf '[NOTICE] store legado composto apenas pelos 16 registros piloto foi retirado do estado ativo e preservado em: %s\n' \
        "${quarantine_path}"
      return 0
    fi
  done
}

start_background() {
  require_binary

  local pid
  local rc=0

  pid="$(running_pid)" || rc="$?"

  if [[ "${rc}" -eq 0 ]]; then
    printf '[PASS] SisTer-URT já está ativo (pid=%s).\n' "${pid}"
    return 0
  fi

  if [[ "${rc}" -eq 2 ]]; then
    return 1
  fi

  rm -f "${URT_PID_FILE}"

  quarantine_legacy_pilot_store

  nohup "${URT_BINARY}" "${server_args[@]}" \
    >> "${URT_LOG_PATH}" 2>&1 &

  pid="$!"
  printf '%s\n' "${pid}" > "${URT_PID_FILE}"

  sleep 0.2

  if ! pid_is_alive "${pid}"; then
    printf '[FAIL] SisTer-URT encerrou durante o start. Log: %s\n' "${URT_LOG_PATH}" >&2
    rm -f "${URT_PID_FILE}"
    return 1
  fi

  printf '[PASS] SisTer-URT iniciado (pid=%s).\n' "${pid}"
}

stop_runtime() {
  local pid
  local rc=0

  pid="$(running_pid)" || rc="$?"

  if [[ "${rc}" -eq 2 ]]; then
    return 1
  fi

  if [[ "${rc}" -ne 0 ]]; then
    rm -f "${URT_PID_FILE}"
    printf '[PASS] SisTer-URT já estava parado.\n'
    return 0
  fi

  kill -TERM "${pid}"

  local i
  for i in {1..50}; do
    if ! pid_is_alive "${pid}"; then
      rm -f "${URT_PID_FILE}"
      printf '[PASS] SisTer-URT parado.\n'
      return 0
    fi
    sleep 0.1
  done

  if pid_matches_runtime "${pid}"; then
    kill -KILL "${pid}" 2>/dev/null || true
  fi

  rm -f "${URT_PID_FILE}"
  printf '[PASS] SisTer-URT parado após timeout de encerramento gracioso.\n'
}

status_runtime() {
  local pid
  local rc=0

  pid="$(running_pid)" || rc="$?"

  if [[ "${rc}" -eq 0 ]]; then
    printf 'running pid=%s\n' "${pid}"
    return 0
  fi

  if [[ "${rc}" -eq 2 ]]; then
    return 1
  fi

  printf 'stopped\n'
  return 3
}

http_observation() {
  local path="$1"

  command -v curl >/dev/null 2>&1 || {
    printf '[FAIL] curl é necessário para observar o adapter HTTP corrente.\n' >&2
    return 1
  }

  curl \
    --fail \
    --silent \
    --show-error \
    --max-time 3 \
    "http://${URT_ADDRESS}:${URT_PORT}${path}"
}

health_runtime() {
  http_observation "/_sister/health"
  printf '\n'
}

readiness_runtime() {
  local body
  body="$(http_observation "/_sister/ready")"

  printf '%s\n' "${body}"

  grep -Eq '"status"[[:space:]]*:[[:space:]]*"ready"' <<< "${body}"
}

run_foreground() {
  require_binary
  quarantine_legacy_pilot_store

  printf '%s\n' "$$" > "${URT_PID_FILE}"

  exec "${URT_BINARY}" "${server_args[@]}"
}

case "${ACTION}" in
  start)
    start_background
    ;;
  stop)
    stop_runtime
    ;;
  restart)
    stop_runtime
    start_background
    ;;
  status)
    status_runtime
    ;;
  health)
    health_runtime
    ;;
  readiness)
    readiness_runtime
    ;;
  run)
    # Compatibilidade temporária com o launcher histórico do sister-infra:
    # sem argumento, o runtime permanece em foreground.
    run_foreground
    ;;
  *)
    printf 'Uso: %s {start|stop|restart|status|health|readiness|run}\n' "$0" >&2
    exit 2
    ;;
esac
