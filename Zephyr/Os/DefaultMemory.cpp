// ======================================================================
// \title fprime-baremetal/Os/Zephyr/DefaultMemory.cpp
// \brief sets default Os::Memory to stub implementation via linker
// ======================================================================
#include "Os/Memory.hpp"
#include "fprime-baremetal/Os/Zephyr/Memory.hpp"
#include "Os/Delegate.hpp"

namespace Os {
MemoryInterface* MemoryInterface::getDelegate(MemoryHandleStorage& aligned_new_memory) {
    return Os::Delegate::makeDelegate<MemoryInterface, Os::Zephyr::ZephyrMemory>(aligned_new_memory);
}
}
