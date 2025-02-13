// ======================================================================
// \title Os/Zephyr/DefaultMutex.cpp
// \brief sets default Os::Mutex to no-op Zephyr implementation via linker
// ======================================================================
#include "Zephyr/Os/ConditionVariable.hpp"
#include "Os/Delegate.hpp"
namespace Os {

//! \brief get a delegate for condition variable
//! \param aligned_new_memory: aligned memory to fill
//! \return: pointer to delegate
ConditionVariableInterface *ConditionVariableInterface::getDelegate(ConditionVariableHandleStorage& aligned_new_memory) {
    return Os::Delegate::makeDelegate<ConditionVariableInterface, Os::Zephyr::ZephyrConditionVariable, ConditionVariableHandleStorage>(
        aligned_new_memory
    );
}
}
