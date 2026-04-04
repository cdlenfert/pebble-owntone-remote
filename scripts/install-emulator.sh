#!/usr/bin/env bash

set -euo pipefail

emulator="${PEBBLE_EMULATOR:-basalt}"
install_args=()

while (($#)); do
  case "$1" in
    --emulator)
      if [[ -z "${2:-}" ]]; then
        echo "Missing value for --emulator" >&2
        exit 1
      fi
      emulator="$2"
      shift 2
      ;;
    --emulator=*)
      emulator="${1#*=}"
      shift
      ;;
    aplite|basalt|chalk|diorite|emery|flint|gabbro)
      emulator="$1"
      shift
      ;;
    --)
      shift
      install_args+=("$@")
      break
      ;;
    *)
      install_args+=("$1")
      shift
      ;;
  esac
done

scripts/build.sh
pebble install --emulator "$emulator" "${install_args[@]+"${install_args[@]}"}"
