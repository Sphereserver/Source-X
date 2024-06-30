#include "threadid.h"
#include "../internal/utils.h"
#include <cassert>

namespace cdrc
{
namespace utils
{

size_t num_threads() {
    static size_t n_threads = []() -> size_t {
        //if (const auto env_p = std::getenv("NUM_THREADS")) {
        //    return std::stoi(env_p) + 1;
        //}
        //else {
            return std::thread::hardware_concurrency() + 1;
        //}
        }();
    return n_threads;
}

ThreadID::ThreadID() {
    for (size_t i = 0; i < num_threads(); i++) {
        bool expected = false;
        if (!in_use[i] && in_use[i].compare_exchange_strong(expected, true)) {
            tid = i;
            return;
        }
    }
    //std::cerr << "Error: more than " << num_threads() << " threads created" << std::endl;
    //std::exit(1);
    assert(false);
}

thread_local ThreadID threadID;
std::vector<std::atomic<bool>> ThreadID::in_use(num_threads());

}
} // namespace
