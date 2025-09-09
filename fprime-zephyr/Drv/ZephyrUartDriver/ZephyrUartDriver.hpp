// ======================================================================
// \title  ZephyrUartDriver.hpp
// \author ethanchee
// \brief  hpp file for ZephyrUartDriver component implementation class
// ======================================================================

#ifndef ZephyrUartDriver_HPP
#define ZephyrUartDriver_HPP

#include "config/FwIndexTypeAliasAc.h"
#include "fprime-zephyr/Drv/ZephyrUartDriver/ZephyrUartDriverComponentAc.hpp"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#define RING_BUF_SIZE 1024

namespace Zephyr {

  class ZephyrUartDriver :
    public ZephyrUartDriverComponentBase
  {

    const FwSizeType SERIAL_BUFFER_SIZE = 64;

    public:

        // ----------------------------------------------------------------------
        // Construction, initialization, and destruction
        // ----------------------------------------------------------------------

        //! Construct object ZephyrUartDriver
        //!
        ZephyrUartDriver(
            const char *const compName /*!< The component name*/
        );

        //! Destroy object ZephyrUartDriver
        //!
        ~ZephyrUartDriver();

        void configure(const struct device *dev, U32 baud_rate);

    public:

#if defined(CONFIG_UART_ASYNC_API)
        // Async RX buffers (double buffering) for dma
        U8 async_rx_buffer[2][256];
        FwIndexType async_rx_buffer_idx;
        static void serial_cb(const struct device *dev, struct uart_event *evt, void *user_data);
#elif defined(CONFIG_UART_INTERRUPT_DRIVEN)
        static void serial_cb(const struct device *dev, void *user_data);
#else
#error "Cannot build ZephyrUartDriver without an rx mechanism"
#endif

        void setup_async_rx();

        // ----------------------------------------------------------------------
        // Handler implementations for user-defined typed input ports
        // ----------------------------------------------------------------------

        //! Handler implementation for schedIn
        //!
        void schedIn_handler(
            const FwIndexType portNum, /*!< The port number*/
            U32 context /*!< 
        The call order
        */
        );


        //! Handler implementation for send
        //!
        Drv::ByteStreamStatus send_handler(
            const FwIndexType portNum, /*!< The port number*/
            Fw::Buffer &sendBuffer 
        );

        void recvReturnIn_handler(
            const FwIndexType portNum, /*!< The port number*/
            Fw::Buffer &returnBuffer
        );

        const struct device *m_dev;

        U8 m_ring_buf_data[RING_BUF_SIZE];
        struct ring_buf m_ring_buf;
    };

} // end namespace Zephyr

#endif
