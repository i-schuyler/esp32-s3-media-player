#!/usr/bin/env bash
set -euo pipefail

env_name="${1:-esp32-s3-devkitc-1-n32r16v}"
expected_board="${2:-esp32-s3-devkitc-1-n32r16v}"
expected_partitions="${3:-partitions/partitions_32mb.csv}"
expected_framework="${4:-arduino}"
expected_flash_mode="${5:-opi}"
expected_memory_type="${6:-opi_opi}"
expected_psram_type="${7:-opi}"

if [[ ! -f platformio.ini ]]; then
  echo "::error::platformio.ini not found."
  exit 1
fi

default_env="$(awk '$1=="default_envs"{print $3; exit}' platformio.ini)"
if [[ "$default_env" != "$env_name" ]]; then
  echo "::error::default_envs mismatch. Expected ${env_name}, found ${default_env:-<empty>}"
  exit 1
fi

if ! grep -Eq "^\[env:${env_name}\]$" platformio.ini; then
  echo "::error::Missing [env:${env_name}] section in platformio.ini"
  exit 1
fi

read_env_value() {
  local key="$1"
  awk -v target="[env:${env_name}]" -v key="$key" '
  $0==target {in_env=1; next}
  in_env && /^\[/ {in_env=0}
  in_env && $1==key {print $3; exit}
' platformio.ini
}

board_value="$(read_env_value board)"
if [[ "$board_value" != "$expected_board" ]]; then
  echo "::error::board mismatch in [env:${env_name}]. Expected ${expected_board}, found ${board_value:-<empty>}"
  exit 1
fi

framework_value="$(read_env_value framework)"
if [[ "$framework_value" != "$expected_framework" ]]; then
  echo "::error::framework mismatch in [env:${env_name}]. Expected ${expected_framework}, found ${framework_value:-<empty>}"
  exit 1
fi

flash_mode_value="$(read_env_value board_build.flash_mode)"
if [[ "$flash_mode_value" != "$expected_flash_mode" ]]; then
  echo "::error::board_build.flash_mode mismatch in [env:${env_name}]. Expected ${expected_flash_mode}, found ${flash_mode_value:-<empty>}"
  exit 1
fi

memory_type_value="$(read_env_value board_build.memory_type)"
if [[ "$memory_type_value" != "$expected_memory_type" ]]; then
  echo "::error::board_build.memory_type mismatch in [env:${env_name}]. Expected ${expected_memory_type}, found ${memory_type_value:-<empty>}"
  exit 1
fi

psram_type_value="$(read_env_value board_build.psram_type)"
if [[ "$psram_type_value" != "$expected_psram_type" ]]; then
  echo "::error::board_build.psram_type mismatch in [env:${env_name}]. Expected ${expected_psram_type}, found ${psram_type_value:-<empty>}"
  exit 1
fi

partitions_value="$(read_env_value board_build.partitions)"
if [[ "$partitions_value" != "$expected_partitions" ]]; then
  echo "::error::board_build.partitions mismatch in [env:${env_name}]. Expected ${expected_partitions}, found ${partitions_value:-<empty>}"
  exit 1
fi

if [[ ! -f "$expected_partitions" ]]; then
  echo "::error::Partition file not found: ${expected_partitions}"
  exit 1
fi

read_partition_size() {
  local part_name="$1"
  awk -F',' -v part="$part_name" '
    {
      name=$1
      size=$5
      gsub(/[[:space:]]/, "", name)
      gsub(/[[:space:]]/, "", size)
      if (name == part) {
        print size
        exit
      }
    }
  ' "$expected_partitions"
}

app0_size_hex="$(read_partition_size app0)"
app1_size_hex="$(read_partition_size app1)"

if [[ -z "$app0_size_hex" || -z "$app1_size_hex" ]]; then
  echo "::error::Unable to find app0/app1 partition sizes in ${expected_partitions}"
  exit 1
fi

app0_size_dec=$((app0_size_hex))
app1_size_dec=$((app1_size_hex))
if (( app0_size_dec < 0x100000 || app1_size_dec < 0x100000 )); then
  echo "::error::OTA app partitions are smaller than 1MB (app0=${app0_size_hex}, app1=${app1_size_hex})"
  exit 1
fi

if ! grep -Fq "pio run -e ${env_name} -t upload" README.md; then
  echo "::error::README USB upload command does not match release env ${env_name}."
  exit 1
fi

echo "PARITY: OK env=${env_name} framework=${expected_framework} board=${expected_board} flash_mode=${expected_flash_mode} memory_type=${expected_memory_type} psram_type=${expected_psram_type} partitions=${expected_partitions} app0=${app0_size_hex} app1=${app1_size_hex}"
