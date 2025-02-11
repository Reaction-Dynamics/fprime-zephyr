set(CMAKE_SYSTEM_NAME "Generic")
set(FPRIME_PLATFORM "Zephyr")
set(CONFIG_FORCE_NO_ASSERT true)
set(CMAKE_CROSSCOMPILING 1)

# Explicitly set Zephyr linker script
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${ZEPHYR_BASE}/boards/${BOARD}/zephyr/linker.ld")

# set(SYSCALL_LIST_H "${CMAKE_BINARY_DIR}/zephyr/include/generated/zephyr/syscall_list.h")
# set(PARSE_SYSCALLS_JSON "${CMAKE_BINARY_DIR}/misc/generated/syscalls.json")
# set(PARSE_SYSCALLS_TARGET parse_syscalls_target)
# set(syscalls_subdirs_trigger ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/syscalls_subdirs.trigger)

# # Step 3: Ensure `parse_syscalls.json` is created before F′ builds
# add_custom_target(generate_syscalls
#     COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PARSE_SYSCALLS_TARGET}
#     COMMENT "Ensuring Zephyr syscall parsing (parse_syscalls.json) completes before F′ builds."
# )

# # Add an explicit barrier between `subfolder_list.py` and `parse_syscalls.py`
# # add_dependencies(generate_syscalls syscalls_subdirs_trigger)

# # Step 4: Ensure `syscall_list.h` and `parse_syscalls.json` are both generated
# add_custom_target(wait_for_syscalls
#     COMMAND ${CMAKE_COMMAND} -E sleep 1
#     DEPENDS ${PARSE_SYSCALLS_JSON} ${SYSCALL_LIST_H}
# )

# # Include Generated Headers
# include_directories(
#     ${ZEPHYR_BASE}/include
#     ${CMAKE_BINARY_DIR}/zephyr/include/generated
# )
