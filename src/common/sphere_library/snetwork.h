#ifndef SNETWORK_H
#define SNETWORK_H

#include "../common.h"

namespace sl {

struct ResolveResultV4 {
    std::string canon;                 // canonical DNS name if provided (like hostent->h_name)
    std::vector<uint32_t> addrs_v4;    // sin_addr.s_addr values (network byte order), like h_addr_list
};

auto hostname_resolve_with_timeout_v4(std::string_view name, int timeout_milliseconds)
    -> std::pair<bool, ResolveResultV4>;
}

#endif // SNETWORK_H
