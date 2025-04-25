#ifndef INC_CONTAINER_OPS_H
#define INC_CONTAINER_OPS_H

#include <cstddef>
#include <initializer_list>

//#   define BENCHMARK_LISTS // TODO
//#define DEBUG_LIST_FUNCS

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
void AssignInitlistToCSizedArray(outT* dest, const size_t dest_size, std::initializer_list<inT> const& initializer) noexcept
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
[[nodiscard]] constexpr
bool ContainerIsSorted(T const& cont)
{
    return std::is_sorted(cont.begin(), cont.end());
}

template <ConceptBasicContainer T>
[[nodiscard]] constexpr
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
[[nodiscard]]
bool UnortedContainerHasDuplicates(const T& cont) noexcept
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

template <typename T>
static void SortedVecRemoveElementsByIndices(std::vector<T>& vecMain, const std::vector<size_t>& vecIndicesToRemove)
{
    // Erase in chunks, call erase the least times possible.
    if (vecIndicesToRemove.empty())
        return;

    if (vecMain.empty())
        return;

#ifdef DEBUG_LIST_FUNCS
    const size_t sz = vecMain.size();

    ASSERT(sl::ContainerIsSorted(vecMain));
    ASSERT(sl::ContainerIsSorted(vecIndicesToRemove));
    ASSERT(!sl::SortedContainerHasDuplicates(vecMain));
    ASSERT(!sl::SortedContainerHasDuplicates(vecIndicesToRemove));

    // Copy the original vector to check against later
    std::vector<T> originalVecMain = vecMain;
#endif

/*
    // IMPLEMENTATION 1
    // Start from the end of vecIndicesToRemove, working backwards with normal iterators
    auto itRemoveFirst = vecIndicesToRemove.end();  // Points one past the last element
    while (itRemoveFirst != vecIndicesToRemove.begin())
    {
        // Move the iterator back to point to a valid element.
        --itRemoveFirst;

        auto itRemoveLast = itRemoveFirst;  // Marks the start of a contiguous block
        // Find contiguous block by working backwards and finding consecutive indices
        // This loop identifies a contiguous block of indices to remove.
        // A block is contiguous if the current index *itRemoveFirst is exactly 1 greater than the previous index.
        while (itRemoveFirst != vecIndicesToRemove.begin() && *(itRemoveFirst - 1) + 1 == *itRemoveFirst)
        {
            --itRemoveFirst;
        }

        // Once we find a contiguous block, we erase that block from vecMain.
        auto itRemoveLastPast = (*itRemoveLast == vecMain.size() - 1) ? vecMain.end() : (vecMain.begin() + *itRemoveLast + 1);
        vecMain.erase(vecMain.begin() + *itRemoveFirst, itRemoveLastPast);
    }
*/

    // IMPLEMENTATION 2.
    // Move and swap the elements in memory and remove in one pass the discarded elements at the end of the vector.
    // (like std::remove + std::erase does)

    size_t writeIdx = 0;  // position to write kept elements
    size_t prevIdx = 0;   // start index of next block to keep

    // Process each index to remove

    // Hints for std::move:
    // template< class InputIt, class OutputIt >
    // OutputIt move( InputIt first, InputIt last, OutputIt d_first );
    //   Moves the elements in the range [first, last), to another range beginning at d_first, starting from first and proceeding to last.
    //   After this operation the elements in the moved-from range will still contain valid values of the appropriate type, but not necessarily the same values as before the move.

    for (size_t idx : vecIndicesToRemove)
    {
        // Move block of elements from prevIdx up to (but not including) idx
        if (prevIdx < idx)
        {
            std::move(vecMain.begin() + prevIdx, vecMain.begin() + idx, vecMain.begin() + writeIdx);
            writeIdx += idx - prevIdx;
        }
        // Skip the element at idx (to remove)
        prevIdx = idx + 1;
    }

    // Move any remaining elements after the last removed index
    if (prevIdx < vecMain.size())
    {
        std::move(vecMain.begin() + prevIdx, vecMain.end(), vecMain.begin() + writeIdx);
        writeIdx += vecMain.size() - prevIdx;
    }

    // Erase the leftover elements
    vecMain.erase(vecMain.begin() + writeIdx, vecMain.end());

#ifdef DEBUG_LIST_FUNCS
    // Sanity Check: Verify that the removed elements are no longer present in vecMain
    for (auto index : vecIndicesToRemove) {
        UnreferencedParameter(index);
        ASSERT(index < originalVecMain.size());
        ASSERT(std::find(vecMain.begin(), vecMain.end(), originalVecMain[index]) == vecMain.end());
    }
    ASSERT(vecMain.size() == sz - vecIndicesToRemove.size());
#endif
}

template <typename T>
static void UnsortedVecRemoveElementsByValues(std::vector<T>& vecMain, const std::vector<T> & vecValuesToRemove)
{
    if (vecValuesToRemove.empty())
        return;

#ifdef DEBUG_LIST_FUNCS
    ASSERT(!sl::UnortedContainerHasDuplicates(vecMain));
    ASSERT(!sl::UnortedContainerHasDuplicates(vecValuesToRemove));
#endif

    // Sort valuesToRemove for binary search
    //std::sort(vecValuesToRemove.begin(), vecValuesToRemove.end());

    // Use std::remove_if to shift elements not marked for removal to the front
    auto it = std::remove_if(vecMain.begin(), vecMain.end(),
        [&](const T& element) {
            // Use binary search to check if the element should be removed
            //return std::binary_search(vecValuesToRemove.begin(), vecValuesToRemove.end(), element);
            return std::find(vecValuesToRemove.begin(), vecValuesToRemove.end(), element) != vecValuesToRemove.end();
        });

    // Erase the removed elements in one go
    vecMain.erase(it, vecMain.end());

#ifdef DEBUG_LIST_FUNCS
    for (auto& elem : vecMain) {
        ASSERT(std::find(vecValuesToRemove.begin(), vecValuesToRemove.end(), elem) == vecValuesToRemove.end());
    }
#endif
}

/*
// To be tested and benchmarked.
template <typename T>
static void sortedVecRemoveElementsByValues(std::vector<T>& vecMain, const std::vector<T>& toRemove)
{
    if (toRemove.empty() || vecMain.empty())
        return;

    auto mainIt = vecMain.begin();
    auto removeIt = toRemove.begin();

    // Destination pointer for in-place shifting
    auto destIt = mainIt;

    while (mainIt != vecMain.end() && removeIt != toRemove.end()) {
        // Skip over elements in the main vector that are smaller than the current element to remove
        auto nextRangeEnd = std::lower_bound(mainIt, vecMain.end(), *removeIt);

        // Batch copy the range of elements not marked for removal
        std::move(mainIt, nextRangeEnd, destIt);
        destIt += std::distance(mainIt, nextRangeEnd);

        // Advance main iterator and remove iterator
        mainIt = nextRangeEnd;

        // Skip the elements that need to be removed
        if (mainIt != vecMain.end() && *mainIt == *removeIt) {
            ++mainIt;
            ++removeIt;
        }
    }

    // Copy the remaining elements if there are any left
    std::move(mainIt, vecMain.end(), destIt);

    // Resize the vector to remove the now extraneous elements at the end
    vecMain.resize(destIt - vecMain.begin());
}
*/


} // namespace sl


#endif // INC_CONTAINER_OPS_H
