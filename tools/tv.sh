#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
tv_host="${TV_HOST:-}"
tv_key="${TV_KEY:-}"
tv_device="${TV_DEVICE:-}"
ssh_options=(-i "$tv_key" -o BatchMode=yes -o StrictHostKeyChecking=yes -o ConnectTimeout=8 -o LogLevel=ERROR)
app_id="$(python3 -c 'import json;print(json.load(open("packaging/appinfo.json"))["id"])')"
app_dir="/media/developer/apps/usr/palm/applications/$app_id"
app_jail_dir="/var/palm/jail/$app_id$app_dir"
require_device() { [ -n "$tv_device" ] || { echo 'TV_DEVICE is required (configured ares device name).' >&2; exit 2; }; }
require_ssh() { [ -n "$tv_host" ] && [ -n "$tv_key" ] || { echo 'TV_HOST and TV_KEY are required for direct SSH actions.' >&2; exit 2; }; }
case "${1:-}" in
  install)
    require_device
    python3 tools/package.py
    version="$(python3 -c 'import json;print(json.load(open("packaging/appinfo.json"))["version"])')"
    ares-install -d "$tv_device" "dist/${app_id}_${version}_arm.ipk"
    ;;
  launch) require_device; ares-launch -d "$tv_device" "$app_id" ;;
  close) require_device; ares-launch -d "$tv_device" -c "$app_id" ;;
  update-binary)
    require_device; require_ssh
    # Atomic replacement avoids ETXTBSY and preserves on-device pairing state.
    python3 tools/package.py
    scp "${ssh_options[@]}" build/app/uxplay-webos "root@$tv_host:$app_dir/uxplay-webos.next"
    ares-launch -d "$tv_device" -c "$app_id"
    ssh -tt "${ssh_options[@]}" "root@$tv_host" "chmod 755 '$app_dir/uxplay-webos.next' && mv '$app_dir/uxplay-webos.next' '$app_dir/uxplay-webos'"
    ares-launch -d "$tv_device" "$app_id"
    ;;
  logs)
    require_ssh
    mkdir -p build/logs
    ssh -tt "${ssh_options[@]}" "root@$tv_host" "cat '$app_jail_dir/uxplay.log'" > build/logs/tv-latest.log
    echo "build/logs/tv-latest.log"
    ;;
  status)
    require_ssh
    ssh -tt "${ssh_options[@]}" "root@$tv_host" "ps -ef | grep '[u]xplay-webos'; tail -n 8 '$app_jail_dir/uxplay.log'"
    ;;
  loader-check)
    require_ssh
    ssh -tt "${ssh_options[@]}" "root@$tv_host" "LD_TRACE_LOADED_OBJECTS=1 LD_BIND_NOW=1 '$app_dir/uxplay-webos'"
    ;;
  *) echo 'Usage: tools/tv.sh install|launch|close|update-binary|logs|status|loader-check' >&2; exit 2 ;;
esac
