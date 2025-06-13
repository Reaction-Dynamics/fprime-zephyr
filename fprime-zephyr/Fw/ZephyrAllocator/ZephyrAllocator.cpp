/**
 * \file
 * \author Reginald Marr
 * \brief Implementation of k_malloc based allocator
 *
 */

#include "ZephyrAllocator.hpp"
#include <zephyr/kernel.h>

namespace Fw {

ZephyrAllocator::ZephyrAllocator() {}

ZephyrAllocator::~ZephyrAllocator() {}

void* ZephyrAllocator::allocate(const NATIVE_UINT_TYPE identifier, NATIVE_UINT_TYPE& size, bool& recoverable) {
    recoverable = false;
    void* mem = k_malloc(size);
    if (nullptr == mem) {
        size = 0;  // set to zero if can't get memory
    }
    return mem;
}

void ZephyrAllocator::deallocate(const NATIVE_UINT_TYPE identifier, void* ptr) {
    k_free(ptr);
}

} /* namespace Fw */
