// ======================================================================
// \title  ZephyrAsyncUartDriver.cpp
// \author reggiemarr
// \brief  cpp file for ZephyrAsyncUartDriver component implementation class
// ======================================================================

#include "fprime-zephyr/Drv/ZephyrAsyncUartDriver/ZephyrAsyncUartDriver.hpp"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Logger/Logger.hpp>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include "Drv/ByteStreamDriverModel/ByteStreamStatusEnumAc.hpp"
#include "Fw/Buffer/Buffer.hpp"
#include "Fw/Types/Assert.hpp"
#include "config/FwIndexTypeAliasAc.h"
#include "config/FwSizeTypeAliasAc.h"

#if !defined(CONFIG_UART_ASYNC_API)
#error "Async Uart driver requires CONFIG_UART_ASYNC_API=y"
#endif

namespace Zephyr {

ZephyrAsyncUartDriver::ZephyrAsyncUartDriver(const char* const compName)
    : ZephyrAsyncUartDriverComponentBase(compName), m_dev(nullptr), m_rxBuff(), m_pendingTxBuff() {}

ZephyrAsyncUartDriver::~ZephyrAsyncUartDriver() {}

void ZephyrAsyncUartDriver::uartEventCallback(const struct device* dev, struct uart_event* evt, void* user_data) {
    ZephyrAsyncUartDriver* driver = reinterpret_cast<ZephyrAsyncUartDriver*>(user_data);

    switch (evt->type) {
        case UART_RX_RDY: {
            // Data lives in the persistent rx buffer passed to uart_rx_enable.
            // Copy immediately into a freshly-allocated buffer we own and hand it down.
            const uint8_t* src = reinterpret_cast<const uint8_t*>(evt->data.rx.buf + evt->data.rx.offset);
            FwSizeType len = static_cast<FwSizeType>(evt->data.rx.len);

            if (len == 0) {
                Fw::Buffer empty;
                empty.set(nullptr, 0, Fw::Buffer::NO_CONTEXT);
                driver->recv_out(0, empty, Drv::ByteStreamStatus::RECV_NO_DATA);
                break;
            }

            // Allocate an outgoing buffer sized exactly for this chunk.
            Fw::Buffer outBuf = driver->allocate_out(0, len);
            FW_ASSERT(outBuf.isValid() && outBuf.getSize() >= len, outBuf.getSize());

            // Copy quickly to avoid concurrent DMA changes.
            void* dst = outBuf.getData();
            FW_ASSERT(dst != nullptr);
            memcpy(dst, src, len);

            // Prepare buffer metadata and pass ownership downstream.
            outBuf.setSize(len);
            driver->recv_out(0, outBuf, Drv::ByteStreamStatus::OP_OK);
        } break;
        case UART_RX_DISABLED: {
            // The UART RX path is disabled. We are the owner of the persistent RX
            // buffer: free it here. After this, the component will not receive RX
            // events until re-configured / re-enabled externally.
            if (driver->m_rxBuff.isValid()) {
                driver->deallocate_out(0, driver->m_rxBuff);
                // Mark invalid
                driver->m_rxBuff = Fw::Buffer();
            }
        } break;
        case UART_TX_ABORTED: {
            // TX aborted — notify the original caller with an error.
            if (driver->isConnected_drvAsyncSendReturnOut_OutputPort(0)) {
                driver->drvAsyncSendReturnOut_out(0, driver->m_pendingTxBuff, Drv::ByteStreamStatus::OTHER_ERROR);
            }
        } break;
        case UART_TX_DONE: {
            // Normal TX completion — notify success.
            if (driver->isConnected_drvAsyncSendReturnOut_OutputPort(0)) {
                driver->drvAsyncSendReturnOut_out(0, driver->m_pendingTxBuff, Drv::ByteStreamStatus::OP_OK);
            }
        } break;
        default:
            // Intentionally ignore other events.
            break;
    }
}

void ZephyrAsyncUartDriver::configure(const struct device* dev, U32 baud_rate, FwSizeType rx_buffer_size) {
    FW_ASSERT(dev != nullptr);
    this->m_dev = dev;

    FW_ASSERT(device_is_ready(this->m_dev));

    int status = uart_callback_set(this->m_dev, this->uartEventCallback, (void*)this);
    FW_ASSERT(status == 0, status);

    // Allocate a single persistent RX buffer and hand it to the Zephyr UART
    // driver. We will copy data out on RX_RDY; this persistent buffer is owned by
    // this component and will be freed on UART_RX_DISABLED.
    this->m_rxBuff = this->allocate_out(0, rx_buffer_size);
    FW_ASSERT(this->m_rxBuff.isValid() && this->m_rxBuff.getSize() >= rx_buffer_size, this->m_rxBuff.getSize());

    status = uart_rx_enable(this->m_dev, reinterpret_cast<uint8_t*>(this->m_rxBuff.getData()), this->m_rxBuff.getSize(),
                            SYS_FOREVER_US);
    FW_ASSERT(status == 0, status);

    if (this->isConnected_ready_OutputPort(0)) {
        this->ready_out(0);
    }
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined typed input ports
// ----------------------------------------------------------------------
void ZephyrAsyncUartDriver::asyncSend_handler(const FwIndexType portNum, Fw::Buffer& sendBuffer) {
    FW_ASSERT(this->isConnected_drvAsyncSendReturnOut_OutputPort(0));

    // Store pending tx to correlate in TX_DONE / TX_ABORTED event
    this->m_pendingTxBuff = sendBuffer;

    int status =
        uart_tx(this->m_dev, reinterpret_cast<uint8_t*>(sendBuffer.getData()), sendBuffer.getSize(), SYS_FOREVER_US);
    switch (status) {
        case (-EBUSY):
            this->drvAsyncSendReturnOut_out(0, sendBuffer, Drv::ByteStreamStatus::SEND_RETRY);
            break;
        default:
            this->drvAsyncSendReturnOut_out(0, sendBuffer, Drv::ByteStreamStatus::OTHER_ERROR);
            break;
        case 0:
            // Normal start: completion will be signalled via UART_TX_DONE
            break;
    }
}

Drv::ByteStreamStatus ZephyrAsyncUartDriver::send_handler(const FwIndexType portNum, Fw::Buffer& sendBuffer) {
    // This driver doesn't support synchronous send
    return Drv::ByteStreamStatus::OTHER_ERROR;
}

void ZephyrAsyncUartDriver::recvReturnIn_handler(const FwIndexType portNum, Fw::Buffer& returnBuffer) {
    // We allocate a fresh buffer on RX_RDY and hand ownership downstream; when
    // the downstream component returns the buffer to us via this handler, free it
    // here.
    if (returnBuffer.isValid()) {
        this->deallocate_out(0, returnBuffer);
    }
}

}  // end namespace Zephyr
