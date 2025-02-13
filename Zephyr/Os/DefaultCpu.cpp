// ======================================================================
// \title Os/Zephyr/DefaultCpu.cpp
// \brief sets default Os::Zephyr::ZephyrCpu to stub implementation via linker
// ======================================================================
#include "Os/Cpu.hpp"
#include "fprime-baremetal/Os/Zephyr//Cpu.hpp"
#include "Os/Delegate.hpp"

namespace Os {
CpuInterface* CpuInterface::getDelegate(CpuHandleStorage& aligned_new_memory) {
    return Os::Delegate::makeDelegate<CpuInterface, Os::Zephyr::ZephyrCpu>(aligned_new_memory);
}
}
