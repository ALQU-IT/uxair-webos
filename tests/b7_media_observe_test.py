#!/usr/bin/env python3
"""Exercise the read-only collector with synthetic, privacy-sensitive Luna replies."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / "tools/b7-media-observe.sh"


def main():
    with tempfile.TemporaryDirectory() as tmp:
        tools = Path(tmp)
        (tools / "timeout").write_text('#!/bin/sh\nshift\nexec "$@"\n')
        (tools / "pidof").write_text('#!/bin/sh\nexit 1\n')
        (tools / "luna-send").write_text('''#!/bin/sh
case "$4" in
  *videooutput*)
    if [ "${B7_TEST_FALLBACK:-0}" = 1 ]; then echo '{"returnValue":false,"context":"SECRET_DENIED"}'; exit; fi
    echo '{"video":[{"connectedSource":"VDEC","sourceInput":{"width":3840,"height":2160},"displayOutput":{"width":1920,"height":1080},"sink":"MAIN","context":"SECRET_CONTEXT"}],"returnValue":true}' ;;
  *avoutput*)
    echo '{"video":[{"connectedSource":"VDEC","sourceInput":{"width":3840,"height":2160},"displayOutput":{"width":1920,"height":1080},"sink":"MAIN","context":"SECRET_CONTEXT"}],"returnValue":true}' ;;
  *media*)
    echo '{"acbs":[{"width":1920,"height":1080,"pipelineId":"SECRET_PIPELINE"}],"appId":"com.github.danlflh.uxplaywebos.hevcprobe","returnValue":true}' ;;
esac
''')
        for name in ("timeout", "pidof", "luna-send"):
            (tools / name).chmod(0o755)
        env = os.environ.copy()
        env["PATH"] = str(tools) + os.pathsep + env["PATH"]
        result = subprocess.run(["sh", str(SCRIPT), "snapshot"], env=env,
                                capture_output=True, text=True, timeout=10, check=True)
        output = result.stdout
        assert "video_service=videooutput" in output, output
        assert "first_source_rect=3840x2160" in output, output
        assert "first_output_rect=1920x1080" in output, output
        assert "media_service=available foreground=probe" in output, output
        assert "first_acb_window=1920x1080" in output, output
        assert "SECRET_" not in output + result.stderr
        assert len(output) < 5000
        fallback_env = env.copy()
        fallback_env["B7_TEST_FALLBACK"] = "1"
        fallback = subprocess.run(["sh", str(SCRIPT), "snapshot"], env=fallback_env,
                                  capture_output=True, text=True, timeout=10, check=True)
        assert "video_service=avoutput" in fallback.stdout, fallback.stdout
        assert "SECRET_" not in fallback.stdout + fallback.stderr
        bad = subprocess.run(["sh", str(SCRIPT), "invalid"], env=env,
                             capture_output=True, text=True, timeout=5)
        assert bad.returncode == 2
    print("PASS: bounded read-only media collector extracts geometry without Luna identifiers")


if __name__ == "__main__":
    main()
