// ======================================================================
// \title  ZephyrAsyncUartDriver.cpp
// \author reggiemarr
// \brief  cpp file for ZephyrAsyncUartDriver component implementation class
// ======================================================================

#include "fprime-zephyr/Drv/ZephyrAsyncUartDriver/ZephyrAsyncUartDriver.hpp"
#include "Drv/ByteStreamDriverModel/ByteStreamStatusEnumAc.hpp"
#include "Fw/Buffer/Buffer.hpp"
#include "Fw/Types/Assert.hpp"
#include "config/FwIndexTypeAliasAc.h"
#include "fprime-zephyr/Drv/ZephyrAsyncUartDriver/ZephyrUartStopReasonEnumAc.hpp"
#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Logger/Logger.hpp>
#include <cerrno>
#include <cstdint>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#if !defined(CONFIG_UART_ASYNC_API)
#error "Async Uart driver requires CONFIG_UART_ASYNC_API=y"
#endif

namespace Zephyr {

void ZephyrAsyncUartDriver::uartEventCallback(const struct device *dev,
                                              struct uart_event *evt,
                                              void *user_data) {
  ZephyrAsyncUartDriver *driver =
      reinterpret_cast<ZephyrAsyncUartDriver *>(user_data);
  I32 rc;
  Fw::Buffer uartBuff;
  ZephyrUartStopReason stopReason(
      static_cast<ZephyrUartStopReason::T>(evt->data.rx_stop.reason));

  switch (evt->type) {
  case UART_RX_RDY:
    uartBuff.set(reinterpret_cast<U8 *>(evt->data.rx.buf + evt->data.rx.offset),
                 evt->data.rx.len, Fw::Buffer::NO_CONTEXT);
    // NOTE this is expected to block until the buffer is passed down stream and
    // the ownership is passed back to us Currently this will work since it
    // passes through a series of sync ports but we should leverage a semaphore
    // here
    driver->recv_out(0, uartBuff,
                     evt->data.rx.len > 0
                         ? Drv::ByteStreamStatus::OP_OK
                         : Drv::ByteStreamStatus::RECV_NO_DATA);
    break;
  case UART_RX_DISABLED:
    uartBuff.set(reinterpret_cast<U8 *>(evt->data.rx_buf.buf), evt->data.rx.len,
                 driver->m_rxBuffContext);
    driver->deallocate_out(0, uartBuff);
    break;

  case UART_TX_DONE:
    // Return TX buffer to its owner
    driver->drvAsyncSendReturnOut_out(0, driver->m_pendingTxBuff,
                                      Drv::ByteStreamStatus::OP_OK);
    break;

  default:
    // Ignore other events (UART_RX_BUF_RELEASED handled above).
    break;
  }
}

ZephyrAsyncUartDriver::ZephyrAsyncUartDriver(const char *const compName)
    : ZephyrAsyncUartDriverComponentBase(compName) {
  // Initialize tracked pointer to null
  this->m_rxBuffPtr = nullptr;
}

ZephyrAsyncUartDriver::~ZephyrAsyncUartDriver() {}

void ZephyrAsyncUartDriver::configure(const struct device *dev, U32 baud_rate) {
  FW_ASSERT(dev != nullptr);
  this->m_dev = dev;

  FW_ASSERT(device_is_ready(this->m_dev));

  I32 ret =
      uart_callback_set(this->m_dev, this->uartEventCallback, (void *)this);
  FW_ASSERT(ret == 0, ret);

  FW_ASSERT(ret == 0, ret);
  this->m_rxWorkContexts.at(0).pendingBuff =
      this->allocate_out(0, RX_ACCUMULATE_SIZE);
  FW_ASSERT(this->m_rxWorkContexts.at(0).pendingBuff.isValid() &&
                this->m_rxWorkContexts.at(0).pendingBuff.getSize() >=
                    RX_ACCUMULATE_SIZE,
            this->m_rxWorkContexts.at(0).pendingBuff.getSize());
  this->m_rxBuffContext = this->m_rxWorkContexts.at(0).pendingBuff.getContext();
  ret = uart_rx_enable(this->m_dev,
                       reinterpret_cast<uint8_t *>(
                           this->m_rxWorkContexts.at(0).pendingBuff.getData()),
                       this->m_rxWorkContexts.at(0).pendingBuff.getSize(),
                       SYS_FOREVER_US);

  if (this->isConnected_ready_OutputPort(0)) {
    this->ready_out(0);
  }
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined typed input ports
// ----------------------------------------------------------------------
void ZephyrAsyncUartDriver::asyncSend_handler(const FwIndexType portNum,
                                              Fw::Buffer &sendBuffer) {
  FW_ASSERT(this->isConnected_drvAsyncSendReturnOut_OutputPort(0));

  // Store pending tx to correlate in TX_DONE event
  this->m_pendingTxBuff = sendBuffer;

  int status =
      uart_tx(this->m_dev, reinterpret_cast<uint8_t *>(sendBuffer.getData()),
              sendBuffer.getSize(), SYS_FOREVER_US);
  switch (status) {
  case (-EBUSY):
    this->drvAsyncSendReturnOut_out(0, sendBuffer,
                                    Drv::ByteStreamStatus::SEND_RETRY);
    break;
  default:
    this->drvAsyncSendReturnOut_out(0, sendBuffer,
                                    Drv::ByteStreamStatus::OTHER_ERROR);
    break;
  case 0:
    // Normal start: actual completion will be signalled via UART_TX_DONE
    break;
  }
}

Drv::ByteStreamStatus
ZephyrAsyncUartDriver::send_handler(const FwIndexType portNum,
                                    Fw::Buffer &sendBuffer) {
  // This driver doesn't support synchronous send
  return Drv::ByteStreamStatus::OTHER_ERROR;
}

void ZephyrAsyncUartDriver::recvReturnIn_handler(const FwIndexType portNum,
                                                 Fw::Buffer &returnBuffer) {
  // NOTE: For ring-buffer DMA drivers the buffer pointers seen in RX_RDY are
  // driver-owned. W
  // matches dwibuffer. Therefore this method remains a NO-OP.
}

} // end namespace Zephyr
