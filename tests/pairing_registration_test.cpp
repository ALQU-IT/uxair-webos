#include "pairing_registration.h"
#include "pairing_ui.h"
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unistd.h>

struct Completion {
    std::mutex mutex;
    std::condition_variable changed;
    bool started = false;
    bool ready = false;
    PairingRegistrationResult result = PairingRegistrationResult::PersistenceFailed;
    bool dismissed = false;
};

int main() {
    char temp[] = "/tmp/uxair-pairing-XXXXXX";
    char *directory = mkdtemp(temp);
    assert(directory);

    PairingOverlayState pairing;
    std::mutex state_mutex, store_mutex;
    std::set<std::string> keys;
    std::string status = "Waiting for a connection";

    // Required hardware sequence: first request is cancelled, then a fresh
    // PIN is accepted after the receiver restart.
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        assert(pairing.update_pin("111222"));
        assert(pairing_input_result(pairing.visible(), true, false)==PairingInputResult::Cancel);
        assert(pairing.cancelled());
        assert(pairing.update_pin("333444"));
    }

    // Hold the persistence lock to model slow flash. The SDL/UI state lock
    // must remain available while the callback waits to write the key.
    store_mutex.lock();
    Completion completion;
    const std::string key(44, 'A');
    std::thread callback([&] {
        {
            std::lock_guard<std::mutex> lock(completion.mutex);
            completion.started = true;
        }
        completion.changed.notify_one();
        bool dismissed = false;
        PairingRegistrationResult result = register_pairing_key(
            store_mutex, state_mutex, keys, pairing, status, directory, key, dismissed);
        {
            std::lock_guard<std::mutex> lock(completion.mutex);
            completion.result = result;
            completion.dismissed = dismissed;
            completion.ready = true;
        }
        completion.changed.notify_one();
    });

    {
        std::unique_lock<std::mutex> lock(completion.mutex);
        if (!completion.changed.wait_for(lock, std::chrono::seconds(3), [&]{return completion.started;})) {
            std::fputs("FAIL: callback did not start\n", stderr);
            std::_Exit(2);
        }
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex);
        assert(pairing.visible() && pairing.pin()=="333444");
        assert(pairing_input_result(pairing.visible(), false, false)==PairingInputResult::Consumed);
    }
    store_mutex.unlock();

    {
        std::unique_lock<std::mutex> lock(completion.mutex);
        if (!completion.changed.wait_for(lock, std::chrono::seconds(3), [&]{return completion.ready;})) {
            std::fputs("FAIL: callback-to-UI pairing transition timed out\n", stderr);
            std::_Exit(2);
        }
    }
    callback.join();
    assert(completion.result==PairingRegistrationResult::Added);
    assert(completion.dismissed);
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        assert(!pairing.visible());
        assert(status=="AirPlay pairing complete");
        assert(keys.count(key)==1);
    }
    std::ifstream saved(std::string(directory)+"/paired.keys");
    std::string stored;
    assert(std::getline(saved,stored) && stored==key);
    unlink((std::string(directory)+"/paired.keys").c_str());
    rmdir(directory);
    std::puts("PASS: cancelled pairing, responsive UI during key persistence, callback completion and PIN dismissal");
}
