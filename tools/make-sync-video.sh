#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p dist
# White flashes and short tones share the same source clock. Use a camera
# recording both screens to measure end-to-end delay rather than estimating it.
ffmpeg -hide_banner -loglevel error -y \
  -f lavfi -i 'color=c=black:s=1280x720:r=30:d=30' \
  -f lavfi -i 'aevalsrc=if(lt(mod(t\,1)\,0.05)\,0.2*sin(2*PI*880*t)\,0):s=44100:d=30' \
  -vf "drawbox=x=0:y=0:w=iw:h=ih:color=white:t=fill:enable='lt(mod(t,1),0.0667)'" \
  -c:v libx264 -pix_fmt yuv420p -profile:v baseline -preset fast -c:a aac -ac 2 \
  -movflags +faststart dist/sync-test.mp4
