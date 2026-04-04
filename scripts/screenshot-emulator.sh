#!/usr/bin/env bash

set -euo pipefail

emulator="${PEBBLE_EMULATOR:-basalt}"
screenshot_args=()

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
      screenshot_args+=("$@")
      break
      ;;
    *)
      screenshot_args+=("$1")
      shift
      ;;
  esac
done

if [[ "${#screenshot_args[@]}" -eq 0 ]]; then
  output_dir="screenshot/tmp"
  timestamp="$(date +"%Y-%m-%dT%H-%M-%S")"
  output_path="${output_dir}/${timestamp}.png"
  mkdir -p "$output_dir"
  pebble screenshot "$output_path" --emulator "$emulator"
else
  pebble screenshot --emulator "$emulator" "${screenshot_args[@]}"
fi
