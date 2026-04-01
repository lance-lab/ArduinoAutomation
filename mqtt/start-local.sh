#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "${SCRIPT_DIR}/data"

exec mosquitto -c "${SCRIPT_DIR}/mosquitto.conf" -v
