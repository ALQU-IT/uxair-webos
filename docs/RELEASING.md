# Releasing UxPlay webOS

## Before tagging

1. Start from a clean checkout on `main`; confirm no generated files are tracked.
2. Set the release version only in `packaging/appinfo.json`. The Git tag must be `v<version>` and the GitHub release title must be `UxPlay webOS v<version>`.
3. Update `CHANGELOG.md`. During `0.x`, use patch versions for fixes and minor versions for new capabilities. Reserve `1.0.0` for broader stability and compatibility.
4. Run `python3 tools/validate-metadata.py` and `tools/test.sh`.
5. Build from the clean checkout with `tools/build.sh`, then create corresponding source with `tools/source-package.py`.
6. Verify the staged ELF with `tools/verify-elf.py build/app`: ARM32 ABI, GLIBC <= 2.19, and the app-local `libstdc++.so.6` must resolve the required C++ symbols.
7. Inspect both archives for SSH keys, logs, personal IP addresses, and pairing data. Confirm the output names are `com.github.danlflh.uxplaywebos_<version>_arm.ipk` and `uxplay-webos-<version>-source.tar.gz`.
8. On the validated TV, install the new identity after removing any old development-ID build. Confirm English UI, discovery, PIN, video, audio, rotation, disconnect/reconnect, and relaunch. Record limitations that remain.

## Publish

1. Create a clean initial/release commit and an annotated `v<version>` tag only after all checks pass.
2. Push `main` and the tag to `github.com/danlflh/uxplay-webos`.
3. Create a normal GitHub release titled `UxPlay webOS v<version>` (not a pre-release), clearly describing it as experimental.
4. Attach the IPK, source archive, and a `SHA256SUMS` file generated from those two artifacts.
5. Use the matching changelog section as release notes and include validated hardware/firmware plus known limitations.

This project does not create a webOS Brew catalog manifest or submission in this release process.
