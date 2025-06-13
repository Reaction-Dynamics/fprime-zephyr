module Drv {
  enum AdcStatus {
    OP_OK @< Operation succeeded
    INVALID_CHANNEL @< Operation not permitted with current configuration
    UNKNOWN_ERROR @< An unknown error occurred
  }
}

module Zephyr {
    constant ADC_MAX_CHANNELS = 32
    array ADC_CHANNEL_U32s = [ADC_MAX_CHANNELS] U32
    array ADC_CHANNEL_F32s = [ADC_MAX_CHANNELS] F32

    port U32_Port() -> U32
    port F32_Port() -> F32

    @ ADC driver for Zephyr RTOS using device tree configuration
    active component ZephyrAdcDriver {

        # ----------------------------------------------------------------------
        # General ports
        # ----------------------------------------------------------------------

        @ Command receive port
        command recv port cmdIn

        @ Command registration port
        command reg port cmdRegOut

        @ Command response port
        command resp port cmdResponseOut

        @ Event port
        event port eventOut

        @ Port for sending textual representation of events (required for active components)
        text event port logTextOut

        @ Telemetry port
        telemetry port tlmOut

        @ Time get port
        time get port timeGetOut

        @ Parameter get port
        param get port prmGetOut

        @ Parameter set port
        param set port prmSetOut

        # ----------------------------------------------------------------------
        # Specialized ports
        # ----------------------------------------------------------------------

        @ ADC data output ports (async per channel)
        async input port schedIn: Svc.Sched

        @ ADC sample output ports - one per channel configured in device tree
        output port adcCountSample: [ADC_MAX_CHANNELS] U32_Port
        output port adcVoltageSample: [ADC_MAX_CHANNELS] F32_Port

        # ----------------------------------------------------------------------
        # Commands
        # ----------------------------------------------------------------------

        @ Enable/disable ADC sampling
        async command ADC_ENABLE(
            enable: bool @< Enable/disable ADC sampling
        )

        @ Set sampling rate for specific channel
        async command ADC_SET_RATE(
            channel: U8 @< Channel index (0-based)
            rate_hz: F32 @< Sampling rate in Hz
        )

        @ Read single ADC sample on demand
        async command ADC_READ_SINGLE(
            channel: U8 @< Channel index to read
        )

        @ Calibrate ADC (if supported by hardware)
        async command ADC_CALIBRATE

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        @ ADC driver initialized successfully
        event ADC_INITIALIZED(
            num_channels: U8 @< Number of configured channels
        ) severity activity high \
        format "Initialized ADC with {} channels"

        @ ADC sampling enabled/disabled
        event ADC_SAMPLING_CHANGED(
            enabled: bool @< New sampling state
        ) severity activity low \
        format "Sampling enabled: {}"

        @ ADC read error
        event ADC_READ_ERROR(
            channel: U8 @< Channel that failed
            error_code: I32 @< Error code from driver
        ) severity warning high \
        format "Read Error {} {}"

        @ ADC calibration completed
        # event ADC_CALIBRATION_DONE(
        #     status: Drv.AdcStatus @< Calibration result
        # ) severity activity low \
        # format "Calibration done: {}"

        @ Channel configuration error
        event ADC_CHANNEL_ERROR(
            channel: U8 @< Problem channel
            description: string @< Error description
        ) severity activity high \
        format "Channel Error: {} {}"

        # ----------------------------------------------------------------------
        # Parameters
        # ----------------------------------------------------------------------

        @ Global ADC sampling enable
        param ADC_SAMPLING_ENABLED: bool default true

        @ Per-channel sampling rates (Hz) - array sized by compile-time channels
        # param ADC_SAMPLE_RATES: F32 [ADC_MAX_CHANNELS] default [10.0, 10.0, 1.0, 1.0, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1]

        @ ADC timeout in milliseconds
        param ADC_TIMEOUT_MS: U32 default 100

        @ Enable voltage conversion (vs raw counts)
        param ADC_VOLTAGE_MODE: bool default true

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ Raw ADC counts per channel
        telemetry ADC_RAW_COUNTS: ADC_CHANNEL_U32s

        telemetry LATEST_ADC_SAMPLE: U32

        @ Converted voltages per channel (mV)
        telemetry ADC_VOLTAGES_MV: ADC_CHANNEL_F32s

        @ ADC read errors per channel
        # telemetry ADC_ERROR_COUNT: ADC_CHANNEL_U32s

        @ ADC sampling frequency achieved per channel
        telemetry ADC_ACTUAL_RATES: ADC_CHANNEL_F32s

        # Driver status
        # telemetry ADC_DRIVER_STATUS: Drv.AdcStatus

    }
}
