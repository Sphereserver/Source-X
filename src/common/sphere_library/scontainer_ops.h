#ifndef INC_CONTAINER_OPS_H
#define INC_CONTAINER_OPS_H

#include <cstddef>
#include <initializer_list>

namespace sl
{

// Define a concept for STL containers
template <typename T>
concept ConceptBasicContainer = requires(T t)
{
    // Type must have an iterator type
    typename T::iterator;

    // Must have a begin() method returning an iterator
    { t.begin() } -> std::same_as<typename T::iterator>;

    // Must have an end() method returning an iterator
    { t.end() } -> std::same_as<typename T::iterator>;
};


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

template <ConceptBasicContainer T>
bool ContainerIsSorted(T const& cont)
{
    return std::is_sorted(cont.begin(), cont.end());
}

template <ConceptBasicContainer T>
bool SortedContainerHasDuplicates(T const& cont)
{
    return std::adjacent_find(cont.begin(), cont.end()) != cont.end();
}

/*
template <ConceptBasicContainer ContT, typename PredT>
bool SortedContainerHasDuplicatesPred(ContT const& cont, PredT const& predicate)
{
    return std::adjacent_find(cont.begin(), cont.end(), predicate) == cont.end();
}
*/

template <ConceptBasicContainer T>
bool UnsortedContainerHasDuplicates(const T& cont) noexcept
{
    for (size_t i = 0; i < cont.size(); ++i) {
        for (size_t j = i + 1; j < cont.size(); ++j) {
            if ((i != j) && (cont[i] == cont[j])) {
                return true; // Found a duplicate
            }
        }
    }
    return false; // No duplicates found
}

} // namespace sl

#endif // INC_CONTAINER_OPS_H
