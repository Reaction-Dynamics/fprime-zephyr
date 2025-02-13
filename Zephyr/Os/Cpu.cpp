// ======================================================================
// \title fprime-baremetal/Os/Zephyr/Cpu.hpp
// \brief stub implementation for Os::Zephyr::ZephyrCpu, implementations
// ======================================================================
#include <fprime-baremetal/Os/Zephyr/Cpu.hpp>

namespace Os {
namespace Zephyr {

CpuInterface::Status ZephyrCpu::_getCount(FwSizeType& cpu_count) {
    // Assumes singular CPU
    cpu_count = 1;
    return Status::OP_OK;
}

CpuInterface::Status ZephyrCpu::_getTicks(Os::Cpu::Ticks& ticks, FwSizeType cpu_index) {
    // Zephyr currently returns 100% in all cases
    ticks.total = 1;
    ticks.used = 1;
    return Status::OP_OK;
}

CpuHandle* ZephyrCpu::getHandle() {
    return &this->m_handle;
}

} // namespace Zephyr
} // namespace Os
