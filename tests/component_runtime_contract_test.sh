#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DESCRIPTOR="${ROOT}/.sister/component.json"
RUNTIME="${ROOT}/scripts/runtime.sh"

python3 - "${DESCRIPTOR}" <<'PY'
import json
import sys
from pathlib import Path

path = Path(sys.argv[1])
doc = json.loads(path.read_text(encoding="utf-8"))

assert doc["schema"] == "sister.component/1.0.0"
assert doc["component_id"] == "urt"
assert doc["system_id"] == "sister_urt"
assert doc["deployment_role"] == "system"
assert doc["semantic_contract"] == "sister.subsystem/1.0.0"

runtime = doc["runtime"]
assert runtime["schema"] == "sister.runtime/1.0.0"
assert runtime["entrypoint"] == "scripts/runtime.sh"
assert set(runtime["actions"]) == {
    "start",
    "stop",
    "restart",
    "status",
    "health",
    "readiness",
}
assert runtime["state_policy"] == "persistent-external"

forbidden = {
    "address",
    "binding",
    "endpoint",
    "host",
    "listen_address",
    "listen_port",
    "port",
    "public_url",
    "socket",
    "transport",
    "url",
}

def property_names(value):
    names = set()
    if isinstance(value, dict):
        names.update(value)
        for child in value.values():
            names.update(property_names(child))
    elif isinstance(value, list):
        for child in value:
            names.update(property_names(child))
    return names

assert not (property_names(doc) & forbidden)
PY

grep -Fq 'SISTER_RESOLVED_DEPLOYMENT_FILE' "${RUNTIME}"
grep -Fq '.components[] | select(.system_id == $id)' "${RUNTIME}"

TMP="$(mktemp -d)"
cleanup() {
  URT_BINARY="${TMP}/fake-sister-urt-http" \
  URT_STATE_DIR="${TMP}/state" \
  URT_RUNTIME_DIR="${TMP}/run" \
  URT_WEB_INDEX="${TMP}/index.html" \
  URT_SEED_PATH="${TMP}/seed.json" \
  PATH="${TMP}/bin:${PATH}" \
    "${RUNTIME}" stop >/dev/null 2>&1 || true

  rm -rf "${TMP}"
}
trap cleanup EXIT

mkdir -p "${TMP}/bin" "${TMP}/state" "${TMP}/run"
printf '<html></html>\n' > "${TMP}/index.html"
printf '[]\n' > "${TMP}/seed.json"

cat > "${TMP}/fake-sister-urt-http" <<'SH'
#!/usr/bin/env bash
trap 'exit 0' TERM INT
while true; do
  sleep 1
done
SH
chmod +x "${TMP}/fake-sister-urt-http"

cat > "${TMP}/bin/curl" <<'SH'
#!/usr/bin/env bash
case "$*" in
  *"/_sister/health")
    printf '{"status":"ok"}\n'
    ;;
  *"/_sister/ready")
    printf '{"status":"ready"}\n'
    ;;
  *)
    exit 22
    ;;
esac
SH
chmod +x "${TMP}/bin/curl"

run_runtime() {
  URT_BINARY="${TMP}/fake-sister-urt-http" \
  URT_STATE_DIR="${TMP}/state" \
  URT_RUNTIME_DIR="${TMP}/run" \
  URT_WEB_INDEX="${TMP}/index.html" \
  URT_SEED_PATH="${TMP}/seed.json" \
  PATH="${TMP}/bin:${PATH}" \
    "${RUNTIME}" "$@"
}

run_runtime start
run_runtime status
run_runtime health >/dev/null
run_runtime readiness >/dev/null
run_runtime restart
run_runtime status
run_runtime stop

if run_runtime status >/dev/null 2>&1; then
  printf '[FAIL] status deveria falhar após stop.\n' >&2
  exit 1
fi

# Regressão: precedência e herança de SISTER_RUNTIME_RUN_DIR e SISTER_RUNTIME_STATE_DIR
SANDBOX_TMP="$(mktemp -d)"
(
  export SISTER_RUNTIME_RUN_DIR="${SANDBOX_TMP}/run"
  export SISTER_RUNTIME_STATE_DIR="${SANDBOX_TMP}/state"
  mkdir -p "${SANDBOX_TMP}/run" "${SANDBOX_TMP}/state"

  URT_BINARY="${TMP}/fake-sister-urt-http" \
  URT_WEB_INDEX="${TMP}/index.html" \
  URT_SEED_PATH="${TMP}/seed.json" \
  PATH="${TMP}/bin:${PATH}" \
    "${RUNTIME}" start >/dev/null

  [[ -f "${SANDBOX_TMP}/run/sister-urt.pid" ]] || {
    printf '[FAIL] sister-urt.pid não foi gravado dentro de SISTER_RUNTIME_RUN_DIR.\n' >&2
    exit 1
  }

  child_pid="$(cat "${SANDBOX_TMP}/run/sister-urt.pid")"
  if ! tr '\0' '\n' < "/proc/${child_pid}/environ" | grep -Fqx "SISTER_RUNTIME_RUN_DIR=${SANDBOX_TMP}/run"; then
    printf '[FAIL] processo filho não herdou SISTER_RUNTIME_RUN_DIR no /proc/environ.\n' >&2
    exit 1
  fi

  URT_BINARY="${TMP}/fake-sister-urt-http" \
  URT_WEB_INDEX="${TMP}/index.html" \
  URT_SEED_PATH="${TMP}/seed.json" \
  PATH="${TMP}/bin:${PATH}" \
    "${RUNTIME}" stop >/dev/null
)
rm -rf "${SANDBOX_TMP}"

printf '[PASS] sister.component/1.0.0 + sister.runtime/1.0.0 smoke local.\n'
