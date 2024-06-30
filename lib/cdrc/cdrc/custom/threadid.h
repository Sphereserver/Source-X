#ifndef _INC_CDRC_THREADID_H
#define _INC_CDRC_THREADID_H

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace cdrc
{
namespace utils
{

size_t num_threads();

struct ThreadID {
    static std::vector<std::atomic<bool>> in_use; // initialize to false
    int tid;

    ThreadID();
    ~ThreadID() {
        in_use[tid] = false;
    }

    int getTID() const { return tid; }
};

extern thread_local ThreadID threadID;

}
} // namespace

#endif // _INC_THREADID_H
