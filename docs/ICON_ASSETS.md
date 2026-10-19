# UxAir launcher artwork

`assets/uxair-icon.svg` is the vector master. Run `python3 tools/render-icons.py` with Pillow installed to regenerate all PNG sizes from that source:

- `packaging/icon.png` — required 80×80 small icon.
- `packaging/largeIcon.png` — 130×130 large icon.
- `assets/uxair-icon-256.png` and `assets/uxair-icon-512.png` — repository previews/source artwork.

Every asset is opaque to all four edges and uses the same `#1D2D47` slate-blue background. `packaging/appinfo.json` sets `iconColor` to the same value; LG's app metadata reference describes `iconColor` as the application tile background. It documents the 80×80 `icon` and optional 130×130 `largeIcon`. The older `bgColor` property applies only to webOS TV 1.x/2.x and is ignored on later versions. See [LG's appinfo.json reference](https://webostv.developer.lge.com/develop/references/appinfo-json) and [app resource guide](https://webostv.developer.lge.com/develop/getting-started/app-resources).

These documented settings are the closest supported application-level control. The B7/webOS 3.9 launcher must still be checked in person to confirm whether it honors `iconColor` or adds system decoration around native app icons. No firmware or launcher files are modified by UxAir.
