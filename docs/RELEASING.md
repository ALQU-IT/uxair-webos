# UxAir release preparation

## Release candidate checks

1. Start from the reviewed UxAir release-preparation branch and record the source commit. Do not rewrite Git history or publish a release as part of candidate preparation.
2. Keep version `1.0.0` in `packaging/appinfo.json` for the RC. The package identity is `com.github.depius.uxair`; the former UxPlay development app has a different ID and is kept installed as the rollback option.
3. Run `python3 tools/validate-metadata.py`, `tools/test.sh`, `tools/build.sh`, `python3 tools/verify-elf.py build/app`, `python3 tools/validate-ipk.py`, and `tools/source-package.py`.
4. Inspect the IPK and source archive for private keys, logs, sender data, pairing/settings files, and local paths. Confirm the IPK is `com.github.depius.uxair_1.0.0_arm.ipk` and the source archive is `uxair-webos-1.0.0-source.tar.gz`.
5. The Actions RC workflow runs host tests, builds the ARMv7 IPK, validates the staged ELF, package identity and bundled donation QR, generates corresponding sources and SHA-256 checksums, then uploads them as a candidate artifact.
6. Validate on a rooted LG OLED B7 running webOS 3.9. Standalone AirPlay audio is still awaiting verification. Also recheck H.264 720p/60 and 1080p/60, HEVC 4K/60, 30 FPS modes, rotation, mirroring audio, Magic Remote navigation, large PIN pairing, donation QR readability, settings, and reconnect/recovery.

## Identity and user data

UxAir uses the new app ID `com.github.depius.uxair`. Installing it does not overwrite or uninstall the UxPlay development app (`com.github.danlflh.uxplaywebos.dev`). App-private pairing keys, receiver identity, and settings are distinct; the user pairs and configures UxAir separately. No data is migrated or deleted automatically. Keep the old app until the candidate is accepted.

## Final publication

Do not create a final release tag, rename the GitHub repository, change GitHub repository settings, or publish a GitHub Release until the owner has reviewed the artifact and explicitly approved final publication. The RC branch is not the public release branch. Final release notes must retain the audio validation status and the limitations about HDR/Main10, displayed FPS, native 4K output, and unverified TV models. Attach the IPK, corresponding-source archive, and `SHA256SUMS` only after that approval.

This repository does not create a webOS Brew catalog manifest or submission.
