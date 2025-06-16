// ======================================================================
// \title  ZephyrAdcDriver.hpp
// \author user
// \brief  hpp file for ZephyrAdcDriver component implementation class
// ======================================================================


#ifndef ZEPHYR_ADC_DRIVER_HPP
#define ZEPHYR_ADC_DRIVER_HPP
#include "fprime-zephyr/Drv/ZephyrAdcDriver/ADC_CHANNEL_F32sArrayAc.hpp"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>

// Save and clear problematic macros before F Prime headers
#pragma push_macro("EMPTY")
#ifdef EMPTY
#undef EMPTY
#endif

#include "config/FwIndexTypeAliasAc.h"
#include "config/FwSizeTypeAliasAc.h"
#include "fprime-zephyr/Drv/ZephyrAdcDriver/FppConstantsAc.hpp"
#include "fprime-zephyr/Drv/ZephyrAdcDriver/ZephyrAdcDriverComponentAc.hpp"

#include <Os/Mutex.hpp>
#include <Fw/Time/Time.hpp>

// Restore Zephyr macros if needed later
#pragma pop_macro("EMPTY")

// Extract channel names from device tree
// #if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channel_names)
// #define CHANNEL_NAME(node_id, prop, idx)
//     DT_PROP_BY_IDX(node_id, prop, idx),
// static const char* const channel_names[] = {
//     DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channel_names, CHANNEL_NAME)
// };
// #else
// #error "Must have channel for device"
// #endif

// Device tree validation
#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No ADC channels defined in device tree overlay"
#endif

#if !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "io-channels property missing from device tree"
#endif

#define ADC_FOREACH_DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

namespace Zephyr {
    class ZephyrAdcDriver : public ZephyrAdcDriverComponentBase {
    public:
        // Constructor
        ZephyrAdcDriver(const char* const compName);

        // Destructor
        ~ZephyrAdcDriver();

        void init(FwSizeType queueDepth, FwEnumStoreType instance);

        // Number of channels configured in device tree
        // static constexpr FwSizeType NUM_CHANNELS = DT_PROP_LEN(DT_PATH(zephyr_user), io_channels);
        FwSizeType NUM_CHANNELS;

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
        bool readChannel(FwIndexType channelIndex);

        // Process all configured channels
        void processAllChannels();

        // Update telemetry for a channel
        void updateChannelTelemetry();

        // Initialize ADC channels from device tree
        bool initializeChannels();

        // Convert raw ADC to engineering units
        F32 convertToVoltage(FwIndexType channelIndex, U32 rawValue);

        // ----------------------------------------------------------------------
        // Member variables
        // ----------------------------------------------------------------------

        // ADC channel specifications from device tree
        // const struct adc_dt_spec ADC_CHANNELS[NUM_CHANNELS] = {
        //         DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, ADC_FOREACH_DT_SPEC_AND_COMMA)
        // };
        // struct adc_dt_spec ADC_CHANNELS[ADC_MAX_CHANNELS] = {};
        static const struct adc_dt_spec ADC_CHANNELS[];

        // Mutex for thread safety
        Os::Mutex m_mutex;

        // Sampling state
        U32 m_errorCounts[ADC_MAX_CHANNELS];

        F32 m_voltageSamples[ADC_MAX_CHANNELS];
        U32 m_countSamples[ADC_MAX_CHANNELS];

        // ADC sequence buffer for single reads
        U16 m_sampleBuffer;
        struct adc_sequence m_sequence;

        // Telemetry update counters
        U32 m_tlmUpdateCounter;
    };

} // end namespace Zephyr

#endif // ZEPHYR_ADC_DRIVER_HPP
