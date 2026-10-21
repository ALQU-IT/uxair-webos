#pragma once
// Host-only stand-ins for input constants; renderer tests do not call Shell::open.
#define SDL_WEBOS_SCANCODE_BACK SDL_SCANCODE_ESCAPE
#define SDL_WEBOS_SCANCODE_EXIT SDL_SCANCODE_F12
#define SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK "test.webos.back"
#define SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_EXIT "test.webos.exit"
