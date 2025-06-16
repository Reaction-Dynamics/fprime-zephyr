// ======================================================================
// \title  ZephyrAdcDriver.cpp
// \author user
// \brief  cpp file for ZephyrAdcDriver component implementation class
// ======================================================================

#include "ZephyrAdcDriver.hpp"
#include "Fw/Types/BasicTypes.h"
#include "config/FwIndexTypeAliasAc.h"
#include "config/FwSizeTypeAliasAc.h"
#include "fprime-zephyr/Drv/ZephyrAdcDriver/FppConstantsAc.hpp"
#include "fprime-zephyr/Drv/ZephyrAdcDriver/ZephyrAdcDriverComponentAc.hpp"
#include "zephyr/drivers/adc.h"
#include <Fw/Types/Assert.hpp>
#include <Fw/Logger/Logger.hpp>

namespace Zephyr {

    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------
    ZephyrAdcDriver::ZephyrAdcDriver(const char* const compName) :
        ZephyrAdcDriverComponentBase(compName),
        m_sampleBuffer(0),
        m_tlmUpdateCounter(0)
    {
        this->NUM_CHANNELS = DT_PROP_LEN(DT_PATH(zephyr_user), io_channels);

        // Initialize ADC sequence for single reads
        this->m_sequence.buffer = &m_sampleBuffer;
        this->m_sequence.buffer_size = sizeof(m_sampleBuffer);

        // Reset error counts
        for (FwIndexType i = 0; i < FW_NUM_ARRAY_ELEMENTS(this->m_errorCounts); i++) {
            this->m_errorCounts[i] = 0;
        }
    }
    const struct adc_dt_spec ZephyrAdcDriver::ADC_CHANNELS[] = {
        DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, ADC_FOREACH_DT_SPEC_AND_COMMA)
    };

    void ZephyrAdcDriver::init(FwSizeType queueDepth, FwEnumStoreType instance) {
        // Initialize each channel
        FwSizeType channelIdx = 0;
        Fw::Logger::log("Initializing %u %u %u %u channels %u\n", this->NUM_CHANNELS, ZephyrAdcDriver::NUM_CHANNELS, FW_NUM_ARRAY_ELEMENTS(this->ADC_CHANNELS), FW_NUM_ARRAY_ELEMENTS(ZephyrAdcDriver::ADC_CHANNELS), channelIdx);
        int st;
        while (channelIdx < this->NUM_CHANNELS) {
            Fw::Logger::log("Init channel id: %" PRI_BYTE " cfgId: %" PRI_BYTE " idx: %" PRI_FwSizeType " (%s)\n",
                            this->ADC_CHANNELS[channelIdx].channel_id, this->ADC_CHANNELS[channelIdx].channel_cfg.channel_id,
                            channelIdx, this->ADC_CHANNELS[channelIdx].dev->name);

            // Check if ADC device is ready
            if (adc_is_ready_dt(&this->ADC_CHANNELS[channelIdx]) == false) {
                break;
            }

            // Setup channel using device tree configuration
            st = adc_channel_setup_dt(&this->ADC_CHANNELS[channelIdx]);
            if (0 != st) {
                break;
            }

            channelIdx++;
        }
        FW_ASSERT(channelIdx == this->NUM_CHANNELS, st, channelIdx, this->NUM_CHANNELS);

        // Call parent's initializer afterwards
        ZephyrAdcDriverComponentBase::init(queueDepth, instance);
    }

    ZephyrAdcDriver::~ZephyrAdcDriver() {
        // Cleanup handled by Zephyr
    }

    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    void ZephyrAdcDriver::schedIn_handler(
        const FwIndexType portNum,
        U32 context
    ) {
        // Process all configured channels
        this->processAllChannels();

        // Update telemetry periodically
        // if ((++m_tlmUpdateCounter % 50) == 0) { // Every ~500ms
        //     this->tlmWrite_ADC_DRIVER_STATUS(Drv::AdcStatus::OP_OK);
        // }
    }

    void ZephyrAdcDriver::processAllChannels() {
        Os::ScopeLock lock(m_mutex);

        for (FwSizeType i = 0; i < this->NUM_CHANNELS; i++) {
            U32 rawValue;
            F32 voltageValue;
            bool status = this->readChannel(i, rawValue, voltageValue);
            if (!status) {
                this->m_errorCounts[i]++;
                this->log_WARNING_HI_ADC_READ_ERROR(
                    static_cast<U8>(i),
                    static_cast<I32>(status)
                );
                continue;
            }

            // Send data on async output port
            if (!this->isConnected_adcCountSample_OutputPort(i)) {
                continue;
            }

            Fw::Time timestamp;
            // this->getTime(timestamp);

            // Create ADC sample
            // Drv::AdcSample sample;
            // sample.setChannel(static_cast<U8>(i));
            // sample.setRawValue(rawValue);
            // sample.setVoltageValue(voltageValue);
            // sample.setTimestamp(timestamp);

            // this->adcCountSample_out(i, sample);

            // Update telemetry
            this->updateChannelTelemetry(i, rawValue, voltageValue);
        }
    }

    bool ZephyrAdcDriver::readChannel(FwIndexType channelIndex,
                                               U32& rawValue, F32& voltageValue) {
        // Initialize sequence for this specific channel
        (void)adc_sequence_init_dt(&this->ADC_CHANNELS[channelIndex], &this->m_sequence);

        // Read the channel
        int ret = adc_read_dt(&this->ADC_CHANNELS[channelIndex], &this->m_sequence);
        FW_ASSERT(ret == 0, ret, channelIndex);

        // Handle differential vs single-ended
        rawValue = this->ADC_CHANNELS[channelIndex].channel_cfg.differential ?
                   static_cast<U32>(static_cast<I16>(m_sampleBuffer)) :
                   static_cast<U32>(m_sampleBuffer);

        // Convert to voltage if requested
        // bool voltageMode;
        // // this->paramGet_ADC_VOLTAGE_MODE(voltageMode);

        // if (voltageMode) {
        //     I32 val_mv = static_cast<I32>(rawValue);
        //     ret = adc_raw_to_millivolts_dt(&this->ADC_CHANNELS[channelIndex], &val_mv);
        //     voltageValue = (ret < 0) ? 0.0f : static_cast<F32>(val_mv);
        // } else {
        //     voltageValue = static_cast<F32>(rawValue);
        // }
        voltageValue = static_cast<F32>(rawValue);

        return true;
    }

    void ZephyrAdcDriver::updateChannelTelemetry(FwIndexType channelIndex,
                                               U32 rawValue, F32 voltageValue) {
        // Update per-channel telemetry arrays
        // this->tlmWrite_ADC_RAW_COUNTS(channelIndex, rawValue);
        // this->tlmWrite_ADC_VOLTAGES_MV(channelIndex, voltageValue);
        // this->tlmWrite_ADC_ERROR_COUNT(channelIndex, m_errorCount[channelIndex]);
    }

    // ----------------------------------------------------------------------
    // Command handler implementations
    // ----------------------------------------------------------------------


    void ZephyrAdcDriver::ADC_READ_SINGLE_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        U8 channel
    ) {
        Os::ScopeLock lock(m_mutex);

        U32 rawValue;
        F32 voltageValue;
        Fw::Logger::log("Handling read request for channel %u\n", channel);
        bool status = this->readChannel(channel, rawValue, voltageValue);
        if (!status) {
            this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        // Send immediate result on output port
        if (this->isConnected_adcCountSample_OutputPort(channel)) {
            Fw::Time timestamp;
            // this->getTime(timestamp);

            // Drv::AdcSample sample;
            // sample.setChannel(channel);
            // sample.setRawValue(rawValue);
            // sample.setVoltageValue(voltageValue);
            // sample.setTimestamp(timestamp);

            // this->adcCountSample_out(channel, sample);
        }
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void ZephyrAdcDriver::ADC_CALIBRATE_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq
    ) {
        Os::ScopeLock lock(m_mutex);

        // Perform calibration read on all channels
        m_sequence.calibrate = true;

        bool calibrationOk = true;
        for (FwSizeType i = 0; i < ZephyrAdcDriver::NUM_CHANNELS; i++) {
            (void)adc_sequence_init_dt(&this->ADC_CHANNELS[i], &m_sequence);
            int ret = adc_read_dt(&this->ADC_CHANNELS[i], &m_sequence);
            if (ret < 0) {
                calibrationOk = false;
                break;
            }
        }

        m_sequence.calibrate = false; // Reset for normal operations

        // Drv::AdcStatus status = calibrationOk ? Drv::AdcStatus::OP_OK : Drv::AdcStatus::UNKNOWN_ERROR;
        // this->log_ACTIVITY_LOW_ADC_CALIBRATION_DONE(status);

        this->cmdResponse_out(opCode, cmdSeq,
                            calibrationOk ? Fw::CmdResponse::OK : Fw::CmdResponse::EXECUTION_ERROR);
    }

} // end namespace Zephyr
