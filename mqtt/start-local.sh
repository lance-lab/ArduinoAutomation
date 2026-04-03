#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "${HOME}/.local/state/arduinoautomation-mqtt"

exec mosquitto -c "${SCRIPT_DIR}/mosquitto.conf" -v
