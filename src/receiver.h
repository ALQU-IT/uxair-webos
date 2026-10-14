#pragma once
#include <atomic>
#include <deque>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include "raop.h"
#include "diagnostics.h"
struct Packet {
    bool video;
    unsigned codec;
    uint64_t timestamp;
    std::vector<uint8_t> data;
};
struct ReceiverDiagnostics {
    size_t queue_packets = 0, queue_bytes = 0;
    size_t peak_queue_packets = 0, peak_queue_bytes = 0;
    uint64_t dropped_packets = 0, dropped_video_frames = 0;
};
struct Receiver {
    ~Receiver();
    bool start(const std::string& directory);
    void stop();
    void disconnect();
    bool pop(Packet& packet, uint64_t latest = UINT64_MAX);
    void enqueue(bool video, unsigned codec, uint64_t time, const uint8_t *data, int size);
    void clear();
    ReceiverDiagnostics diagnostics();
    VideoTimingSample take_video_timing();
    std::string status();
    void show(const std::string& text);
    std::string pin();
    void adjust_audio_delay(int delta_ms);
    std::atomic<int> audio_delay_ms{100};
    std::atomic<bool> reset{false}, audio_flush{false}, paused{false}, has_audio{false}, mirror{false};
    std::atomic<int> width{1280},height{720};
    std::atomic<float> volume{1};
    std::atomic<int64_t> activity{0};
    std::atomic<int64_t> last_audio_arrival{0};
    std::atomic<unsigned> overflow{0};
    std::atomic<unsigned> session_resets{0};
    raop_t *raop = nullptr;
    dnssd_t *dnssd = nullptr;
    std::mutex mutex;
    std::deque<Packet> packets;
    size_t bytes=0;
    size_t peak_packets=0, peak_bytes=0;
    uint64_t dropped_packets=0, dropped_video_frames=0;
    VideoTiming video_timing;
    std::string directory, message="Waiting for a connection", pin_text, client;
    std::set<std::string> keys;
};
