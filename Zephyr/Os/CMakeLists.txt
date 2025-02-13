
add_custom_target("${FPRIME_CURRENT_MODULE}")

set(SOURCE_FILES
    "${CMAKE_CURRENT_LIST_DIR}/error.cpp"
)
set(HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/error.hpp"
)


register_fprime_module(Os_Zephyr_Shared)
set(MOD_DEPS config)

register_os_implementation("Mutex;ConditionVariable" Zephyr kernel Os_Zephyr_Shared)
register_os_implementation(RawTime Zephyr kernel Os_Zephyr_Shared)
register_os_implementation(Queue Zephyr kernel Os_Zephyr_Shared)
register_os_implementation(Console Zephyr kernel Os_Zephyr_Shared)
register_os_implementation(Task Zephyr kernel Os_Zephyr_Shared)
register_os_implementation("Cpu" Zephyr)
register_os_implementation("Memory" Zephyr)

# target_link_libraries(Os_Zephyr_Shared PRIVATE ZephyrKernel)

# register_os_implementation("Mutex" Zephyr kernel Os_Zephyr_Shared)
# register_os_implementation("ConditionVariable" Zephyr kernel Os_Zephyr_Shared)
# register_os_implementation("Task" Zephyr kernel Os_Zephyr_Shared Fw_Types)
# register_os_implementation("Queue" Zephyr kernel Os_Zephyr_Shared)
# register_os_implementation("Console" Zephyr kernel Os_Zephyr_Shared)
# register_os_implementation("RawTime" Zephyr kernel Os_Zephyr_Shared)
