#include "usage_stats.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

const char *session_kind_name(SessionKind kind) {
    switch (kind) {
    case SessionKind::Audio: return "Audio";
    case SessionKind::Direct: return "Direct";
    default: return "Mirror";
    }
}
void UsageStats::record(const SessionSummary& s) {
    ++successful_sessions;
    if(s.kind==SessionKind::Audio) ++audio_sessions;
    else if(s.kind==SessionKind::Direct) ++direct_sessions;
    else ++mirror_sessions;
    playback_ms+=s.duration_ms;
    incoming_frames+=s.incoming_frames; feed_frames+=s.feed_frames;
    packet_drops+=s.packet_drops; video_frame_drops+=s.video_frame_drops;
    decoder_errors+=s.decoder_errors; session_errors+=s.session_errors;
    recent.push_front(s);
    while(recent.size()>16) recent.pop_back();
}
void UsageStats::reset() { *this=UsageStats(); }

static bool read_summary(const std::string& line, SessionSummary& s) {
    std::istringstream in(line); std::string tag; unsigned kind=0;
    if(!(in>>tag>>s.started_utc>>s.duration_ms>>kind>>s.width>>s.height>>s.codec
        >>s.incoming_frames>>s.feed_frames>>s.packet_drops>>s.video_frame_drops>>s.decoder_errors>>s.session_errors)) return false;
    std::string extra;
    if(tag!="session" || (in>>extra) || kind<1 || kind>3 || s.width>3840 || s.height>2160 || s.codec>2) return false;
    s.kind=static_cast<SessionKind>(kind); return true;
}
bool load_usage_stats(const std::string& path, UsageStats& stats) {
    std::ifstream in(path.c_str()); std::string line;
    if(!std::getline(in,line) || line!="UXPLAY_USAGE_V1") return false;
    UsageStats parsed;
    if(!std::getline(in,line) || line.size()>300) return false;
    std::istringstream totals(line); std::string tag;
    if(!(totals>>tag>>parsed.playback_ms>>parsed.successful_sessions>>parsed.mirror_sessions
        >>parsed.audio_sessions>>parsed.direct_sessions>>parsed.incoming_frames>>parsed.feed_frames
        >>parsed.packet_drops>>parsed.video_frame_drops>>parsed.decoder_errors>>parsed.session_errors) || tag!="totals") return false;
    std::string extra;
    if(totals>>extra) return false;
    while(std::getline(in,line)) {
        if(parsed.recent.size()==16 || line.size()>300) return false;
        SessionSummary s;
        if(!read_summary(line,s)) return false;
        parsed.recent.push_back(s);
    }
    if(parsed.mirror_sessions+parsed.audio_sessions+parsed.direct_sessions!=parsed.successful_sessions) return false;
    stats=parsed; return true;
}
bool save_usage_stats(const std::string& path, const UsageStats& stats) {
    const std::string temp=path+".tmp";
    FILE *f=fopen(temp.c_str(),"w"); if(!f) return false;
    chmod(temp.c_str(),0600);
    bool ok=fprintf(f,"UXPLAY_USAGE_V1\ntotals %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
        (unsigned long long)stats.playback_ms,(unsigned long long)stats.successful_sessions,
        (unsigned long long)stats.mirror_sessions,(unsigned long long)stats.audio_sessions,(unsigned long long)stats.direct_sessions,
        (unsigned long long)stats.incoming_frames,(unsigned long long)stats.feed_frames,(unsigned long long)stats.packet_drops,(unsigned long long)stats.video_frame_drops,
        (unsigned long long)stats.decoder_errors,(unsigned long long)stats.session_errors)>0;
    for(const auto& s:stats.recent) {
        if(fprintf(f,"session %llu %llu %u %u %u %u %llu %llu %llu %llu %llu %llu\n",
            (unsigned long long)s.started_utc,(unsigned long long)s.duration_ms,unsigned(s.kind),s.width,s.height,s.codec,
            (unsigned long long)s.incoming_frames,(unsigned long long)s.feed_frames,(unsigned long long)s.packet_drops,(unsigned long long)s.video_frame_drops,
            (unsigned long long)s.decoder_errors,(unsigned long long)s.session_errors)<0) ok=false;
    }
    if(fflush(f)!=0 || fsync(fileno(f))!=0) ok=false;
    if(fclose(f)!=0) ok=false;
    if(ok && rename(temp.c_str(),path.c_str())==0) return true;
    remove(temp.c_str()); return false;
}
