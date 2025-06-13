####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
#
# Note: using PROJECT_NAME as EXECUTABLE_NAME
####

# TODO this should probably become zephyr types or use the choose_fprime_implementation
set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/ZephyrAllocator.cpp"
)

register_fprime_module()
### UTs ###
# TODO
# set(UT_SOURCE_FILES
#   "${CMAKE_CURRENT_LIST_DIR}/test/ut/ExternalSerializeBufferTest.cpp"
#   "${CMAKE_CURRENT_LIST_DIR}/test/ut/TypesTest.cpp"
# )
# set(UT_MOD_DEPS
#   "${FPRIME_FRAMEWORK_PATH}/Os"
# )
# register_fprime_ut()

# Non-test directory
# add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/GTest")
