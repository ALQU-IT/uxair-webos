#include "usage_stats.h"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

const char *session_kind_name(SessionKind k) {
    switch(k){case SessionKind::Audio:return "Audio";case SessionKind::Direct:return "Direct";default:return "Mirror";}
}
const char *usage_period_name(UsagePeriod p) {
    return p==UsagePeriod::Days30?"Last 30 days":p==UsagePeriod::Months12?"Last 12 months":"Last 7 days";
}
std::string usage_axis_label(uint64_t milliseconds) {
    char label[32];
    if(milliseconds<1000)snprintf(label,sizeof(label),"%llums",(unsigned long long)milliseconds);
    else if(milliseconds>=3600000)snprintf(label,sizeof(label),"%lluh",(unsigned long long)(milliseconds/3600000+(milliseconds%3600000>=1800000)));
    else if(milliseconds>=60000)snprintf(label,sizeof(label),"%llum",(unsigned long long)(milliseconds/60000+(milliseconds%60000>=30000)));
    else snprintf(label,sizeof(label),"%llus",(unsigned long long)(milliseconds/1000+(milliseconds%1000>=500)));
    return label;
}
unsigned usage_mirror_percent(uint64_t mirror_ms,uint64_t audio_ms) {
    if(!mirror_ms&&!audio_ms)return 0;
    const long double total=static_cast<long double>(mirror_ms)+static_cast<long double>(audio_ms);
    return static_cast<unsigned>(static_cast<long double>(mirror_ms)*100.0L/total+0.5L);
}
uint32_t usage_utc_day(uint64_t seconds) {
    uint64_t d=seconds/86400;return d>UINT32_MAX?UINT32_MAX:uint32_t(d);
}
static uint64_t sat_add(uint64_t a,uint64_t b) {
    return b>UINT64_MAX-a?UINT64_MAX:a+b;
}
static void daily_add(UsageStats& stats,const SessionSummary& s) {
    if(s.kind==SessionKind::Direct||!s.started_utc||!s.duration_ms)return;
    const uint64_t first_ms=s.started_utc*1000;
    if(first_ms/1000!=s.started_utc||UINT64_MAX-first_ms<s.duration_ms)return;
    uint64_t cursor=first_ms,end=first_ms+s.duration_ms;
    while(cursor<end) {
        uint32_t day=usage_utc_day(cursor/1000);
        uint64_t next=(uint64_t(day)+1)*86400000ULL;
        uint64_t part=std::min(end,next)-cursor;
        auto it=std::lower_bound(stats.daily.begin(),stats.daily.end(),day,
            [](const DailyUsage& a,uint32_t b){return a.utc_day<b;});
        if(it==stats.daily.end()||it->utc_day!=day){DailyUsage d;d.utc_day=day;it=stats.daily.insert(it,d);}
        if(s.kind==SessionKind::Audio){it->audio_ms=sat_add(it->audio_ms,part);if(it->audio_sessions<UINT32_MAX)++it->audio_sessions;}
        else {it->mirror_ms=sat_add(it->mirror_ms,part);if(it->mirror_sessions<UINT32_MAX)++it->mirror_sessions;}
        cursor+=part;
    }
    uint32_t last=stats.daily.empty()?usage_utc_day(s.started_utc):stats.daily.back().utc_day;
    uint32_t cutoff=last>=364?last-364:0;
    stats.daily.erase(std::remove_if(stats.daily.begin(),stats.daily.end(),[cutoff](const DailyUsage& d){return d.utc_day<cutoff;}),stats.daily.end());
    if(stats.daily.size()>UsageStats::MaxDailyRecords)
        stats.daily.erase(stats.daily.begin(),stats.daily.begin()+(stats.daily.size()-UsageStats::MaxDailyRecords));
    if(!stats.daily_from_day||stats.daily_from_day>usage_utc_day(s.started_utc))stats.daily_from_day=usage_utc_day(s.started_utc);
    if(stats.daily_from_day<cutoff)stats.daily_from_day=cutoff;
}
void UsageStats::record(const SessionSummary& s) {
    if(successful_sessions<UINT64_MAX)++successful_sessions;
    if(s.kind==SessionKind::Audio){if(audio_sessions<UINT64_MAX)++audio_sessions;}
    else if(s.kind==SessionKind::Direct){if(direct_sessions<UINT64_MAX)++direct_sessions;}
    else if(mirror_sessions<UINT64_MAX)++mirror_sessions;
    playback_ms=sat_add(playback_ms,s.duration_ms);incoming_frames=sat_add(incoming_frames,s.incoming_frames);
    feed_frames=sat_add(feed_frames,s.feed_frames);packet_drops=sat_add(packet_drops,s.packet_drops);
    video_frame_drops=sat_add(video_frame_drops,s.video_frame_drops);decoder_errors=sat_add(decoder_errors,s.decoder_errors);
    session_errors=sat_add(session_errors,s.session_errors);
    recent.push_front(s);while(recent.size()>MaxRecentSessions)recent.pop_back();daily_add(*this,s);
}
void UsageStats::reset(){*this=UsageStats();}
static bool read_summary(const std::string& line,SessionSummary& s) {
    std::istringstream in(line);std::string tag,extra;unsigned kind=0;
    if(!(in>>tag>>s.started_utc>>s.duration_ms>>kind>>s.width>>s.height>>s.codec>>s.incoming_frames>>s.feed_frames
        >>s.packet_drops>>s.video_frame_drops>>s.decoder_errors>>s.session_errors))return false;
    if((in>>extra)||tag!="session"||kind<1||kind>3||s.width>3840||s.height>3840||s.codec>2||
       s.duration_ms>365ULL*24*60*60*1000)return false;
    s.kind=SessionKind(kind);return true;
}
static bool read_totals(const std::string& line,UsageStats& s) {
    std::istringstream in(line);std::string tag,extra;
    if(!(in>>tag>>s.playback_ms>>s.successful_sessions>>s.mirror_sessions>>s.audio_sessions>>s.direct_sessions
        >>s.incoming_frames>>s.feed_frames>>s.packet_drops>>s.video_frame_drops>>s.decoder_errors>>s.session_errors)||
       (in>>extra)||tag!="totals")return false;
    if(s.mirror_sessions>s.successful_sessions||s.audio_sessions>s.successful_sessions-s.mirror_sessions)return false;
    return s.direct_sessions==s.successful_sessions-s.mirror_sessions-s.audio_sessions;
}
static bool read_bounded(const std::string& path,std::string& data,bool& exists) {
    exists=false;struct stat st;
    if(stat(path.c_str(),&st)!=0)return errno==ENOENT;
    exists=true;if(!S_ISREG(st.st_mode)||st.st_size<1||uint64_t(st.st_size)>UsageStats::MaxSerializedBytes)return false;
    FILE *f=fopen(path.c_str(),"rb");if(!f)return false;
    data.resize(size_t(st.st_size));size_t n=fread(&data[0],1,data.size(),f);bool close_ok=fclose(f)==0;
    return n==data.size()&&close_ok;
}
bool load_usage_stats(const std::string& path,UsageStats& stats) {
    std::string data;bool exists=false;
    if(!read_bounded(path,data,exists)){if(exists||errno!=ENOENT)stats.storage_corrupt=true;return false;}
    if(!exists)return false;
    std::istringstream in(data);std::string line;
    if(!std::getline(in,line)){stats.storage_corrupt=true;return false;}
    bool v1=line=="UXPLAY_USAGE_V1",v2=line=="UXAIR_USAGE_V2";
    if(!v1&&!v2){stats.storage_corrupt=true;return false;}
    UsageStats parsed;
    if(!std::getline(in,line)||line.size()>300||!read_totals(line,parsed)){stats.storage_corrupt=true;return false;}
    if(v1) {
        while(std::getline(in,line)){if(parsed.recent.size()>=16||line.size()>300){stats.storage_corrupt=true;return false;}
            SessionSummary s;if(!read_summary(line,s)){stats.storage_corrupt=true;return false;}parsed.recent.push_back(s);}
        parsed.daily_from_day=usage_utc_day(uint64_t(std::time(nullptr)));
    } else {
        std::string tag,extra;
        if(!std::getline(in,line)||line.size()>40){stats.storage_corrupt=true;return false;}
        std::istringstream meta(line);if(!(meta>>tag>>parsed.daily_from_day)||(meta>>extra)||tag!="coverage"){stats.storage_corrupt=true;return false;}
        if(!std::getline(in,line)||line.size()>20){stats.storage_corrupt=true;return false;}
        std::istringstream sc(line);size_t count=0;if(!(sc>>tag>>count)||(sc>>extra)||tag!="sessions"||count>UsageStats::MaxRecentSessions){stats.storage_corrupt=true;return false;}
        for(size_t i=0;i<count;++i){if(!std::getline(in,line)||line.size()>300){stats.storage_corrupt=true;return false;}
            SessionSummary s;if(!read_summary(line,s)){stats.storage_corrupt=true;return false;}parsed.recent.push_back(s);}
        if(!std::getline(in,line)||line.size()>20){stats.storage_corrupt=true;return false;}
        std::istringstream dc(line);count=0;if(!(dc>>tag>>count)||(dc>>extra)||tag!="daily"||count>UsageStats::MaxDailyRecords){stats.storage_corrupt=true;return false;}
        uint32_t previous=0;
        for(size_t i=0;i<count;++i){if(!std::getline(in,line)||line.size()>200){stats.storage_corrupt=true;return false;}
            std::istringstream row(line);DailyUsage d;
            if(!(row>>tag>>d.utc_day>>d.mirror_ms>>d.audio_ms>>d.mirror_sessions>>d.audio_sessions)||(row>>extra)||
               tag!="day"||(i&&d.utc_day<=previous)){stats.storage_corrupt=true;return false;}
            parsed.daily.push_back(d);previous=d.utc_day;}
        if(std::getline(in,line)){stats.storage_corrupt=true;return false;}
    }
    parsed.storage_corrupt=false;stats=parsed;return true;
}
bool save_usage_stats(const std::string& path,const UsageStats& s) {
    if(s.storage_corrupt||s.recent.size()>UsageStats::MaxRecentSessions||s.daily.size()>UsageStats::MaxDailyRecords)return false;
    std::ostringstream out;out<<"UXAIR_USAGE_V2\n";
    out<<"totals "<<s.playback_ms<<" "<<s.successful_sessions<<" "<<s.mirror_sessions<<" "<<s.audio_sessions<<" "<<s.direct_sessions<<" "
       <<s.incoming_frames<<" "<<s.feed_frames<<" "<<s.packet_drops<<" "<<s.video_frame_drops<<" "<<s.decoder_errors<<" "<<s.session_errors<<"\n";
    out<<"coverage "<<s.daily_from_day<<"\nsessions "<<s.recent.size()<<"\n";
    for(const auto& x:s.recent)out<<"session "<<x.started_utc<<" "<<x.duration_ms<<" "<<unsigned(x.kind)<<" "<<x.width<<" "<<x.height<<" "<<x.codec<<" "
        <<x.incoming_frames<<" "<<x.feed_frames<<" "<<x.packet_drops<<" "<<x.video_frame_drops<<" "<<x.decoder_errors<<" "<<x.session_errors<<"\n";
    out<<"daily "<<s.daily.size()<<"\n";
    for(const auto& d:s.daily)out<<"day "<<d.utc_day<<" "<<d.mirror_ms<<" "<<d.audio_ms<<" "<<d.mirror_sessions<<" "<<d.audio_sessions<<"\n";
    std::string bytes=out.str();if(bytes.size()>UsageStats::MaxSerializedBytes)return false;
    std::string temp=path+".tmp";FILE *f=fopen(temp.c_str(),"wb");if(!f)return false;chmod(temp.c_str(),0600);
    bool ok=fwrite(bytes.data(),1,bytes.size(),f)==bytes.size();
    if(fflush(f)!=0||fsync(fileno(f))!=0)ok=false;if(fclose(f)!=0)ok=false;
    if(ok&&rename(temp.c_str(),path.c_str())==0){size_t slash=path.find_last_of('/');std::string dir=slash==std::string::npos?".":path.substr(0,slash);
        int fd=open(dir.c_str(),O_RDONLY);if(fd>=0){fsync(fd);close(fd);}return true;}
    remove(temp.c_str());return false;
}
size_t usage_storage_bytes(const std::string& path) {
    size_t total=0;const std::string files[]={path,path+".tmp"};
    for(const std::string& name:files){struct stat st;if(stat(name.c_str(),&st)==0&&S_ISREG(st.st_mode)&&st.st_size>0)total+=size_t(st.st_size);}
    return total;
}
static uint32_t month_key(uint32_t day) {
    time_t t=time_t(uint64_t(day)*86400);struct tm v={};if(!gmtime_r(&t,&v))return 0;
    return uint32_t((v.tm_year+1900)*100+v.tm_mon+1);
}
static bool month_first_day(uint32_t key,uint32_t& day,std::string& label) {
    int year=int(key/100),month=int(key%100);if(year<1970||month<1||month>12)return false;
    struct tm v={};v.tm_year=year-1900;v.tm_mon=month-1;v.tm_mday=1;
    time_t t=timegm(&v);if(t<0)return false;day=usage_utc_day(uint64_t(t));
    struct tm normalized={};if(!gmtime_r(&t,&normalized))return false;
    char b[16];strftime(b,sizeof(b),"%b",&normalized);label=b;return true;
}
UsageSummary summarize_usage(const UsageStats& stats,UsagePeriod p,uint32_t today) {
    UsageSummary out;int slots=p==UsagePeriod::Months12?12:p==UsagePeriod::Days30?30:7;
    bool monthly=p==UsagePeriod::Months12;std::vector<uint32_t> keys(static_cast<size_t>(slots),0),month_starts(static_cast<size_t>(slots),0);
    std::vector<std::string> labels(static_cast<size_t>(slots));
    for(int i=0;i<slots;++i) {
        if(monthly) {
            uint32_t current=month_key(today);int y=int(current/100),m=int(current%100)+(i-(slots-1));
            while(m<1){m+=12;--y;}while(m>12){m-=12;++y;}keys[size_t(i)]=uint32_t(y*100+m);
            if(!month_first_day(keys[size_t(i)],month_starts[size_t(i)],labels[size_t(i)]))labels[size_t(i)]="?";
        } else {
            keys[size_t(i)]=today>=uint32_t(slots-1-i)?today-uint32_t(slots-1-i):0;month_starts[size_t(i)]=keys[size_t(i)];
            time_t t=time_t(uint64_t(keys[size_t(i)])*86400);struct tm d={};char b[16];
            if(gmtime_r(&t,&d)){strftime(b,sizeof(b),p==UsagePeriod::Days7?"%a":"%d",&d);labels[size_t(i)]=b;}
        }
    }
    out.bars.resize(size_t(slots));
    for(int i=0;i<slots;++i){out.bars[size_t(i)].label=labels[size_t(i)];
        out.bars[size_t(i)].available=stats.daily_from_day&&(monthly?month_key(month_starts[size_t(i)])>=month_key(stats.daily_from_day):month_starts[size_t(i)]>=stats.daily_from_day&&keys[size_t(i)]<=today);
        if(monthly&&out.bars[size_t(i)].available) {
            const uint32_t first=month_starts[size_t(i)];
            out.bars[size_t(i)].partial=std::max(first,stats.daily_from_day)>first||keys[size_t(i)]==month_key(today);
        }
    }
    for(const DailyUsage& d:stats.daily) {
        int slot=-1;
        for(int i=0;i<slots;++i)if(monthly?month_key(d.utc_day)==keys[size_t(i)]:d.utc_day==keys[size_t(i)]){slot=i;break;}
        if(slot<0||!out.bars[size_t(slot)].available)continue;
        UsageBar& b=out.bars[size_t(slot)];b.mirror_ms=sat_add(b.mirror_ms,d.mirror_ms);b.audio_ms=sat_add(b.audio_ms,d.audio_ms);
        out.mirror_ms=sat_add(out.mirror_ms,d.mirror_ms);out.audio_ms=sat_add(out.audio_ms,d.audio_ms);
        out.mirror_sessions+=d.mirror_sessions;out.audio_sessions+=d.audio_sessions;
    }
    return out;
}
