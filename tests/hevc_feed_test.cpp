#include "hevc_feed.h"
#include <cassert>

int main() {
    using namespace hevc_probe;
    assert(classify_feed_reply("{\"returnValue\":\"Ok\"}") == FeedReply::Accepted);
    assert(classify_feed_reply("BufferFull") == FeedReply::Backpressure);
    assert(classify_feed_reply("Pending") == FeedReply::Backpressure);
    assert(classify_feed_reply("") == FeedReply::Rejected);
    assert(classify_feed_reply("Error") == FeedReply::Rejected);
    assert(send_eos_after_feed(30,30,false,false));
    assert(!send_eos_after_feed(4,30,false,false));
    assert(!send_eos_after_feed(30,30,true,false));
    assert(!send_eos_after_feed(30,30,false,true));
}
