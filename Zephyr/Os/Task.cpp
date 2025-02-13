// ======================================================================
// \title Os/Zephyr/Task.cpp
// \brief Zephyr implementations for Os::Task
// ======================================================================
#include "Zephyr/Os/Task.hpp"
#include <Fw/Types/Assert.hpp>
#include <Fw/Logger/Logger.hpp>
#include "zephyr/debug/thread_analyzer.h"
#include "zephyr/sys/kobject.h"
namespace Os {
namespace Zephyr {

//! Zephyr thread entry function expects three arguments. This is a glue that creates such function.
static void zephyrEntryWrapper(void* wrapper_pointer,  //!< Pointer to `Task::TaskRoutineWrapper`
                        void* p2,  //!< Unused
                        void* p3   //!< Unused
) {
    FW_ASSERT(wrapper_pointer != nullptr);
    Os::Task::TaskRoutineWrapper& wrapper = *reinterpret_cast<Os::Task::TaskRoutineWrapper*>(wrapper_pointer);
    Fw::Logger::log("Starting %p with %p %p\n", wrapper_pointer, p2, (void*)&wrapper.m_task);
    thread_analyzer_print(0);
    wrapper.run(p2);
}

ZephyrTask::~ZephyrTask() {
    if (this->m_handle.m_tid != nullptr) {
        k_object_release(this->m_handle.m_tid);
    }
}

void ZephyrTask::onStart() {}

Os::TaskInterface::Status ZephyrTask::join() {
    if (this->m_handle.m_tid == nullptr) {
        return Os::TaskInterface::Status::INVALID_HANDLE;
    }

    NATIVE_INT_TYPE stat = k_thread_join(this->m_handle.m_tid, K_MSEC(0));

    return (stat == 0) ? Os::TaskInterface::Status::OP_OK : Os::TaskInterface::Status::JOIN_ERROR;
}

void ZephyrTask::suspend(Os::TaskInterface::SuspensionType suspensionType) {}

void ZephyrTask::resume() {}

Os::TaskHandle* ZephyrTask::getHandle() {
    return &this->m_handle;
}

Os::TaskInterface::Status ZephyrTask::start(const Os::TaskInterface::Arguments& arguments) {
    FW_ASSERT(arguments.m_cpuAffinity == Task::TASK_DEFAULT);  // Zephyr SMP support is not implemented.
    FW_ASSERT(arguments.m_routine);

    k_thread_stack_t* stack = k_thread_stack_alloc(arguments.m_stackSize, 0);
    k_thread* thread = reinterpret_cast<k_thread*>(k_object_alloc(K_OBJ_THREAD));
    bool isValid = k_object_is_valid(thread, K_OBJ_THREAD);

    Fw::Logger::log("Allocating %d bytes for thread %s.\n\t Stack allocated to: %p\n\t Thread Allocated to: %p - isValid: %d\n",
                    arguments.m_stackSize, arguments.m_name.toChar(), stack, thread, isValid);
    Fw::Logger::log("\t Routine @ %p called with %p \n",
                    (void *)arguments.m_routine, (void *)arguments.m_routine_argument);
    if (thread == nullptr) {
        return Os::TaskInterface::Status::ERROR_RESOURCES;
    }

    FwSizeType priority = arguments.m_priority;
    // Zephyr priroties range from -16 to +14
    if (priority > 14) {
        Fw::Logger::log("Priority of %d exceeds maximum value. Setting to a priority value of 14.\n", priority);
        priority = 14;
    }

    k_tid_t tid = k_thread_create(thread, stack, arguments.m_stackSize, zephyrEntryWrapper, (void*)&arguments.m_routine,
                                  (void*)arguments.m_routine_argument, nullptr, priority, 0, K_NO_WAIT);
#ifdef CONFIG_THREAD_NAME
    NATIVE_INT_TYPE ret = k_thread_name_set(thread, arguments.m_name.toChar());
    FW_ASSERT(ret == 0, ret);
#endif
    if (tid == nullptr) {
        Fw::Logger::log("Failed to create %s at %p with stack %p (%d bytes) - %p\n",
                        arguments.m_name.toChar(), (void *)arguments.m_routine, stack, arguments.m_stackSize, thread);
        return Os::TaskInterface::Status::INVALID_HANDLE;
    }

    k_thread_start(tid);

    this->m_handle.m_tid = tid;

    return Os::TaskInterface::Status::OP_OK;
}

bool ZephyrTask::isCooperative() {
    return false;
}

Os::Task::Status ZephyrTask::_delay(Fw::TimeInterval interval) {
    FW_ASSERT(0);
    return Os::Task::Status::UNKNOWN_ERROR;
}

}  // namespace Zephyr
}  // namespace Os
