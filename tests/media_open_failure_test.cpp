#include "media_open_failure.h"
#include <cassert>
#include <cstring>

int main() {
    assert(std::strcmp(media_open_failure_message(MediaOpenFailure::None),
                       "Video playback could not start.")==0);
    assert(std::strcmp(media_open_failure_message(MediaOpenFailure::StarfishLoad),
                       "Video playback could not start (Starfish Load failed).") == 0);
    assert(std::strcmp(media_open_failure_message(MediaOpenFailure::AcbInitialize),
                       "Video playback could not start (ACB initialization failed).") == 0);
}
