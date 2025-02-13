// ======================================================================
// \title fprime-baremetal/Os/Zephyr/Memory.cpp
// \brief implementation for Os::Zephyr::ZephyrMemory
// ======================================================================
#include <fprime-baremetal/Os/Zephyr/Memory.hpp>

namespace Os {
namespace Zephyr {

MemoryInterface::Status ZephyrMemory::_getUsage(Os::Memory::Usage& memory_usage) {
    memory_usage.used = 1;
    memory_usage.total = 1;
    return Status::OP_OK;
}


MemoryHandle* ZephyrMemory::getHandle() {
    return &this->m_handle;
}

} // namespace Zephyr
} // namespace Os
