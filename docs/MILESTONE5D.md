# Milestone 5D: bounded 1080p HEVC follow-up probe

The subsequent [Milestone 5E UHD experiment](MILESTONE5E.md) documents the 0.1.2 current-launch 1080p gate and remote UHD procedure. The instructions below describe probe 0.1.1.

## What the B7 run established

The 0.1.0 probe accepted four of 30 HEVC Main 8-bit access units. The owner's full log has no fifth Feed attempt, Feed rejection, retry exhaustion, timeout, or Starfish error. It does have `acb-sink=0 loaded-state-raw=1`, an accepted Play call and a later Starfish state event. The probe set `events.failed=true` solely because `AcbAPI_setSinkType` returned zero. Its `for` condition then silently stopped at the next frame. The working H.264 receiver calls the same ACB functions but does not treat this undocumented sink return as a fatal Feed condition. Thus the **4/30 stop was application logic**, not evidence of an HEVC Feed rejection or a decoder limit. Zero retries in the old result means no recognized `BufferFull` or `Pending` response was observed for those four accepted calls.

The new probe records raw ACB returns without treating the sink return alone as proof of success or failure. It logs `feed-begin`, `feed-stop reason=...`, the next frame, event times, signal number, state flags, and fixed-size Feed reply classifications. EOS is sent only after every access unit was accepted and no error or stop signal occurred. A partial run drains briefly, then unloads and finalizes. The 45-second hard watchdog, 10 retries per access unit, file/access-unit/frame bounds and private sample files remain unchanged. A 1080p result can report `control-path=COMPLETE`; `decoder-path` remains **UNPROVEN** without independent decoded/displayed-picture evidence. UHD is unconditionally blocked in this build, including when an old `1080p-decoder-pass.flag` exists.

The HEVC sample is Annex B with AUD-delimited access units and VPS/SPS/PPS before VCL. This validates the local file's structure but does not prove the Rockhopper HEVC BUFFERSTREAM contract. The reported GStreamer caps/structure assertions happened during initialization, before the first Feed. The same class of warnings was observed in working H.264 mirroring with the analogous Load options. No application GStreamer API call produces those warnings directly; a firmware Starfish/ACB internal path is the leading hypothesis, but the precise call and whether it matters for HEVC decoding are unverified. The probe does not suppress them or change the `H265`/`RAW` Load settings without evidence.

`FRAMEREADY` callbacks are throttled on this firmware in the working H.264 receiver and are neither per-frame output timing nor proof of panel presentation. Their absence alone does not identify a decoder failure. Starfish/ACB video-info at 1920×1080 reports metadata, not displayed pixels.

## Manual update on a rooted B7, without webOS CLI

Download the separate `hevc-probe-armv7-<commit>` GitHub Actions artifact, extract it on Windows, and check its `SHA256SUMS`. Keep the old 0.1.0 IPK for rollback. Close any mirroring session and ensure `pidof hevc-probe` on the TV returns no process. From Windows PowerShell, copy **only** the new probe IPK to the rooted TV; replace the host and key placeholders:

```powershell
scp -i C:\path\to\tv-key .\com.github.danlflh.uxplaywebos.hevcprobe_0.1.1_arm.ipk root@TV_IP:/tmp/
ssh -i C:\path\to\tv-key root@TV_IP
```

On the TV's SSH shell, install the package from that local file and wait for the install service's completion response. The `dev/install` Luna method and `ipkUrl` syntax are documented by [webOS appInstallService](https://www.webosose.org/docs/reference/ls2-api/com-webos-appinstallservice/) and the [webOS Homebrew command reference](https://www.webosbrew.org/pages/commands-cheatsheet); method availability on this exact Rockhopper build should be confirmed from its response:

```sh
APP=com.github.danlflh.uxplaywebos.hevcprobe
luna-send -i -f luna://com.webos.appInstallService/dev/install '{"id":"com.github.danlflh.uxplaywebos.hevcprobe","ipkUrl":"/tmp/com.github.danlflh.uxplaywebos.hevcprobe_0.1.1_arm.ipk","subscribe":true}'
```

Run **1080p only** through Application Manager. A `returnValue:true` response indicates the launch request was accepted; inspect the log for the actual probe result:

```sh
luna-send -n 1 -f luna://com.webos.applicationManager/launch '{"id":"com.github.danlflh.uxplaywebos.hevcprobe","params":{"mode":"1080p"}}'
```

After the process exits, collect the probe-only log from the path that exists on this B7. The in-jail installation path is inferred from the working receiver; the startup marker `log=app` or `log=tmp` distinguishes the writable target:

```sh
APP=com.github.danlflh.uxplaywebos.hevcprobe
for FILE in \
  "/var/palm/jail/$APP/media/developer/apps/usr/palm/applications/$APP/hevc-probe.log" \
  "/media/developer/apps/usr/palm/applications/$APP/hevc-probe.log" \
  "/media/cryptofs/apps/usr/palm/applications/$APP/hevc-probe.log" \
  "/var/palm/jail/$APP/tmp/uxplay-hevc-probe.log" \
  /tmp/uxplay-hevc-probe.log; do
  if test -f "$FILE"; then echo "LOG=$FILE"; tail -n 150 "$FILE"; fi
done
```

Please preserve lines from `startup` through `cleanup`, especially `acb-sink-raw`, `feed-stop`, `result`, `control-path`, and GStreamer warnings. Note whether the process was launched by Luna and whether any TV picture was observed; no visual observation is required to report control-path results. Do **not** run `mode=uhd`. If the new package misbehaves, let the process finish or stop it with `kill -TERM <probe-PID>` (find the exact PID with `pidof hevc-probe`), then reinstall the saved 0.1.0 IPK by the same local install method. Do not remove or modify the working UxPlay Dev application, its pairing data, firmware libraries or media configuration. If ACB remains occupied after a forced stop, restart `AcbService` or reboot before other playback.
