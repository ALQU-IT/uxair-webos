# ARMv7 development package

The `Build ARMv7 development IPK` GitHub Actions workflow runs on the standard Apple Silicon `macos-15` runner when `feature/b7-optimization` is pushed or manually dispatched. Its own Ubuntu native-test job must pass before the macOS build job starts. It uses the checksum-pinned Darwin ARM64 webOS SDK in `dependencies.lock.json`, builds the pinned private libraries and receiver, checks the ELF ABI, validates the IPK contents, and uploads the package only if every step passes. Host FFmpeg tools are not needed for this target build: `tools/build-deps.sh` cross-compiles the pinned FFmpeg source. The separate Ubuntu workflow remains unchanged.

The artifact contains:

- `com.github.danlflh.uxplaywebos.dev_<version>_arm.ipk` — a separate development app.
- `ELF_REPORT.txt` — ARM ELF32 soft-float and maximum required glibc version reported by `tools/verify-elf.py`.
- `SHA256SUMS` — the SHA-256 digest of the IPK itself (the Actions artifact archive has its own, different digest).
- `BUILD_COMMIT.txt` — the exact Git commit checked out by the build job.

The development app uses app ID `com.github.danlflh.uxplaywebos.dev` and AirPlay name **UxPlay webOS Dev**. Its executable falls back to that app ID for ACB/Starfish if the launcher does not provide `APPID`. Its pairing keys, receiver identity, settings, and logs live beside its own executable in the development app directory. The existing `com.github.danlflh.uxplaywebos` installation and its state are separate. Run only one receiver at a time so two apps do not compete for the TV's media hardware.

## Download and check the artifact

1. Open the successful build run on the feature branch and download its `uxplay-webos-armv7-dev-<commit>` artifact. Extract the downloaded ZIP to a local folder.
2. Check that `BUILD_COMMIT.txt` matches the run's source commit. Verify the IPK against `SHA256SUMS` before installation:

   ```sh
   shasum -a 256 -c SHA256SUMS
   ```

   On Windows PowerShell, run `Get-FileHash -Algorithm SHA256 .\com.github.danlflh.uxplaywebos.dev_*.ipk` and compare its digest with `SHA256SUMS`.

3. Read `ELF_REPORT.txt`. It must report ARM ELF32 soft-float and a maximum required `GLIBC` version no newer than 2.19.

## Install and roll back with webOS Dev Manager

These are manual steps for the TV owner. No CI job connects to a TV.

1. In webOS Dev Manager, connect to the intended TV. Close the existing **UxPlay webOS** app before starting the development build.
2. In the Apps view, choose **Install** and select the verified `.dev_..._arm.ipk`. Confirm the new **UxPlay webOS Dev** launcher entry appears. Launch only that entry and select **UxPlay webOS Dev** from the sender's Screen Mirroring list. Pair it separately; the existing installation's pairing keys are not copied.
3. If it fails to launch or play correctly, close **UxPlay webOS Dev** in Dev Manager, uninstall only `com.github.danlflh.uxplaywebos.dev`, and relaunch the original **UxPlay webOS** app. This leaves the original app ID and its pairing state in place. Keep a copy of the known-good production IPK as an additional recovery option; do not uninstall the original app while testing.

The owner has tested a previous development package on the LG OLED B7 with Rockhopper 3.9.0, confirming 1080p/30 H.264 mirroring with actual 1920×1080 SPS, no reported drops, Magic Remote OK and the playback overlay. Each newly built UI package still needs the [Milestone 4 TV procedure](MILESTONE4.md); a passing build and ELF check do not establish its runtime behavior.

The development package has a temporary, metadata-only [direct-media request probe](MILESTONE4C.md). The [Milestone 4D handshake experiment](MILESTONE4D.md) also answers a narrowly scoped HTTP `/info` request without enabling HLS or direct playback. Follow its four-case capture and rollback procedure, and share only extracted probe markers.
