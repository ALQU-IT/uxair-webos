#!/bin/sh
# Read-only, bounded Rockhopper media-status sampler. Never print raw Luna JSON.
APP=com.github.depius.uxair.hevcprobe

case "${1:-snapshot}" in
    snapshot) count=1 ;;
    watch) count=12 ;;
    *) echo '[b7-observe] usage: sh b7-media-observe.sh snapshot|watch' >&2; exit 2 ;;
esac

field_number() {
    printf '%s' "$1" | sed -n "s/.*\"$2\"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p" | head -n 1
}

rect() {
    case "$1" in
        *\"$2\"*) part=${1#*\"$2\"}; part=${part#*\{}; part=${part%%\}*} ;;
        *) echo unknown; return ;;
    esac
    rw=$(field_number "$part" width)
    rh=$(field_number "$part" height)
    if [ -n "$rw" ] && [ -n "$rh" ]; then echo "${rw}x${rh}"; else echo unknown; fi
}

query_luna() {
    if ! command -v timeout >/dev/null 2>&1 || ! command -v luna-send >/dev/null 2>&1; then
        echo '[b7-observe] luna=unavailable-or-unbounded'
        return
    fi
    # Command output is held only in memory and capped; never print raw JSON or error text.
    response=$(timeout 2 luna-send -n 1 -f luna://com.webos.service.videooutput/getStatus '{"subscribe":false}' 2>/dev/null | head -c 4096)
    endpoint=videooutput
    if ! printf '%s' "$response" | grep -q '"returnValue"[[:space:]]*:[[:space:]]*true'; then
        response=$(timeout 2 luna-send -n 1 -f luna://com.webos.service.avoutput/video/getStatus '{"subscribe":false}' 2>/dev/null | head -c 4096)
        endpoint=avoutput
    fi
    if printf '%s' "$response" | grep -q '"returnValue"[[:space:]]*:[[:space:]]*true'; then
        if printf '%s' "$response" | grep -q '"connectedSource"[[:space:]]*:[[:space:]]*"VDEC"'; then
            source=vdec-present
        else
            source=no-vdec-marker
        fi
        echo "[b7-observe] video_service=$endpoint source=$source first_source_rect=$(rect "$response" sourceInput) first_output_rect=$(rect "$response" displayOutput) attribution=unverified"
    else
        echo '[b7-observe] video_service=unavailable-or-denied'
    fi
    response=$(timeout 2 luna-send -n 1 -f luna://com.webos.media/getForegroundAppInfo '{}' 2>/dev/null | head -c 4096)
    if printf '%s' "$response" | grep -q '"returnValue"[[:space:]]*:[[:space:]]*true'; then
        case "$response" in
            *"$APP"*) foreground=probe ;;
            *) foreground=other ;;
        esac
        case "$response" in
            *'"acbs"'*) acbs=${response#*\"acbs\"}; acbs=${acbs#*\{}; acbs=${acbs%%\}*}
                aw=$(field_number "$acbs" width); ah=$(field_number "$acbs" height)
                if [ -n "$aw" ] && [ -n "$ah" ]; then acb="${aw}x${ah}"; else acb=unknown; fi ;;
            *) acb=unknown ;;
        esac
        echo "[b7-observe] media_service=available foreground=$foreground first_acb_window=$acb attribution=unverified"
    else
        echo '[b7-observe] media_service=unavailable-or-denied'
    fi
}

snapshot() {
    stamp=$(date -u '+%Y-%m-%dT%H:%M:%SZ' 2>/dev/null || echo unknown)
    echo "[b7-observe] snapshot=$1 utc=$stamp"
    if command -v pidof >/dev/null 2>&1; then
        pid=$(pidof hevc-probe 2>/dev/null | awk '{print $1}')
    else
        pid=
    fi
    case "$pid" in
        ''|*[!0-9]*) echo '[b7-observe] probe_process=absent-or-unavailable' ;;
        *) echo "[b7-observe] probe_process=present pid=$pid"
           if [ -r "/proc/$pid/status" ]; then
               awk '/^State:/ {print "[b7-observe] proc_state=" $2} /^VmRSS:/ {print "[b7-observe] rss_kib=" $2}' "/proc/$pid/status" | head -n 2
           fi ;;
    esac
    if [ -r /proc/interrupts ]; then
        irq=$(awk 'tolower($0) ~ /hevc|vdec|vpu/ {n=0; for(i=2;i<=NF && $i ~ /^[0-9]+$/;i++) n+=$i; print n; exit}' /proc/interrupts)
        if [ -n "$irq" ]; then echo "[b7-observe] candidate_decoder_irq_total=$irq attribution=unverified";
        else echo '[b7-observe] candidate_decoder_irq=unavailable'; fi
    else
        echo '[b7-observe] candidate_decoder_irq=unavailable'
    fi
    found=0
    for state in /sys/kernel/debug/dri/0/state /sys/kernel/debug/dri/1/state; do
        [ -r "$state" ] || continue
        found=1
        echo "[b7-observe] drm_state=$(basename "$(dirname "$state")")"
        # Emit only plane ordinals and numeric source/destination rectangles.
        sed -n '1,160p' "$state" 2>/dev/null | awk '
            /plane\[/ {plane++; if(plane>8) exit}
            /^[[:space:]]*crtc-pos=/ {v=$0; sub(/.*crtc-pos=/,"",v); gsub(/[^0-9x+.-]/,"",v); if(length(v)>0) print "[b7-observe] plane" plane "_dst=" v}
            /^[[:space:]]*src-pos=/ {v=$0; sub(/.*src-pos=/,"",v); gsub(/[^0-9x+.-]/,"",v); if(length(v)>0) print "[b7-observe] plane" plane "_src=" v}
        ' | head -n 16
    done
    [ "$found" -eq 1 ] || echo '[b7-observe] drm_state=unavailable'
    case "$1" in 1|4|7|10) query_luna ;; esac
}

i=1
while [ "$i" -le "$count" ]; do
    snapshot "$i"
    [ "$i" -eq "$count" ] || sleep 1
    i=$((i+1))
done
