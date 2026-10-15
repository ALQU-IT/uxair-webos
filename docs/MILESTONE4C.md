# Milestone 4C: direct-media request probe

This is a **temporary development-package diagnostic**, not a direct photo/video player. It does not change AirPlay DNS-SD feature bits, enable HLS, fetch URLs or decode direct media. The production package does not compile this probe. The [pinned UxPlay request gate](https://github.com/FDH2/UxPlay/blob/9f3c2bbc658645533fa1057e76c29ec8c947fa0f/lib/raop.c#L242-L245) still ignores the unsupported AirPlay HTTP request after the record is emitted. Mirroring, audio, pairing, TV UI and Starfish/ACB playback paths are unchanged.

The macOS build verifies the Git blob hashes of the pinned `raop.c`, `http_request.c` and `httpd.c`, then generates development-only copies. A changed upstream file stops the build. The probe records only fixed-vocabulary method, path, content-type, protocol and category labels; whether a session header was present; and a clamped numeric body length. It never formats raw URLs, query strings, headers, media bytes or session IDs. `result=0` means the unsupported request was ignored **without an HTTP response**, not HTTP status 200. `result=body_limit` means the development parser refused a body over 32 MiB before more bytes were allocated; its method/path are deliberately unknown. Displayed `body_bytes` is capped at 16 MiB with `body_capped=1`. Output is limited to 192 records plus one suppression marker per process. The development HTTP parser's existing raw-buffer error log and raw URL debug log are redacted as an additional precaution.

Example marker (fixed labels only):

```text
[direct-probe] method=POST path=play content_type=binary-plist body_bytes=4137 body_capped=0 protocol=HTTP/1.1 session_header=1 category=video result=0
```

Other path and type values are `other` rather than arbitrary path/header text. A line with `path=other` does **not** identify the exact route; it means a second narrowly scoped probe may be needed after reviewing the first capture. A missing marker does not prove the Photos app made no AirPlay request: discovery, pairing, transport or log rotation may have stopped the request before this gate. The current app's general `uxplay.log` can still contain older AirPlay and TV diagnostics; share **only extracted `[direct-probe]` lines**, not the full log or pairing files.

## LG B7 capture procedure

1. Download the successful `uxplay-webos-armv7-dev-<commit>` artifact and verify `SHA256SUMS` and `ELF_REPORT.txt` as in [development IPK instructions](DEVELOPMENT-IPK.md). Install the `.dev` IPK manually with webOS Dev Manager. Do not uninstall the known-good app; close it before launching **UxPlay webOS Dev**.
2. Keep the iPhone and TV on the same network. If the Photos item is stored in iCloud, download it to the iPhone first. Record only the iOS major version and intended test category outside the app log. Use **Photos → Share → AirPlay** or the Photos video's AirPlay control. **Do not start Screen Mirroring** for these four captures.
3. Run one case per fresh launch of UxPlay webOS Dev: (a) ordinary JPEG/HEIC photo, (b) source photograph known to be 4K or larger, (c) SDR video, and (d) source video known to be 4K HDR. Wait for the share attempt to finish or fail, then extract the **current** log before testing the next case. Do not infer the transmitted codec or resolution solely from the iPhone's library metadata.
4. On a Bash-capable host with SSH access to the rooted TV, set `TV_HOST` and `TV_KEY`, then run `tools/tv.sh direct-probe-logs`. It writes only current-run probe lines to `build/logs/direct-probe.log`. Copy that file to a separately named local result for each case before restarting the app. The helper does not modify TV files.
5. On Windows PowerShell with OpenSSH, the equivalent extraction is:

   ```powershell
   $devLog = '/var/palm/jail/com.github.danlflh.uxplaywebos.dev/media/developer/apps/usr/palm/applications/com.github.danlflh.uxplaywebos.dev/uxplay.log'
   ssh -i $env:TV_KEY -o StrictHostKeyChecking=yes "root@$($env:TV_HOST)" "grep '^\[direct-probe\]' '$devLog' || true" | Set-Content -Encoding utf8 .\direct-probe-photo.txt
   ```

   Change the local output filename for each case. Keep the extracted files local until you have checked them. Never share raw `uxplay.log`, photo/video files, URLs or pairing data.
6. After the four direct-media attempts, separately confirm that ordinary H.264 1080p/30 Screen Mirroring, audio, pairing and remote controls still work. If the diagnostic build misbehaves, close it and launch the known-good app. No CI step deploys to the TV.

For each case, report whether the sender showed the development receiver, whether the share attempt failed or fell back to mirroring, and the extracted marker lines. The receiver still cannot play direct media in this phase. Review those results before implementing a photo or video path.
