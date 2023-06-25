#ifndef INC_CONTAINER_OPS_H
#define INC_CONTAINER_OPS_H

#include <cstddef>
#include <initializer_list>

template<typename inT, typename outT>
void AssignInitlistToCSizedArray(outT* dest, const size_t dest_size, std::initializer_list<inT> initializer) noexcept
{
    size_t i = 0;
    for (auto const& elem : initializer)
    {
        if (i >= dest_size)
            break;
        dest[i] = elem;
        ++i;
    }
}

#endif // INC_CONTAINER_OPS_H
