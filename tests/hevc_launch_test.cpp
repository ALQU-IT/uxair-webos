#include "hevc_launch.h"
#include <cassert>
#include <string>

using hevc_probe::LaunchMode;
using hevc_probe::LaunchStatus;

static void expect(const char *argument, LaunchMode mode, LaunchStatus status) {
    const hevc_probe::LaunchDecision parsed=hevc_probe::parse_launch_argument(argument);
    assert(parsed.mode==mode);
    assert(parsed.status==status);
}

int main() {
    expect(nullptr,LaunchMode::Idle,LaunchStatus::Missing);
    expect("",LaunchMode::Idle,LaunchStatus::NotJson);
    expect("{\"mode\":\"1080p\"}",LaunchMode::Hd1080,LaunchStatus::Valid);
    expect(" { \"@system_native_app\":true, \"nid\":\"probe\", \"mode\":\"1080p\" } ",
           LaunchMode::Hd1080,LaunchStatus::Valid);
    expect("{\"params\":{\"mode\":\"1080p\"},\"@system_native_app\":true}",
           LaunchMode::Hd1080,LaunchStatus::Valid);
    expect("{\"mode\":\"uhd\",\"nid\":\"probe\"}",LaunchMode::Uhd,LaunchStatus::Valid);
    expect("{\"params\":{\"mode\":\"uhd\"}}",LaunchMode::Uhd,LaunchStatus::Valid);
    expect("{\"nid\":\"probe\"}",LaunchMode::Idle,LaunchStatus::NoMode);
    expect("{\"nested\":{\"mode\":\"1080p\"}}",LaunchMode::Idle,LaunchStatus::NoMode);
    expect("{\"text\":\"\\\"mode\\\":\\\"1080p\\\"\"}",LaunchMode::Idle,LaunchStatus::NoMode);
    expect("{\"mode\":\"720p\"}",LaunchMode::Idle,LaunchStatus::Unsupported);
    expect("{\"mode\":\"1080p\",\"mode\":\"uhd\"}",LaunchMode::Idle,LaunchStatus::Duplicate);
    expect("{\"mode\":\"1080p\",\"params\":{\"mode\":\"uhd\"}}",
           LaunchMode::Idle,LaunchStatus::Duplicate);
    expect("{\"mode\":\"1080p\"",LaunchMode::Idle,LaunchStatus::Malformed);
    expect("{\"mode\":\"1080p\",\"x\":bad}",LaunchMode::Idle,LaunchStatus::Malformed);
    expect("{\"params\":{\"mode\":\"1080p\"},\"x\":[0,false,null,{\"a\":1.5e2}]}",
           LaunchMode::Hd1080,LaunchStatus::Valid);
    std::string oversized(2049,'x');
    expect(oversized.c_str(),LaunchMode::Idle,LaunchStatus::Oversized);
}
