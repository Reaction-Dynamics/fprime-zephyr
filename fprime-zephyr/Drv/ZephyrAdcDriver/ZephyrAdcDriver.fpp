module Drv {
  enum AdcStatus {
    OP_OK @< Operation succeeded
    INVALID_CHANNEL @< Operation not permitted with current configuration
    UNKNOWN_ERROR @< An unknown error occurred
  }

  struct AdcConfig {
    Id: U8
    VRefMv: F32
  }

  struct AdcSample {
    RateHz: F32
    Count: U32
    Mv: F32
    Raw: U32
  }
}

module Zephyr {
    constant ADC_MAX_CHANNELS = 8
    array ADC_CHANNELS = [ADC_MAX_CHANNELS] Drv.AdcSample

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

        # @ Parameter get port
        # param get port prmGetOut

        # @ Parameter set port
        # param set port prmSetOut

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

        @ Calibrate ADC (if supported by hardware)
        async command ADC_CALIBRATE

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        @ ADC read error
        event ADC_READ_ERROR(
            channel: U8 @< Channel that failed
            error_code: I32 @< Error code from driver
        ) severity warning high \
        format "Read Error {} {}"

        @ ADC calibration completed
        event ADC_CALIBRATION_DONE(
            status: Drv.AdcStatus @< Calibration result
        ) severity activity low \
        format "Calibration done: {}"

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
        # param ADC_SAMPLING_ENABLED: bool default true

        @ Per-channel sampling rates (Hz) - array sized by compile-time channels
        # param ADC_SAMPLE_RATES: F32 [ADC_MAX_CHANNELS] default [10.0, 10.0, 1.0, 1.0, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1]

        @ ADC timeout in milliseconds
        # param ADC_TIMEOUT_MS: U32 default 100

        @ Enable voltage conversion (vs raw counts)
        # param ADC_VOLTAGE_MODE: bool default true

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ Driver status
        telemetry ADC_STATUS: ADC_CHANNELS
    }
}
