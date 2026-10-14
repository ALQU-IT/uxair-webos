#include "diagnostics.h"
#include <cassert>

int main() {
    VideoTiming timing;
    timing.record(1000000000ULL);
    timing.record(1033333333ULL);
    timing.record(1066666666ULL);
    timing.record(1100000000ULL);
    VideoTimingSample sample=timing.take_window();
    assert(sample.frames==4);
    assert(sample.intervals==3);
    assert(sample.interval_total_ns==100000000ULL);
    assert(sample.interval_min_ns==33333333ULL);
    assert(sample.interval_max_ns==33333334ULL);

    // Backwards timestamps and large discontinuities do not pollute cadence.
    timing.record(2000000000ULL);
    timing.record(1990000000ULL);
    timing.record(9000000000ULL);
    sample=timing.take_window();
    assert(sample.frames==3);
    assert(sample.intervals==0);
    assert(sample.interval_total_ns==0);
    assert(timing.take_window().frames==0);

    timing.reset();
    timing.record(100ULL);
    sample=timing.take_window();
    assert(sample.frames==1&&sample.intervals==0);
}
