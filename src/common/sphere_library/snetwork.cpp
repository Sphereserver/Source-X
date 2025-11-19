#include "snetwork.h"
#include <chrono>
#include <future>
//#include <string>
//#include <vector>
#include <thread>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // getaddrinfo, freeaddrinfo, NI_MAXHOST
#else
#include <netdb.h>     // getaddrinfo, freeaddrinfo
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> // sockaddr_in
#include <unistd.h>
#endif

namespace sl {

// Thread-safe DNS resolve with timeout
// - Runs getaddrinfo(AF_INET, AI_CANONNAME) in a background thread, since that api doesn't have a timeout feature/parameter.
// - Waits up to `timeout` and returns true only if at least one IPv4 address is obtained
// On Windows, ensure WSAStartup() succeeded before calling.
auto hostname_resolve_with_timeout_v4(std::string_view name, int timeout_milliseconds)
    -> std::pair<bool, ResolveResultV4>
{
    const auto timeout = std::chrono::milliseconds(timeout_milliseconds);

    // Worker returns its own ResolveResultV4; no references to caller state.
    // To add IPv6 later, switch to AF_UNSPEC and also handle AF_INET6 entries
    std::packaged_task<ResolveResultV4()> task(
        [name]() -> ResolveResultV4
        {
            ResolveResultV4 rr;

            addrinfo hints{};
            hints.ai_family   = AF_INET;     // IPv4 only, to mirror gethostbyname semantics
            hints.ai_socktype = 0;           // any
            hints.ai_flags    = AI_CANONNAME;

            addrinfo* res = nullptr;
            const int rc = getaddrinfo(name.data(), nullptr, &hints, &res);
            if (rc != 0 || !res)
                return rr; // empty => failure

            // Canonical name (like hostent->h_name)
            if (res->ai_canonname)
                rr.canon = res->ai_canonname;

            // Collect IPv4 addresses in network byte order (like hostent->h_addr_list)
            for (addrinfo* p = res; p; p = p->ai_next)
            {
                if (p->ai_family != AF_INET || !p->ai_addr) continue;
                auto sin = reinterpret_cast<const sockaddr_in*>(p->ai_addr);
                rr.addrs_v4.push_back(static_cast<uint32_t>(sin->sin_addr.s_addr));
            }

            freeaddrinfo(res);
            return rr;
        });

    std::future<ResolveResultV4> fut = task.get_future();
    std::thread(std::move(task)).detach(); // detached worker never touches caller-owned memory

    if (fut.wait_for(timeout) != std::future_status::ready) {
        return {false, {}}; // timeout: caller keeps local hostname and skips address enumeration
    }

    ResolveResultV4 rr = fut.get(); // synchronized handoff; worker’s writes are now visible
    if (rr.addrs_v4.empty()) {
        return {false, {}}; // treat empty set as resolution failure, keep local hostname
    }

    return {true, rr};
}


}
