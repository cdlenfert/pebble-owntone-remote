#!/usr/bin/env bash

set -euo pipefail

ip="${IP:-}"
install_args=()
ip_set_from_arg=0

while (($#)); do
  case "$1" in
    --)
      shift
      install_args+=("$@")
      break
      ;;
    -*)
      install_args+=("$1")
      shift
      ;;
    *)
      if [[ $ip_set_from_arg -eq 0 ]]; then
        ip="$1"
        ip_set_from_arg=1
      else
        install_args+=("$1")
      fi
      shift
      ;;
  esac
done

if [[ -z "$ip" ]]; then
  printf 'usage: %s [phone-ip] [-- pebble-install-args...]\n' "$0" >&2
  printf 'or set IP in .env and run: mise install-phone\n' >&2
  exit 1
fi

scripts/build.sh
pebble install --phone "$ip" "${install_args[@]+"${install_args[@]}"}"
