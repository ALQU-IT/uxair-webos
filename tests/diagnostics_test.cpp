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
    assert(sample.pts_gaps_over_100ms==0);
    assert(sample.pts_discontinuities==0);

    // Backwards timestamps and large discontinuities do not pollute cadence.
    timing.record(4000000000ULL);
    timing.record(3990000000ULL);
    timing.record(9000000000ULL);
    sample=timing.take_window();
    assert(sample.frames==3);
    assert(sample.intervals==0);
    assert(sample.interval_total_ns==0);
    assert(sample.pts_discontinuities==3);
    assert(timing.take_window().frames==0);

    timing.reset();
    timing.record(100ULL);
    sample=timing.take_window();
    assert(sample.frames==1&&sample.intervals==0);

    // Arrival and PTS gaps can disagree; the paired counters preserve that evidence.
    timing.reset();
    timing.record(1000000000ULL, 1000000000ULL);
    timing.record(1033333333ULL, 1033333333ULL);
    timing.record(1233333333ULL, 1233333333ULL); // both gaps
    timing.record(1266666666ULL, 1433333333ULL); // arrival only
    timing.record(1466666666ULL, 1466666666ULL); // PTS only
    sample=timing.take_window();
    assert(sample.frames==5 && sample.arrival_intervals==4);
    assert(sample.pts_gaps_over_100ms==2);
    assert(sample.arrival_gaps_over_100ms==2);
    assert(sample.gaps_in_both==1);
    assert(sample.arrival_only_gaps==1);
    assert(sample.pts_only_gaps==1);
    assert(sample.arrival_interval_max_ns==200000000ULL);
    timing.record(1500000000ULL, 1500000000ULL);
    assert(timing.take_window().arrival_intervals==1); // window boundary retains history

    DurationWindow durations;
    durations.record(1000000ULL);
    durations.record(60000000ULL);
    DurationSample d=durations.take_window();
    assert(d.count==2 && d.total_ns==61000000ULL && d.max_ns==60000000ULL);
    assert(d.over_50ms==1 && d.average_ms()==30.5 && d.maximum_ms()==60.0);
    assert(durations.take_window().count==0);
    durations.record(70000000ULL);
    durations.reset();
    assert(durations.take_window().count==0);
}
