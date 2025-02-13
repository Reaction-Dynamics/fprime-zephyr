// ======================================================================
// \title Os/Zephyr/Queue.cpp
// \brief Zephyr implementation for Os::Queue
// ======================================================================
#include "Zephyr/Os/Queue.hpp"
#include <Fw/Types/Assert.hpp>
#include <cstdint>
#include <cstring>
#include "Fw/Logger/Logger.hpp"
#include "syscalls/kernel.h"
#include "zephyr/debug/thread_analyzer.h"
#include "zephyr/kernel.h"

namespace Os {
namespace Zephyr {

ZephyrQueue::~ZephyrQueue() {
    FW_ASSERT(this->m_handle.m_msgq != nullptr);

    NATIVE_INT_TYPE ret = k_msgq_cleanup(this->m_handle.m_msgq);
    FW_ASSERT(ret == 0, ret);
    k_free(this->m_handle.m_msgq);
}

QueueInterface::Status ZephyrQueue::create(const Fw::StringBase& name, FwSizeType depth, FwSizeType messageSize) {
    if (messageSize > static_cast<NATIVE_INT_TYPE>(ZEPHYR_Q_MAX_USER_MSG_SIZE)) {
        return QueueInterface::Status::SIZE_MISMATCH;
    }

    struct k_msgq* msgq = reinterpret_cast<struct k_msgq*>(k_malloc(sizeof(struct k_msgq)));
    if (msgq == nullptr) {
        return QueueInterface::Status::UNINITIALIZED;
    }

    FW_ASSERT(messageSize + ZEPHYR_Q_RESERVED <= ZEPHYR_Q_MAX_MSG_SIZE, messageSize);
    NATIVE_INT_TYPE ret = k_msgq_alloc_init(msgq, messageSize + ZEPHYR_Q_RESERVED, depth);
    if (ret != 0) {
        return QueueInterface::Status::UNINITIALIZED;
    }

    this->m_handle.m_msgq = msgq;

    return QueueInterface::Status::OP_OK;
}

QueueInterface::Status ZephyrQueue::send(const U8* buffer,
                                         FwSizeType size,
                                         FwQueuePriorityType priority,
                                         QueueInterface::BlockingType blockType) {
    if (this->m_handle.m_msgq == nullptr) {
        return QueueInterface::Status::UNINITIALIZED;
    }

    U8 tmp_buf[ZEPHYR_Q_MAX_MSG_SIZE];
    *reinterpret_cast<U16*>(tmp_buf) = static_cast<U16>(size);
    memcpy(&tmp_buf[ZEPHYR_Q_RESERVED], buffer, size);
    // thread_analyzer_print(0);
    struct k_msgq_attrs msgqAttrs;
    uint32_t numUsed, numFree;
    numFree = k_msgq_num_free_get(this->m_handle.m_msgq);
    numUsed = k_msgq_num_used_get(this->m_handle.m_msgq);
    k_msgq_get_attrs(this->m_handle.m_msgq, &msgqAttrs);
    Fw::Logger::log("Free/Used %d/%d  %d size %d max %d\n", numFree, numUsed, msgqAttrs.used_msgs, msgqAttrs.msg_size, msgqAttrs.max_msgs);

    NATIVE_INT_TYPE ret = k_msgq_put(this->m_handle.m_msgq, tmp_buf,
                                     (blockType == QueueInterface::BlockingType::BLOCKING) ? K_FOREVER : K_NO_WAIT);

    if (ret != 0) {
        switch (blockType) {
            case QueueInterface::BlockingType::BLOCKING:
                FW_ASSERT(0, ret);  // Nonzero return shall not occur for blocking send
                return QueueInterface::Status::SEND_ERROR;
            case QueueInterface::BlockingType::NONBLOCKING:
                FW_ASSERT(ret != -EAGAIN, ret);  // EAGAIN means timeout expired, shall not occur
                return QueueInterface::Status::FULL;
            default:
                FW_ASSERT(0, blockType);
                return QueueInterface::Status::SEND_ERROR;
        }
    }

    return QueueInterface::Status::OP_OK;
}

QueueInterface::Status ZephyrQueue::receive(U8* destination,
                                            FwSizeType capacity,
                                            QueueInterface::BlockingType blockType,
                                            FwSizeType& actualSize,
                                            FwQueuePriorityType& priority) {
    if (this->m_handle.m_msgq == nullptr) {
        return QueueInterface::Status::UNINITIALIZED;
    }

    // Priority is not supported in Zephyr
    priority = 0;

    U8 tmp_buf[ZEPHYR_Q_MAX_MSG_SIZE];
    NATIVE_INT_TYPE ret = k_msgq_get(this->m_handle.m_msgq, tmp_buf,
                                     (blockType == QueueInterface::BlockingType::BLOCKING) ? K_FOREVER : K_NO_WAIT);

    if (ret != 0) {
        switch (blockType) {
            case QueueInterface::BlockingType::BLOCKING:
                FW_ASSERT(0, ret);  // Nonzero return shall not occur for blocking receive
                return QueueInterface::Status::RECEIVE_ERROR;
            case QueueInterface::BlockingType::NONBLOCKING:
                FW_ASSERT(ret != -EAGAIN, ret);  // EAGAIN means timeout expired, shall not occur
                return QueueInterface::Status::EMPTY;
            default:
                FW_ASSERT(0, blockType);
                return QueueInterface::Status::RECEIVE_ERROR;
        }
    }

    actualSize = *reinterpret_cast<U16*>(tmp_buf);
    memcpy(destination, &tmp_buf[ZEPHYR_Q_RESERVED], actualSize);

    return QueueInterface::Status::OP_OK;
}

FwSizeType ZephyrQueue::getMessagesAvailable() const {
    FW_ASSERT(this->m_handle.m_msgq != nullptr);

    return static_cast<FwSizeType>(this->m_handle.m_msgq->used_msgs);
}

FwSizeType ZephyrQueue::getMessageHighWaterMark() const {
    return 0;
}

QueueHandle* ZephyrQueue::getHandle() {
    return &this->m_handle;
}

}  // namespace Zephyr
}  // namespace Os
