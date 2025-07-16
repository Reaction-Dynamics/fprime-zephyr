// ======================================================================
// \title  ZephyrAdcDriver.hpp
// \author user
// \brief  hpp file for ZephyrAdcDriver component implementation class
// ======================================================================


#ifndef ZEPHYR_ADC_DRIVER_HPP
#define ZEPHYR_ADC_DRIVER_HPP
#include "Fw/Types/BasicTypes.h"
#include "Os/RawTime.hpp"
#include "fprime-zephyr/Drv/ZephyrAdcDriver/AdcSampleSerializableAc.hpp"
#include "fprime-zephyr/Drv/ZephyrAdcDriver/AdcConfigSerializableAc.hpp"
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


#define AFEC_COMPAT atmel_sam_afec

// Macro to get ADC specs and instance names for each channel using standard API
#define CREATE_ADC_SPEC_FOR_CHANNEL(node_id) \
    ADC_DT_SPEC_STRUCT(DT_PARENT(node_id), DT_REG_ADDR(node_id)),

#define PROCESS_AFEC_CHANNELS(inst, compat, ...) \
    DT_FOREACH_CHILD(DT_INST(inst, compat), CREATE_ADC_SPEC_FOR_CHANNEL)

// NOTE we should be able to use DT_STRINGIFY_INTERNAL to get the node label.
// Instead we leverage compat here which is a bit of a hack
#define CREATE_INSTANCE_NAME_FOR_CHANNEL(node_id, compat, inst) \
     #compat "_" DT_STRINGIFY_INTERNAL(inst) "_" DT_NODE_FULL_NAME(node_id),

#define PROCESS_AFEC_INSTANCE_NAMES(inst, compat, ...) \
    DT_FOREACH_CHILD_VARGS(DT_INST(inst, compat), CREATE_INSTANCE_NAME_FOR_CHANNEL, compat, inst)

#define COUNT_SINGLE_CHANNEL(node_id) +1

#define COUNT_AFEC_CHANNELS(compat, inst) \
    DT_FOREACH_CHILD(DT_DRV_INST(inst), COUNT_SINGLE_CHANNEL)

namespace Zephyr {
    class ZephyrAdcDriver : public ZephyrAdcDriverComponentBase {
    public:
        // Constructor
        ZephyrAdcDriver(const char* const compName);

        // Destructor
        ~ZephyrAdcDriver() {};

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

        void ADC_CALIBRATE_cmdHandler(
            const FwOpcodeType opCode,
            const U32 cmdSeq
        ) override;

        //! Handler implementation for command EMIT_CONFIG
        void EMIT_DEVICE_CONFIG_cmdHandler(FwOpcodeType opCode, //!< The opcode
                                U32 cmdSeq           //!< The command sequence number
                                ) override;

        // ----------------------------------------------------------------------
        // Private helper methods
        // ----------------------------------------------------------------------

        // // Read single ADC channel
        bool readChannel(FwIndexType channelIndex, I32 &rawSample, F32 &mvSample);

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
        static const char* const __attribute__((optimize(0))) INSTANCE_NAMES[];

        // Mutex for thread safety
        Os::Mutex m_mutex;

        // Sampling state
        Drv::AdcSample m_samples[ADC_MAX_CHANNELS];
        Os::RawTime m_timestamps[ADC_MAX_CHANNELS];
        Drv::AdcConfig m_configs[ADC_MAX_CHANNELS];

        // ADC sequence buffer for single reads
        U16 m_sampleBuffer;
        struct adc_sequence m_sequence;
    };

} // end namespace Zephyr

#endif // ZEPHYR_ADC_DRIVER_HPP
