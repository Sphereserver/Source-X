#ifndef INC_CONTAINER_OPS_H
#define INC_CONTAINER_OPS_H

#include <initializer_list>

template<typename inT, typename outT>
void AssignInitlistToCSizedArray(outT* dest, size_t dest_size, std::initializer_list<inT> const& initializer)
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