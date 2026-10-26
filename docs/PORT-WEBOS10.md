# webOS 5+ / webOS 10 support

UxAir 1.1.0 plays through `StarfishMediaAPIs` + `AcbAPI`. Neither library
exists on webOS 10 (LG "webOS 25"), so the app does not start there. webOS 5
and later have LG's **NDL direct media** API (`libNDL_directmedia.so.1`, API
version 2, as Moonlight TV uses). This port adds a second player on that API
and keeps upstream's Starfish player for webOS 3-4.

Tested on an LG OLED65C48LA (OLED evo C4, 2024, rooted webOS 10.3.1) with
iPhone and iPad: mirroring
(HEVC up to 4K, H.264), photos and videos while mirroring, audio-only playback.
The webOS 3-4 build compiles but has not been tested on a TV.

## Two build variants

| | `tools/build.sh` (default) | `tools/build.sh --media=starfish` |
|---|---|---|
| For | webOS 5 and later | webOS 3-4 (upstream's verified LG B7, webOS 3.9) |
| Player | NDL direct media: `src/media_ndl.cpp`, `src/ndl_backend.c` | Starfish + ACB: `src/media_starfish.cpp` (upstream's, unchanged) |
| Timing | anchored to frame arrival, at most 50 ms hold | upstream's 200 ms PTS pre-roll |
| Package | `dist/<id>_<version>_arm-webos5plus.ipk` | `dist/<id>_<version>_arm-webos3-4.ipk` |

CMake option `UXAIR_MEDIA=ndl|starfish`. Both share the additions below.
SDL2_image (cover art) is opened at run time, so a TV without it shows no
cover instead of failing to start; the system bus code uses `LSGmainAttach`,
which old luna-service2 versions have as well.

## NDL on webOS 10: what the player needs

Found with `src/ndl_probe.c` (a small app that plays a test stream; package
it with `tools/package-ndl-probe.py <file.h264>`):

1. The player registers on the luna bus, so it only works inside an installed
   app (from a shell: `LSRegisterPubPriv FAILED`).
2. The video is bound to an SDL exported window:
   `SDL_webOSCreateExportedWindow(0)` -> `NDL_DirectMediaSetWindowId(id)` ->
   `SDL_webOSSetExportedWindow(id, src, dst)`. `NDL_DirectMediaSetWindowId` is
   exported by the firmware but not by the SDK's stub library, so it is looked
   up with `dlsym`.
3. NDL is initialised and bound to the window once per process; a session
   only loads and unloads a stream. `Quit` + `Init` between sessions loses the
   binding ("No matching WebOSExported", load event 0x12 "fail to create
   AVProxy"), e.g. when the sender switches to a full-screen photo.
4. Playback only advances while audio is fed as well; the main loop already
   feeds silence when there is none.
5. NDL renders as data arrives (ns and us timestamps behave the same), so
   UxAir's scheduling is the only buffer: it is anchored to the first frame's
   arrival and holds no frame longer than 50 ms.
6. Load-callback events use Starfish's `PF_EVENT_TYPE_*` codes (0x16 load
   completed, 0x1a playing, 0x17 unload completed); `LOADCOMPLETED` only
   arrives once data flows, so a successful `Load` counts as ready.
7. Audio-only sessions: `NDL_DirectMediaLoad` with no video type, PCM only.
8. LG's NDL headers only compile as C; `src/ndl_backend.c` wraps them for the
   C++ code.

## Additions (both variants)

- **Data folder:** a jailed native app on webOS 10 runs as its own user over a
  root-owned app folder; log, pairing keys and settings go to
  `/media/developer/apps/var/lib/<app-id>` when the app folder is not writable
  (`data_directory()`).
- **Now Playing screen** for audio-only sessions (`Shell::now_playing`): cover
  art, title (scrolls when too long), artist, album, sender name and progress
  from AirPlay metadata (`md=0,1,2`); the background takes the cover's colour.
  For OLED panels it dims to 35% after 20 s without a key (a new track shows
  at full brightness for 8 s) and the layout drifts slowly.
- **System bus** (`src/system_bus.cpp`):
  - LG's screen saver is postponed while a session plays (tvpower
    `registerScreenSaverRequest` / `responseScreenSaverRequest`).
  - UxAir keeps running in the background and stays discoverable. A device
    that starts streaming (or needs the PIN) brings it to the front; when the
    device leaves, the app that was in front before comes back. Back/Exit send
    UxAir to the background instead of quitting.
  - Launch parameters `{"background":true,"return_to":"<app-id>"}` start it
    hidden, e.g. from a boot script.

## Building on Linux

`tools/bootstrap.py` picks the toolchain for the host: the pinned macOS ARM64
SDK, or on Linux x86-64 the equivalent openlgtv/buildroot-nc4 `webos-d7ed7ee`
SDK (`toolchain-linux-x86_64` in `dependencies.lock.json`). Without the webOS
CLI, `tools/package.py` writes the IPK with `tools/make-ipk.py` (same layout
as `ares-package`).
