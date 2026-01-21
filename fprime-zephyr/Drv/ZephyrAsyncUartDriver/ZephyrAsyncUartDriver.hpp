// ======================================================================
// \title  ZephyrAsyncUartDriver.hpp
// \author reggiemarr
// \brief  hpp file for ZephyrAsyncUartDriver component implementation class
// ======================================================================

#ifndef ZephyrAsyncUartDriver_HPP
#define ZephyrAsyncUartDriver_HPP

#include "Fw/Buffer/Buffer.hpp"
#include "config/FwIndexTypeAliasAc.h"
#include "config/FwSizeTypeAliasAc.h"
#include "fprime-zephyr/Drv/ZephyrAsyncUartDriver/ZephyrAsyncUartDriverComponentAc.hpp"

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <array>
#include <atomic>

namespace Zephyr {

class ZephyrAsyncUartDriver : public ZephyrAsyncUartDriverComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    //! Construct object ZephyrAsyncUartDriver
    //!
    ZephyrAsyncUartDriver(const char* const compName /*!< The component name*/
    );

    //! Destroy object ZephyrAsyncUartDriver
    //!
    ~ZephyrAsyncUartDriver();

    void configure(const struct device* dev, U32 baud_rate, FwSizeType rx_buffer_size);

  public:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for send
    //!
    void asyncSend_handler(const FwIndexType portNum,  //!< The port number
                           Fw::Buffer& sendBuffer      //!< The buffer to send through UART
    );

    // Handler implementation for send
    //!
    Drv::ByteStreamStatus send_handler(const FwIndexType portNum,  //!< The port number
                                       Fw::Buffer& sendBuffer      //!< The buffer to send through UART
    );

    void recvReturnIn_handler(const FwIndexType portNum,  //!< The port number
                              Fw::Buffer& returnBuffer    //!< The buffer being returned to its owner
    );

  private:
    void static uartEventCallback(const struct device* dev, struct uart_event* evt, void* user_data);
    const struct device* m_dev;
    Fw::Buffer m_rxBuff;
    Fw::Buffer m_pendingTxBuff;
};

}  // end namespace Zephyr

#endif
