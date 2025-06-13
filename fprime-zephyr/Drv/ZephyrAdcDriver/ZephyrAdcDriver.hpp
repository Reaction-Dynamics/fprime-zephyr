// ======================================================================
// \title  ZephyrAdcDriver.hpp
// \author user
// \brief  hpp file for ZephyrAdcDriver component implementation class
// ======================================================================


#ifndef ZEPHYR_ADC_DRIVER_HPP
#define ZEPHYR_ADC_DRIVER_HPP

#include "Platform/PlatformSizeTypeAliasAc.h"
#include "config/FwIndexTypeAliasAc.h"
#include "config/FwSizeTypeAliasAc.h"
#include "fprime-zephyr/Drv/ZephyrAdcDriver/FppConstantsAc.hpp"
#include "fprime-zephyr/Drv/ZephyrAdcDriver/ZephyrAdcDriverComponentAc.hpp"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>
#include <Os/Mutex.hpp>
#include <Fw/Time/Time.hpp>

// Device tree validation
#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No ADC channels defined in device tree overlay"
#endif

// Macro to generate ADC channel specs from device tree
#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

namespace Zephyr {
    class ZephyrAdcDriver : public ZephyrAdcDriverComponentBase {

    public:
        // Constructor
        ZephyrAdcDriver(const char* const compName);

        // Destructor
        ~ZephyrAdcDriver();

        // Get number of configured channels
        FwSizeType getNumChannels() const { return this->m_numChannels; }

        void init(FwSizeType queueDepth, FwEnumStoreType instance);

    PRIVATE:

        // ----------------------------------------------------------------------
        // Handler implementations for user-defined typed input ports
        // ----------------------------------------------------------------------

        // Scheduler input - triggers periodic ADC reads
        void schedIn_handler(
            const FwIndexType portNum,
            U32 context
        ) override;

        // ----------------------------------------------------------------------
        // Command handler implementations
        // ----------------------------------------------------------------------

        void ADC_ENABLE_cmdHandler(
            const FwOpcodeType opCode,
            const U32 cmdSeq,
            bool enable
        ) override;

        void ADC_SET_RATE_cmdHandler(
            const FwOpcodeType opCode,
            const U32 cmdSeq,
            U8 channel,
            F32 rate_hz
        ) override;

        void ADC_READ_SINGLE_cmdHandler(
            const FwOpcodeType opCode,
            const U32 cmdSeq,
            U8 channel
        ) override;

        void ADC_CALIBRATE_cmdHandler(
            const FwOpcodeType opCode,
            const U32 cmdSeq
        ) override;

        // ----------------------------------------------------------------------
        // Private helper methods
        // ----------------------------------------------------------------------

        // // Read single ADC channel
        bool readChannel(FwIndexType channelIndex, U32& rawValue, F32& voltageValue);

        // Process all configured channels
        void processAllChannels();

        // Update telemetry for a channel
        void updateChannelTelemetry(FwIndexType channelIndex, U32 rawValue, F32 voltageValue);

        // Check if channel should be sampled based on rate
        bool shouldSampleChannel(FwIndexType channelIndex);

        // Initialize ADC channels from device tree
        bool initializeChannels();

        // Convert raw ADC to engineering units
        F32 convertToVoltage(FwIndexType channelIndex, U32 rawValue);

        // ----------------------------------------------------------------------
        // Member variables
        // ----------------------------------------------------------------------

        // ADC channel specifications from device tree
        static const struct adc_dt_spec m_adcChannels[];

        // Number of channels configured in device tree
        static const FwSizeType m_numChannels;

        // Mutex for thread safety
        Os::Mutex m_mutex;

        // Sampling state
        bool m_samplingEnabled;

        // Per-channel timing state
        Fw::Time m_lastSampleTime[ADC_MAX_CHANNELS];
        U32 m_sampleCount[ADC_MAX_CHANNELS];
        U32 m_errorCount[ADC_MAX_CHANNELS];

        // Calculated sample intervals (scheduler ticks)
        U32 m_sampleInterval[ADC_MAX_CHANNELS];

        // Driver initialization state
        bool m_initialized;

        // ADC sequence buffer for single reads
        U16 m_sampleBuffer;
        struct adc_sequence m_sequence;

        // Telemetry update counters
        U32 m_tlmUpdateCounter;
    };

} // end namespace Zephyr

#endif // ZEPHYR_ADC_DRIVER_HPP
