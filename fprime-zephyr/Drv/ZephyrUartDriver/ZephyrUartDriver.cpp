// ======================================================================
// \title  ZephyrUartDriver.cpp
// \author ethanchee
// \brief  cpp file for ZephyrUartDriver component implementation class
// ======================================================================


#include "fprime-zephyr/Drv/ZephyrUartDriver/ZephyrUartDriver.hpp"
#include "Fw/Types/BasicTypes.hpp"
#include "Fw/Types/Assert.hpp"
#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Logger/Logger.hpp>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>


namespace Zephyr {

    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    ZephyrUartDriver ::
        ZephyrUartDriver(
            const char *const compName
        ) : ZephyrUartDriverComponentBase(compName)
    {
    }

    ZephyrUartDriver ::
        ~ZephyrUartDriver()
    {

    }

    void ZephyrUartDriver::configure(const struct device *dev, U32 baud_rate) {
        FW_ASSERT(dev != nullptr);
        m_dev = dev;

        if (!device_is_ready(this->m_dev)) {
            return;
        }

        struct uart_config uart_cfg = {
            .baudrate = baud_rate,
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits = UART_CFG_STOP_BITS_1,
            .data_bits = UART_CFG_DATA_BITS_8,
            .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
        };
        uart_configure(this->m_dev, &uart_cfg);

        ring_buf_init(&this->m_ring_buf, RING_BUF_SIZE, this->m_ring_buf_data);

        this->setup_async_rx();

        if (this->isConnected_ready_OutputPort(0)) {
            this->ready_out(0);
        }
    }

#if defined(CONFIG_UART_ASYNC_API)
    void ZephyrUartDriver::setup_async_rx()
    {
        I32 ret = uart_callback_set(this->m_dev, this->serial_cb, (void *)this);  // Pass 'this' instead of uart_dev
        FW_ASSERT(ret == 0, ret);
        ret = uart_rx_enable(this->m_dev, this->async_rx_buffer[0], sizeof(this->async_rx_buffer[0]), SYS_FOREVER_US);
        FW_ASSERT(ret == 0, ret);
        // uart_tx_disable(this->m_dev);
        this->async_rx_buffer_idx = 1;  // Next buffer to provide
    }

    void ZephyrUartDriver::serial_cb(const struct device *dev, struct uart_event *evt, void *user_data)
    {
        ZephyrUartDriver *driver = reinterpret_cast<ZephyrUartDriver *>(user_data);
        I32 rc;

        // LOG_DBG("EVENT: %d", evt->type);
        switch (evt->type) {
        case UART_RX_BUF_REQUEST:
            /* Provide the next RX buffer */
            // LOG_DBG("Providing buffer index %d", driver->async_rx_buffer_idx);
            rc = uart_rx_buf_rsp(dev,
                            driver->async_rx_buffer[driver->async_rx_buffer_idx],
                            sizeof(driver->async_rx_buffer[0]));
            if (rc == 0) {
                driver->async_rx_buffer_idx = driver->async_rx_buffer_idx ? 0 : 1;
            } else {
                Fw::Logger::log("Failed to provide RX buffer (%d)", rc);
            }
            break;

        case UART_RX_RDY:
            {
                // LOG_HEXDUMP_INF(evt->data.rx.buf + evt->data.rx.offset,
                //             evt->data.rx.len, "RX_RDY");

                /* Put received data into ring buffer */
                U32 bytes_written = ring_buf_put(&driver->m_ring_buf,
                                                evt->data.rx.buf + evt->data.rx.offset,
                                                evt->data.rx.len);

                if (bytes_written != evt->data.rx.len) {
                    Fw::Logger::log("Ring buffer full, dropped %d bytes",
                        evt->data.rx.len - bytes_written);
                }

                /* Trigger processing of received data */
                if (bytes_written > 0) {
                    // You might want to trigger your schedIn_handler here
                    // or set a flag to process data in your main loop
                }
            }
            break;
        case UART_TX_DONE:
            break;

        case UART_RX_BUF_RELEASED:
            // LOG_DBG("RX buffer released: %p", evt->data.rx_buf.buf);
            break;

        case UART_RX_DISABLED:
            // LOG_DBG("RX disabled");
            break;

        default:
            // LOG_WRN("Unhandled UART event %d", evt->type);
            break;
        }
    }
#elif defined(CONFIG_UART_INTERRUPT_DRIVEN)
    void ZephyrUartDriver::setup_async_rx()
    {
        uart_irq_callback_user_data_set(this->m_dev, this->serial_cb, &this->m_ring_buf);
        uart_irq_rx_enable(this->m_dev);
	    uart_irq_tx_disable(this->m_dev);
    }

    void ZephyrUartDriver::serial_cb(const struct device *dev, void *user_data)
    {
        struct ring_buf *ring_buf = reinterpret_cast<struct ring_buf *>(user_data);

        if (!uart_irq_update(dev)) {
            return;
        }

        if (!uart_irq_rx_ready(dev)) {
            return;
        }

        U8 c;
        // TODO: Get rid of the endless loop (in an IRQ handler!).
        while (uart_fifo_read(dev, &c, 1) == 1) {
            if (ring_buf_put(ring_buf, &c, 1) != 1) {
                // TODO: Handle properly.
                printk("UART buffer overrun\n");
            }
        }
    }
#else
#error "Cannot build ZephyrUartDriver without an rx mechanism"
#endif


    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    void ZephyrUartDriver ::
        schedIn_handler(
            const FwIndexType portNum,
            U32 context
        )
    {
        Fw::Buffer recv_buffer = this->allocate_out(0, SERIAL_BUFFER_SIZE);

        U32 recv_size = ring_buf_get(&this->m_ring_buf, recv_buffer.getData(), recv_buffer.getSize());
        if (recv_size > 0) {
            recv_buffer.setSize(recv_size);
            recv_out(0, recv_buffer, Drv::ByteStreamStatus::OP_OK);
        } else {
            // No data available, return the buffer
            this->deallocate_out(0, recv_buffer);
        }
    }

    Drv::ByteStreamStatus ZephyrUartDriver ::
        send_handler(
            const FwIndexType portNum,
            Fw::Buffer &sendBuffer
        )
    {
        for (U32 i = 0; i < sendBuffer.getSize(); i++) {
            uart_poll_out(this->m_dev, sendBuffer.getData()[i]);
        }
        return Drv::ByteStreamStatus::OP_OK;
    }

    void ZephyrUartDriver ::recvReturnIn_handler(const FwIndexType portNum, Fw::Buffer &returnBuffer) {
        this->deallocate_out(0, returnBuffer);
    }

} // end namespace Zephyr
