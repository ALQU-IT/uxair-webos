#pragma once

// The development package has its own launcher identity, AirPlay name, and
// app-local state directory. Keep the ACB fallback ID in step with appinfo.json.
#ifdef UXPLAY_DEV_BUILD
#define UXPLAY_APP_ID "com.github.danlflh.uxplaywebos.dev"
#define UXPLAY_DISPLAY_NAME "UxPlay webOS Dev"
#else
#define UXPLAY_APP_ID "com.github.danlflh.uxplaywebos"
#define UXPLAY_DISPLAY_NAME "UxPlay webOS"
#endif
