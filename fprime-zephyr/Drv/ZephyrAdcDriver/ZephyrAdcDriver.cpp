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
        m_samplingEnabled(false),
        m_initialized(false),
        m_sampleBuffer(0),
        m_tlmUpdateCounter(0)
    {
        this->NUM_CHANNELS = DT_PROP_LEN(DT_PATH(zephyr_user), io_channels);

        // Initialize arrays
        for (FwSizeType i = 0; i < this->NUM_CHANNELS; i++) {
            this->m_sampleCount[i] = 0;
            this->m_errorCount[i] = 0;
            this->m_sampleInterval[i] = 100; // Default to 10Hz (100 * 10ms = 1s)
            // this->ADC_CHANNELS[i] = adcSpecs[i];
        }

        // Initialize ADC sequence for single reads
        this->m_sequence.buffer = &m_sampleBuffer;
        this->m_sequence.buffer_size = sizeof(m_sampleBuffer);
        this->m_sequence.resolution = 12; // Common resolution
        this->m_sequence.oversampling = 0;
        this->m_sequence.calibrate = false;
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
        // Only process if initialized and enabled
        if (!m_initialized || !m_samplingEnabled) {
            return;
        }

        // Load parameters periodically
        // if ((context % 100) == 0) { // Every ~1 second at 10ms rate
        //     this->parameterUpdated(this->PARAMID_ADC_SAMPLING_ENABLED);
        //     this->parameterUpdated(this->PARAMID_ADC_SAMPLE_RATES);
        // }

        // Process all configured channels
        this->processAllChannels();

        // Update telemetry periodically
        // if ((++m_tlmUpdateCounter % 50) == 0) { // Every ~500ms
        //     this->tlmWrite_ADC_DRIVER_STATUS(Drv::AdcStatus::OP_OK);
        // }
    }

    void ZephyrAdcDriver::processAllChannels() {
        Os::ScopeLock lock(m_mutex);

        for (FwSizeType i = 0; i < ZephyrAdcDriver::NUM_CHANNELS; i++) {
            U32 rawValue;
            F32 voltageValue;
            bool status = readChannel(i, rawValue, voltageValue);
            if (!status) {
                m_errorCount[i]++;
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
            updateChannelTelemetry(i, rawValue, voltageValue);

            this->m_sampleCount[i]++;
        }
    }

    bool ZephyrAdcDriver::readChannel(FwIndexType channelIndex,
                                               U32& rawValue, F32& voltageValue) {
        // Initialize sequence for this specific channel
        (void)adc_sequence_init_dt(&ADC_CHANNELS[channelIndex], &this->m_sequence);


        // Read the channel
        int ret = adc_read_dt(&ADC_CHANNELS[channelIndex], &this->m_sequence);
        if (ret < 0) {
            return false;
        }

        // Handle differential vs single-ended
        rawValue = ADC_CHANNELS[channelIndex].channel_cfg.differential ?
                   static_cast<U32>(static_cast<I16>(m_sampleBuffer)) :
                   static_cast<U32>(m_sampleBuffer);

        // Convert to voltage if requested
        bool voltageMode;
        // this->paramGet_ADC_VOLTAGE_MODE(voltageMode);

        if (voltageMode) {
            I32 val_mv = static_cast<I32>(rawValue);
            ret = adc_raw_to_millivolts_dt(&ADC_CHANNELS[channelIndex], &val_mv);
            voltageValue = (ret < 0) ? 0.0f : static_cast<F32>(val_mv);
        } else {
            voltageValue = static_cast<F32>(rawValue);
        }

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

    void ZephyrAdcDriver::ADC_ENABLE_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        bool enable
    ) {
        Os::ScopeLock lock(m_mutex);

        m_samplingEnabled = enable;
        // this->log_ACTIVITY_LOW_ADC_SAMPLING_CHANGED(enable);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void ZephyrAdcDriver::ADC_SET_RATE_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        U8 channel,
        F32 rate_hz
    ) {
        Os::ScopeLock lock(m_mutex);

        // Convert Hz to scheduler interval (assuming 10ms scheduler)
        if (rate_hz > 0.0f) {
            m_sampleInterval[channel] = static_cast<U32>(100.0f / rate_hz); // 100 = 1Hz at 10ms
            m_sampleInterval[channel] = FW_MAX(m_sampleInterval[channel], 1U); // Min 1 tick
        } else {
            m_sampleInterval[channel] = 0xFFFFFFFF; // Effectively disabled
        }

        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void ZephyrAdcDriver::ADC_READ_SINGLE_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        U8 channel
    ) {
        Os::ScopeLock lock(m_mutex);

        U32 rawValue;
        F32 voltageValue;
        bool status = readChannel(channel, rawValue, voltageValue);
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
