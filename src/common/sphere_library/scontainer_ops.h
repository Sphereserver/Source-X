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

#ifdef DEBUG_LIST_FUNCS
    // Sanity Check: Verify that the removed elements are no longer present in vecMain
    for (auto index : vecIndicesToRemove) {
        UnreferencedParameter(index);
        ASSERT(index < originalVecMain.size());
        ASSERT(std::find(vecMain.begin(), vecMain.end(), originalVecMain[index]) == vecMain.end());
    }
    ASSERT(vecMain.size() == sz - vecIndicesToRemove.size());
#endif


    /*
    // Alternative implementation:
    // We cannot use a vector with the indices but we need a vector with a copy of the elements to remove.
    // std::remove_if in itself is more efficient than multiple erase calls, because the number of the element shifts is lesser.
    // Though, we need to consider the memory overhead of reading through an std::pair of two types, which is bigger than just an index.
    // Also, jumping across the vector in a non-contiguous way with the binary search can add additional memory overhead by itself, and
    //   this will be greater the bigger are the elements in the vector..
    // The bottom line is that we need to run some benchmarks between the two algorithms, and possibly also for two versions of this algorithm,
    //   one using binary search and another with linear search.
    // The latter might actually be faster for a small number of elements, since it's more predictable for the prefetcher.

    // Use std::remove_if to shift elements not marked for removal to the front
    auto it = std::remove_if(vecMain.begin(), vecMain.end(),
        [&](const T& element) {
            // Check if the current element is in the valuesToRemove vector using binary search
            return std::binary_search(valuesToRemove.begin(), valuesToRemove.end(), element);
        });

    // Erase the removed elements in one go
    vecMain.erase(it, vecMain.end());
    */
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

/*
template <typename TPair, typename T>
static void sortedVecDifference(
    const std::vector<TPair>& vecMain, const std::vector<T*>& vecToRemove, std::vector<TPair>& vecElemBuffer
    )
{
    auto itMain = vecMain.begin();    // Iterator for vecMain
    auto start = itMain;              // Start marker for non-matching ranges in vecMain

    for (auto& elem : vecToRemove) {
        g_Log.EventDebug("Should remove %p.\n", (void*)elem);
    }
    for (auto& elem : vecMain) {
        g_Log.EventDebug("VecMain %p.\n", (void*)elem.second);
    }

    // Iterate through each element in vecToRemove to locate and exclude its matches in vecMain
    for (const auto& removePtr : vecToRemove) {
        // Binary search to find the start of the block where vecMain.second == removePtr
        itMain = std::lower_bound(itMain, vecMain.end(), removePtr,
            [](const TPair& lhs, const T* rhs) noexcept {
                return lhs.second < rhs;  // Compare TPair.second with T*
            });

        // Insert all elements from `start` up to `itMain` (non-matching elements)
        vecElemBuffer.insert(vecElemBuffer.end(), start, itMain);

        // Skip over all contiguous elements in vecMain that match removePtr
        while (itMain != vecMain.end() && itMain->second == removePtr) {
            ++itMain;
        }

        // Update `start` to the new non-matching range after any matching elements are skipped
        start = itMain;
    }

    // Insert remaining elements from vecMain after the last matched element
    vecElemBuffer.insert(vecElemBuffer.end(), start, vecMain.end());

    for (auto& elem : vecMain) {
        g_Log.EventDebug("VecMain %p.\n", (void*)elem.second);
    }
}
*/

template <typename TPair, typename T>
static void UnsortedVecDifference(
    std::vector<TPair>      & vecMain,
    std::vector<TPair>      & vecElemBuffer,
    std::vector<T*>    const& vecToRemove)
{
    /*
    // Iterate through vecMain, adding elements to vecElemBuffer only if they aren't in vecToRemove
    for (const auto& elem : vecMain) {
        if (std::find(vecToRemove.begin(), vecToRemove.end(), elem.second) == vecToRemove.end()) {
            vecElemBuffer.push_back(elem);
        }
    }
    */
    // vecMain is sorted by timeout (first elem of the pair), not by pointer (second elem)
    // vecToRemove is sorted by its contents (pointer)

    // Reserve space in vecElemBuffer to avoid reallocations
    vecElemBuffer.reserve(vecMain.size() - vecToRemove.size());

    // Use an iterator to store the position for bulk insertion
    auto itCopyFromThis = vecMain.begin();
    /*
    auto itFindBegin = vecToRemove.begin();

    // Iterate through vecMain, copying elements that are not in vecToRemove
    for (auto itMain = vecMain.begin(); itMain != vecMain.end(); ++itMain)
    {
        // Perform a linear search for the current element's pointer in vecToRemove
        auto itTemp = std::find(itFindBegin, vecToRemove.end(), itMain->second);
        if (itTemp != vecToRemove.end())
        {
            // If the element is found in vecToRemove, copy elements before it
            vecElemBuffer.insert(vecElemBuffer.end(), itCopyFromThis, itMain); // Copy up to but not including itMain

            // Move itCopyFromThis forward to the next element
            itCopyFromThis = itMain + 1;
            //itFindBegin = itTemp + 1;

            // Update itFindBegin to continue searching for the next instance in vecToRemove
            // We do not change itFindBegin here, since we want to keep searching for this pointer
            // in vecToRemove for subsequent elements in vecMain
        }
        else
        {
            // If itTemp is not found, we can still copy the current element
            // Check if itCopyFromThis is equal to itMain to avoid double-copying in case of duplicates
            if (itCopyFromThis != itMain)
            {
                // Move itCopyFromThis forward to the next element
                itCopyFromThis = itMain + 1;
            }
        }
    }*/

    // TODO: maybe optimize this algorithm.
    // Iterate through vecMain, copying elements that are not in vecToRemove
    for (auto itMain = vecMain.begin(); itMain != vecMain.end(); ++itMain) {
        // Perform a linear search for the current element's pointer in vecToRemove
        auto itTemp = std::find(vecToRemove.begin(), vecToRemove.end(), itMain->second);
        if (itTemp != vecToRemove.end()) {
            // If the element is found in vecToRemove, copy elements before it
            vecElemBuffer.insert(vecElemBuffer.end(), itCopyFromThis, itMain); // Copy up to but not including itMain

            // Move itCopyFromThis forward to the next element
            itCopyFromThis = itMain + 1; // Move to the next element after itMain
        }
    }

    // Copy any remaining elements in vecMain after the last found element
    vecElemBuffer.insert(vecElemBuffer.end(), itCopyFromThis, vecMain.end());

#ifdef DEBUG_LIST_FUNCS
    ASSERT(vecElemBuffer.size() == vecMain.size() - vecToRemove.size());
#endif

    //vecMain = std::move(vecElemBuffer);
    vecMain.swap(vecElemBuffer);
}

template <typename TPair, typename T>
static void SortedVecRemoveAddQueued(
    std::vector<TPair> &vecMain, std::vector<TPair> &vecElemBuffer,
    std::vector<T> const& vecToRemove, std::vector<TPair> const& vecToAdd)
{
#ifdef DEBUG_LIST_FUNCS
    ASSERT(sl::ContainerIsSorted(vecMain));
    ASSERT(!sl::SortedContainerHasDuplicates(vecMain));

    ASSERT(sl::ContainerIsSorted(vecToRemove));
    ASSERT(!sl::SortedContainerHasDuplicates(vecToRemove));

    ASSERT(sl::ContainerIsSorted(vecToAdd));
    ASSERT(!sl::SortedContainerHasDuplicates(vecToAdd));
#endif

    if (!vecToRemove.empty())
    {
        if (vecMain.empty()) {
            ASSERT(false);  // Shouldn't ever happen.
        }

        // TODO: test and benchmark if the approach of the above function (sortedVecRemoveElementsInPlace) might be faster.
        vecElemBuffer.clear();
        vecElemBuffer.reserve(vecMain.size() - vecToRemove.size());

        // TODO: use a sorted algorithm?
        UnsortedVecDifference(vecMain, vecElemBuffer, vecToRemove);
        vecElemBuffer.clear();

#ifdef DEBUG_LIST_FUNCS
        for (auto& elem : vecToRemove)
        {
            auto it = std::find_if(vecMain.begin(), vecMain.end(), [elem](auto &rhs) constexpr noexcept {return elem == rhs.second;});
            UnreferencedParameter(it);
            ASSERT (it == vecMain.end());
        }

        ASSERT(sl::ContainerIsSorted(vecMain));
        ASSERT(!sl::SortedContainerHasDuplicates(vecMain));
#endif
    }

    if (!vecToAdd.empty())
    {
        vecElemBuffer.clear();
        vecElemBuffer.reserve(vecMain.size() + vecToAdd.size());
        // MergeSort
        std::merge(
            vecMain.begin(), vecMain.end(),
            vecToAdd.begin(), vecToAdd.end(),
            std::back_inserter(vecElemBuffer)
            );

#ifdef DEBUG_LIST_FUNCS
        ASSERT(vecElemBuffer.size() == vecMain.size() + vecToAdd.size());
#endif

        vecMain.swap(vecElemBuffer);
        //vecMain = std::move(vecElemBuffer);
        vecElemBuffer.clear();

#ifdef DEBUG_LIST_FUNCS
        ASSERT(sl::ContainerIsSorted(vecMain));
        ASSERT(!sl::SortedContainerHasDuplicates(vecMain));
#endif

    }

    //EXC_CATCH:
}


} // namespace sl


#endif // INC_CONTAINER_OPS_H
