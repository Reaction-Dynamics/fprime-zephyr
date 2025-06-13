// ======================================================================
// \title  ZephyrAdcDriver.cpp
// \author user
// \brief  cpp file for ZephyrAdcDriver component implementation class
// ======================================================================

#include "ZephyrAdcDriver.hpp"
#include "config/FwIndexTypeAliasAc.h"
#include "config/FwSizeTypeAliasAc.h"
#include "fprime-zephyr/Drv/ZephyrAdcDriver/ZephyrAdcDriverComponentAc.hpp"
#include <Fw/Types/Assert.hpp>
#include <Fw/Logger/Logger.hpp>

// Generate ADC channel array from device tree
static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};

// Extract channel names from device tree
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channel_names)
#define CHANNEL_NAME(node_id, prop, idx) \
    DT_PROP_BY_IDX(node_id, prop, idx),
static const char* const channel_names[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channel_names, CHANNEL_NAME)
};
#else
// Generate default names if not specified
// static const char* const channel_names[] = {
//     "ch0", "ch1", "ch2", "ch3", "ch4", "ch5",
//     "ch6", "ch7", "ch8", "ch9", "ch10", "ch11"
// };
#error "Must have channel for device"
#endif

namespace Zephyr {

    // Static member initialization
    const struct adc_dt_spec ZephyrAdcDriver::m_adcChannels[] = {
        DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
    };

    const FwSizeType ZephyrAdcDriver::m_numChannels = ARRAY_SIZE(adc_channels);

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
        // Initialize arrays
        for (FwIndexType i = 0; i < ADC_MAX_CHANNELS; i++) {
            this->m_sampleCount[i] = 0;
            this->m_errorCount[i] = 0;
            this->m_sampleInterval[i] = 100; // Default to 10Hz (100 * 10ms = 1s)
        }

        // Initialize ADC sequence for single reads
        this->m_sequence.buffer = &m_sampleBuffer;
        this->m_sequence.buffer_size = sizeof(m_sampleBuffer);
        this->m_sequence.resolution = 12; // Common resolution
        this->m_sequence.oversampling = 0;
        this->m_sequence.calibrate = false;
    }

    ZephyrAdcDriver::~ZephyrAdcDriver() {
        // Cleanup handled by Zephyr
    }

    void ZephyrAdcDriver::init(FwSizeType queueDepth, FwEnumStoreType instance) {
        FW_ASSERT(this->m_numChannels > ADC_MAX_CHANNELS);

        // Initialize each channel
        FwIndexType channelIdx = 0;
        for (channelIdx = 0; channelIdx < this->m_numChannels; channelIdx++) {
            // Check if ADC device is ready
            if (!adc_is_ready_dt(&this->m_adcChannels[channelIdx])) {
                break;
            }

            // Setup channel using device tree configuration
            if (0 != adc_channel_setup_dt(&this->m_adcChannels[channelIdx])) {
                break;
            }

            // Fw::Logger::log("ZephyrAdcDriver: Initialized channel %d (%s)\n",
            //                  channelIdx, m_channelNames[channelIdx]);
        }
        FW_ASSERT(channelIdx >= this->m_numChannels);

        // Call parent's initializer afterwards
        ZephyrAdcDriverComponentBase::init(queueDepth, instance);
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

        for (FwIndexType i = 0; i < this->m_numChannels; i++) {
            // Check if this channel should be sampled
            if (!this->shouldSampleChannel(i)) {
                continue;
            }

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

    bool ZephyrAdcDriver::shouldSampleChannel(FwIndexType channelIndex) {
        if (channelIndex >= this->m_numChannels) {
            return false;
        }

        // Simple time-based sampling
        static U32 sampleCounter = 0;
        sampleCounter++;

        // Sample based on configured interval
        return (sampleCounter % m_sampleInterval[channelIndex]) == 0;
    }

    bool ZephyrAdcDriver::readChannel(FwIndexType channelIndex,
                                               U32& rawValue, F32& voltageValue) {
        // Initialize sequence for this specific channel
        (void)adc_sequence_init_dt(&m_adcChannels[channelIndex], &this->m_sequence);


        // Read the channel
        int ret = adc_read_dt(&m_adcChannels[channelIndex], &this->m_sequence);
        if (ret < 0) {
            return false;
        }

        // Handle differential vs single-ended
        rawValue = m_adcChannels[channelIndex].channel_cfg.differential ?
                   static_cast<U32>(static_cast<I16>(m_sampleBuffer)) :
                   static_cast<U32>(m_sampleBuffer);

        // Convert to voltage if requested
        bool voltageMode;
        // this->paramGet_ADC_VOLTAGE_MODE(voltageMode);

        if (voltageMode) {
            I32 val_mv = static_cast<I32>(rawValue);
            ret = adc_raw_to_millivolts_dt(&m_adcChannels[channelIndex], &val_mv);
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
        for (FwIndexType i = 0; i < this->m_numChannels; i++) {
            (void)adc_sequence_init_dt(&this->m_adcChannels[i], &m_sequence);
            int ret = adc_read_dt(&this->m_adcChannels[i], &m_sequence);
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
