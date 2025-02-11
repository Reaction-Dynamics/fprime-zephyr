
# # Add FPrime OSAL Implementations
# choose_fprime_implementation(Os/File Os_File_Stub)

# # Add Baremetal OSAL Implementations
# choose_fprime_implementation(Os/Cpu Os_Cpu_Baremetal)
# choose_fprime_implementation(Os/Memory Os_Memory_Baremetal)

# # Add Zephyr OSAL Implementations
# choose_fprime_implementation(Os/Queue Os_Queue_Zephyr)
# choose_fprime_implementation(Os/Mutex Os_Mutex_Zephyr)
# choose_fprime_implementation(Os/Task Os_Task_Zephyr)
# choose_fprime_implementation(Os/Console Os_Console_Zephyr)
# choose_fprime_implementation(Os/RawTime Os_RawTime_Zephyr)

choose_fprime_implementation(Os/File Os/File/Stub)
choose_fprime_implementation(Os/Cpu Os/Cpu/Baremetal)
choose_fprime_implementation(Os/Memory Os/Memory/Baremetal)
choose_fprime_implementation(Os/Queue Os/Queue/Zephyr)
choose_fprime_implementation(Os/Mutex Os/Mutex/Zephyr)
choose_fprime_implementation(Os/Task Os/Task/Zephyr)
choose_fprime_implementation(Os/Console Os/Console/Zephyr)
choose_fprime_implementation(Os/RawTime Os/RawTime/Zephyr)

add_definitions(-DTGT_OS_TYPE_ZEPHYR)
# Zephyr compiler options
include_directories(
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>
)

include_directories(SYSTEM
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
)

add_compile_definitions(
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>
)

add_compile_options(
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>
    -fno-builtin
    -Wno-shadow -Wno-cast-align
    # -nostdinc
    # -ffreestanding
    # -lstdc++
    # -fno-use-cxa-atexit
    # -fno-strict-overflow
)

# add_compile_options(
#     # $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>
#     -fno-builtin
#     -Wno-shadow -Wno-cast-align
#     -nostdlib
#     -ffreestanding
#     -lstdc++
#     -fno-use-cxa-atexit
#     -fno-strict-overflow
#     -Wl,--gc-sections
#     -nodefaultlibs
#     -fno-exceptions
#     -fno-rtti

# )

# add_compile_options(
#     -fno-exceptions
#     -fno-rtti
#     -fno-builtin
# )
# add_link_options(
#     -nostdlib
#     -Wl,--gc-sections
#     # Use only one of libstdc++ or libm, not both
#     -nodefaultlibs
# )

# add_custom_target("${FPRIME_CURRENT_MODULE}")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-use-cxa-atexit -nostdlib -lstdc++ -lsupc++")
# set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")

include_directories(SYSTEM "${CMAKE_CURRENT_LIST_DIR}/types" "${CMAKE_CURRENT_LIST_DIR}")
