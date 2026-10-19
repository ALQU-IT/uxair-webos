#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="${1:-build/hevc-samples}"
mkdir -p "$out"
for entry in 1080p:1920x1080 uhd:3840x2160; do
  name="${entry%%:*}"
  size="${entry#*:}"
  ffmpeg -hide_banner -loglevel error -y -filter_threads 2 \
    -f lavfi -i "testsrc2=size=$size:rate=30" -frames:v 30 \
    -an -c:v libx265 -preset ultrafast -pix_fmt yuv420p \
    -color_primaries bt709 -color_trc bt709 -colorspace bt709 \
    -x265-params 'aud=1:repeat-headers=1:keyint=30:min-keyint=30:scenecut=0:bframes=0:pools=2:frame-threads=1:log-level=error' \
    -f hevc "$out/sample-$name.hevc"
done
ffmpeg -hide_banner -loglevel error -y -filter_threads 2 \
  -f lavfi -i 'testsrc2=size=996x2160:rate=30' -frames:v 1 \
  -an -c:v libx265 -preset ultrafast -pix_fmt yuv420p \
  -color_primaries bt709 -color_trc bt709 -colorspace bt709 \
  -x265-params 'aud=1:repeat-headers=1:keyint=30:min-keyint=30:scenecut=0:bframes=0:pools=2:frame-threads=1:log-level=error' \
  -f hevc "$out/sample-portrait.hevc"
ffmpeg -hide_banner -loglevel error -y -filter_threads 2 \
  -f lavfi -i 'testsrc2=size=1920x1080:rate=30' -frames:v 1 \
  -an -c:v libx265 -preset ultrafast -profile:v main10 -pix_fmt yuv420p10le \
  -x265-params 'aud=1:repeat-headers=1:keyint=30:min-keyint=30:scenecut=0:bframes=0:pools=2:frame-threads=1:log-level=error' \
  -f hevc "$out/sample-main10.hevc"
