# UxAir 1.0.0 — UI and repository preparation candidate

This candidate starts from the owner hardware-verified RC5 playback baseline. It changes launcher artwork, app tile color metadata, branding documentation, and packaging validation; it does not change the media, pairing, RAOP, ACB, Starfish, H.264/HEVC, frame-rate, or audio implementation.

## UI and repository changes

- The launcher artwork uses a full-bleed `#1D2D47` background with the existing centered light AirPlay-style mark. Small and large webOS icons and repository previews are generated from the SVG master.
- The app manifest specifies the matching tile color and concise subtitle `AirPlay Receiver for webOS`.
- The About screen credits the original UxPlay core and webOS port by their project names and handles; developer identity remains `@depius`.
- The README includes tested hardware, feature limits, installation steps, credits, licensing, optional support, and a placeholder for screenshots that the owner will provide.
- The repository description, topics, and rename instructions are prepared. No repository settings, release, or tag were changed.

## Validation scope

The ARMv7 IPK must pass host tests, metadata validation, ELF/IPK verification, and source archive generation in GitHub Actions. The final launcher appearance and `iconColor` support require review on the LG OLED B7 running webOS 3.9.
