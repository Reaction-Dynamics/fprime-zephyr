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
#include <limits>

namespace Zephyr {

    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------
    ZephyrAdcDriver::ZephyrAdcDriver(const char* const compName) :
        ZephyrAdcDriverComponentBase(compName),
        m_sampleBuffer(0)
    {
        this->NUM_CHANNELS = DT_PROP_LEN(DT_PATH(zephyr_user), io_channels);

        // Initialize ADC sequence for single reads
        this->m_sequence.buffer = &m_sampleBuffer;
        this->m_sequence.buffer_size = sizeof(m_sampleBuffer);
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

            this->m_sequence.calibrate = true;
            this->m_sequence.buffer = &this->m_sampleBuffer;
            this->m_sequence.buffer_size = sizeof(this->m_sampleBuffer);

            // Setup channel using device tree configuration
            st = adc_channel_setup_dt(&this->ADC_CHANNELS[channelIdx]);
            if (0 != st) {
                break;
            }

            // Initialize sequence for this specific channel
            st = adc_sequence_init_dt(&this->ADC_CHANNELS[channelIdx], &this->m_sequence);
            if (0 != st) {
                break;
            }

            Os::RawTimeInterface::Status timeStatus;
            timeStatus = this->m_timestamps[channelIdx].now();
            FW_ASSERT(timeStatus == Os::RawTimeInterface::Status::OP_OK);

            channelIdx++;
        }
        FW_ASSERT(channelIdx == this->NUM_CHANNELS, st, channelIdx, this->NUM_CHANNELS);

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
        Os::ScopeLock lock(this->m_mutex);

        for (FwSizeType i = 0; i < this->NUM_CHANNELS; i++) {
            I32 rawSample;
            F32 mvSample;
            bool status = this->readChannel(i, rawSample, mvSample);
            if (!status) {
                this->log_WARNING_HI_ADC_READ_ERROR(
                    static_cast<U8>(i),
                    static_cast<I32>(status)
                );
                continue;
            }

            Os::RawTimeInterface::Status timeStatus;
            Os::RawTime currentTime;
            Fw::TimeInterval interval;
            timeStatus = currentTime.now();
            FW_ASSERT(timeStatus == Os::RawTimeInterface::Status::OP_OK);

            timeStatus = this->m_timestamps[i].getTimeInterval(currentTime, interval);
            FW_ASSERT(timeStatus == Os::RawTimeInterface::Status::OP_OK);

            F32 intervalTotalSeconds;
            // This will give us a float with [seconds].[milliseconds]
            // NOTE we divide by 1000 not 100000 to convert Us to Ms
            intervalTotalSeconds = interval.getSeconds() + static_cast<F32>(interval.getUSeconds()) / 1000;

            U32 currentCount = this->m_samples[i].getCount();
            this->m_samples[i].set(1.0f/intervalTotalSeconds, ++currentCount, mvSample, rawSample);
            // Send data on async output port
            if (!this->isConnected_adcCountSample_OutputPort(i)) {
                continue;
            }
        }

        // Update telemetry
        this->tlmWrite_ADC_STATUS(this->m_samples);
    }

    bool ZephyrAdcDriver::readChannel(FwIndexType channelIndex, I32 &rawSample, F32 &mvSample) {
        // Read the channel
        int ret = adc_read_dt(&this->ADC_CHANNELS[channelIndex], &this->m_sequence);
        FW_ASSERT(ret == 0, ret, channelIndex);

        // Since we're using an I32 to handle the raw sample we need to make sure the sample is less than that
        FW_ASSERT(std::numeric_limits<I32>::max() > *static_cast<U32*>(this->m_sequence.buffer),
                  *static_cast<U32*>(this->m_sequence.buffer));

        // Handle differential vs single-ended
        rawSample = this->ADC_CHANNELS[channelIndex].channel_cfg.differential ?
                   *static_cast<U32*>(this->m_sequence.buffer) :
                   *static_cast<I32*>(this->m_sequence.buffer);

        I32 voltageValue = rawSample;

        ret = adc_raw_to_millivolts_dt(&this->ADC_CHANNELS[channelIndex], &voltageValue);
        FW_ASSERT(ret == 0, ret, channelIndex);

        mvSample = static_cast<F32>(voltageValue);

        return true;
    }

    // ----------------------------------------------------------------------
    // Command handler implementations
    // ----------------------------------------------------------------------

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
