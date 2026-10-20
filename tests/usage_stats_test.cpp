#include "usage_stats.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

static SessionSummary session(uint64_t start,uint64_t duration,SessionKind kind=SessionKind::Mirror) {
    SessionSummary s;s.started_utc=start;s.duration_ms=duration;s.kind=kind;s.width=kind==SessionKind::Audio?0:1920;s.height=kind==SessionKind::Audio?0:1080;s.codec=kind==SessionKind::Audio?0:2;return s;
}
int main() {
    char dir[]="/tmp/uxair-usage-test-XXXXXX";assert(mkdtemp(dir));
    const std::string root(dir),path=root+"/usage-v1";
    UsageStats empty;assert(!load_usage_stats(path,empty)&&!empty.storage_corrupt);
    assert(summarize_usage(empty,UsagePeriod::Days7,20000).bars.size()==7);

    // A session crossing UTC new year contributes to both daily buckets.
    UsageStats cross;cross.daily_from_day=usage_utc_day(1704067190);
    cross.record(session(1704067190,20000));
    assert(cross.daily.size()==2);
    assert(cross.daily[0].utc_day+1==cross.daily[1].utc_day);
    assert(cross.daily[0].mirror_ms==10000&&cross.daily[1].mirror_ms==10000);
    assert(cross.daily[0].mirror_sessions==1&&cross.daily[1].mirror_sessions==1);

    // Recent history is bounded at 100 while lifetime counters continue.
    UsageStats many;many.daily_from_day=usage_utc_day(1700000000);
    for(unsigned i=0;i<105;++i)many.record(session(1700000000+i*86400,60000,i%3?SessionKind::Mirror:SessionKind::Audio));
    assert(many.successful_sessions==105&&many.recent.size()==100);
    assert(many.recent.front().started_utc==1700000000+104*86400);
    assert(many.recent.back().started_utc==1700000000+5*86400);
    assert(many.daily.size()<=UsageStats::MaxDailyRecords);
    assert(save_usage_stats(path,many));
    assert(usage_storage_bytes(path)>0&&usage_storage_bytes(path)<200*1024);
    UsageStats restored;assert(load_usage_stats(path,restored));
    assert(restored.successful_sessions==105&&restored.recent.size()==100);
    UsageSummary week=summarize_usage(restored,UsagePeriod::Days7,usage_utc_day(1700000000+104*86400));
    assert(week.bars.size()==7&&week.mirror_ms+week.audio_ms>0);
    UsageSummary thirty=summarize_usage(restored,UsagePeriod::Days30,usage_utc_day(1700000000+104*86400));
    assert(thirty.bars.size()==30);
    UsageSummary months=summarize_usage(restored,UsagePeriod::Months12,usage_utc_day(1700000000+104*86400));
    assert(months.bars.size()==12);
    uint32_t leap_day=usage_utc_day(1709164800); // 2024-02-29 UTC
    UsageStats leap;leap.daily_from_day=leap_day;
    DailyUsage leap_record;leap_record.utc_day=leap_day;leap_record.mirror_ms=86400000;leap.daily.push_back(leap_record);
    UsageSummary leap_month=summarize_usage(leap,UsagePeriod::Months12,usage_utc_day(1709251200)); // 2024-03-01
    bool found_february=false;for(const auto& bar:leap_month.bars)if(bar.label=="Feb"){found_february=true;assert(bar.available&&bar.mirror_ms==86400000);}
    assert(found_february);
    UsageSummary leap_week=summarize_usage(leap,UsagePeriod::Days7,usage_utc_day(1709251200));
    assert(leap_week.bars.back().available&&leap_week.bars.back().mirror_ms==0); // zero-use current day is distinct from unavailable dates

    // V1 lifetime totals and recent entries migrate without inventing chart data.
    {std::ofstream v1(path.c_str());v1<<"UXPLAY_USAGE_V1\n"<<"totals 9000 2 1 1 0 90 80 1 2 3 4\n"
      <<"session 1700000000 5000 1 1280 720 1 30 29 0 0 0 0\n"
      <<"session 1700000100 4000 2 0 0 0 0 0 0 0 0 0\n";}
    UsageStats migrated;assert(load_usage_stats(path,migrated));
    assert(migrated.successful_sessions==2&&migrated.playback_ms==9000&&migrated.recent.size()==2);
    assert(migrated.daily.empty()&&migrated.daily_from_day==usage_utc_day((uint64_t)time(nullptr)));
    migrated.recent.push_front(session(1700000000,1000));migrated.recent.front().width=2160;migrated.recent.front().height=3840;
    assert(save_usage_stats(path,migrated));UsageStats portrait;assert(load_usage_stats(path,portrait));
    assert(portrait.recent.front().width==2160&&portrait.recent.front().height==3840);
    assert(save_usage_stats(path,migrated));
    {std::ifstream v2(path.c_str());std::string header;std::getline(v2,header);assert(header=="UXAIR_USAGE_V2");}

    // Invalid and oversized files are bounded, preserved, and require explicit reset.
    {std::ofstream bad(path.c_str(),std::ios::binary);bad<<"UXAIR_USAGE_V2\ntotals 1";}
    UsageStats corrupt;assert(!load_usage_stats(path,corrupt)&&corrupt.storage_corrupt);
    assert(!save_usage_stats(path,corrupt));
    {std::ofstream big(path.c_str(),std::ios::binary);std::string bytes(UsageStats::MaxSerializedBytes+1,'x');big.write(bytes.data(),bytes.size());}
    UsageStats oversized;assert(!load_usage_stats(path,oversized)&&oversized.storage_corrupt);
    oversized.reset();assert(save_usage_stats(path,oversized));
    assert(usage_storage_bytes(path)<UsageStats::MaxSerializedBytes);

    // Reset does not mutate an in-memory active session; it may be recorded normally at finish.
    UsageStats during;during.record(session(1700000000,60000));during.reset();
    assert(during.successful_sessions==0&&during.recent.empty());
    during.record(session(1700000060,30000));assert(during.successful_sessions==1&&during.recent.size()==1);
    remove(path.c_str());rmdir(root.c_str());
    std::puts("PASS: bounded usage history, UTC aggregation, V1 migration, periods, corruption and reset");
}
