#include "hevc_gate.h"
#include <cassert>

int main() {
    hevc_probe::CurrentLaunchUhdGate gate;
    assert(!gate.permits_uhd());
    gate.record_1080p_control_result(1,false);
    assert(!gate.permits_uhd());
    gate.record_1080p_control_result(0,true);
    assert(!gate.permits_uhd());
    gate.record_1080p_control_result(0,false);
    assert(gate.permits_uhd());
    gate.record_1080p_control_result(2,false);
    assert(!gate.permits_uhd());
}
