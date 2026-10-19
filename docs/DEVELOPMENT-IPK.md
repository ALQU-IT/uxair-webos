# UxAir 1.0.0 release-candidate package

The `Build UxAir 1.0.0 Release Candidate ARMv7` workflow runs the sanitizer-backed host tests before it builds the ARMv7 application on Apple Silicon macOS. It validates the ELF ABI and IPK contents, including both launcher icons and the offline donation QR, generates corresponding source and checksums, then uploads one artifact. The artifact is not a GitHub Release and does not publish a tag.

The artifact contains:

- `com.github.depius.uxair_1.0.0_arm.ipk` — UxAir under its separate app ID.
- `bmc-qr.bmp` — 407×407 local QR bitmap encoded for `https://buymeacoffee.com/depius`, also bundled inside the IPK.
- `uxair-webos-1.0.0-source.tar.gz` — corresponding sources, pinned UxPlay source, dependency source archives, notices, and build instructions.
- `ELF_REPORT.txt` — ARM ELF32 soft-float and maximum required GLIBC version from `tools/verify-elf.py`.
- `SHA256SUMS` — checksums for the IPK and source archive.
- `BUILD_COMMIT.txt` — the exact source commit used for the build.

## Download and verify

1. Download the `uxair-1.0.0-release-candidate-armv7-<commit>` artifact from the successful GitHub Actions run and extract it.
2. Confirm `BUILD_COMMIT.txt` matches the run's source commit. Verify both files:

   ```sh
   shasum -a 256 -c SHA256SUMS
   ```

3. Confirm the ELF report lists ARM ELF32 soft-float ABI and GLIBC requirements no newer than 2.19.

## Install and preserve rollback

Install the IPK on a compatible rooted TV with webOS native development access. UxAir uses `com.github.depius.uxair`; it installs separately from `com.github.danlflh.uxplaywebos.dev` and does not uninstall or overwrite the existing UxPlay app. UxAir starts with independent pairing, receiver identity, and settings. Pair it again and configure it separately.

For rollback, close UxAir and launch the retained UxPlay webOS development app. Do not uninstall the old app until UxAir has passed physical checks. CI does not connect to a TV or establish hardware behavior. The owner separately confirmed the tested standalone AirPlay music workflow on the B7; individual negotiated codec variants, native output resolution, and panel-presented FPS were not each measured.
