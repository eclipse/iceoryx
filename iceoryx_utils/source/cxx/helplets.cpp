#include "iceoryx_utils/cxx/helplets.hpp"

namespace iox
{
namespace cxx
{
void* alignedAlloc(const uint64_t alignment, const uint64_t size) noexcept
{
    // -1 == since the max alignment addition is alignment - 1 otherwise the
    // memory is already aligned and we have to do nothing
    uint64_t memory = reinterpret_cast<uint64_t>(malloc(size + alignment + sizeof(void*) - 1));
    if (memory == 0)
    {
        return nullptr;
    }
    uint64_t alignedMemory = align(memory + sizeof(void*), alignment);
    reinterpret_cast<void**>(alignedMemory)[-1] = reinterpret_cast<void*>(memory);

    return reinterpret_cast<void*>(alignedMemory);
}

void alignedFree(void* const memory)
{
    if (memory != nullptr)
    {
        free(reinterpret_cast<void**>(memory)[-1]);
    }
}
} // namespace cxx
} // namespace iox
