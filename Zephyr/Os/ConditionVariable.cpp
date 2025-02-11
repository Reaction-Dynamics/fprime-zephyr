// ======================================================================
// \title Os/Zephyr/ConditionVariable.cpp
// \brief Zephyr implementations for Os::ConditionVariable
// ======================================================================
#include "Zephyr/Os/ConditionVariable.hpp"
#include "Fw/Types/Assert.hpp"

namespace Os {
namespace Zephyr {

ZephyrConditionVariable::Status ZephyrConditionVariable::pend(Os::Mutex& mutex) {
    return ZephyrConditionVariable::Status::ERROR_NOT_IMPLEMENTED;
}
void ZephyrConditionVariable::notify() {
}
void ZephyrConditionVariable::notifyAll() {
}

ConditionVariableHandle* ZephyrConditionVariable::getHandle() {
    return &m_handle;
}

}  // namespace Zephyr
}  // namespace Os
