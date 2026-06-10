/**
 * @file sfDevSFA40.h
 * @brief Header file for the SparkFun SFA40 Formaldehyde Sensor Driver.
 *
 * @details
 * sfDevSFA40 is a comms-agnostic driver for the Sensirion SFA40 miniature electrochemical
 * formaldehyde (HCHO) sensor, built on the SparkFun Toolkit. The SFA40 reports a calibrated
 * formaldehyde concentration (ppb) along with on-board relative humidity and temperature, used
 * internally to compensate the formaldehyde signal.
 *
 * The SFA40 uses Sensirion's command-based I2C protocol: 16-bit commands are sent most significant
 * byte first, and returned data arrives in 3-byte groups (a 16-bit word followed by a CRC-8 byte).
 *
 * @author SparkFun Electronics
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * @see https://github.com/sparkfun/SparkFun_SFA40_Arduino_Library
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

// SparkFun Toolkit core headers
#include <sfTk/sfToolkit.h>
#include <sfTk/sfTkII2C.h>

///////////////////////////////////////////////////////////////////////////////
// Sensor Status Bits
///////////////////////////////////////////////////////////////////////////////
/// @brief Bit positions within the status byte returned by readMeasurement().
/// @details After power-on the sensor needs time to settle. While bit 0 is set the sensor is not
/// ready (the first minute after power-on, when the formaldehyde output is forced to 0). While bit 1
/// is set the sensor is ready but not yet within specifications (the first ten minutes after
/// power-on). When both bits are clear the sensor is reading within specifications.
typedef enum sfe_sfa40_status_bit_t : uint8_t
{
    SFA40_STATUS_NOT_READY = 0,      ///< Bit 0: sensor not ready (< 1 min after power-up)
    SFA40_STATUS_NOT_WITHIN_SPEC = 1 ///< Bit 1: sensor not yet within specifications (< 10 min after power-up)
} sfe_sfa40_status_bit_t;

///////////////////////////////////////////////////////////////////////////////
// Class Declaration
///////////////////////////////////////////////////////////////////////////////

/// @brief Platform-independent driver for the Sensirion SFA40 formaldehyde sensor.
///
/// @details This class implements command-level access to the SFA40 via the SparkFun Toolkit bus
/// interface. Most methods return a SparkFun Toolkit error code (::ksfTkErrOk on success, a negative
/// value on error). The CRC-8 checksum of every received data word is validated automatically;
/// ::ksfTkErrFail is returned if any checksum does not match.
///
/// The typical flow is: begin(), startMeasurement(), then call readMeasurement() at roughly 1 Hz and
/// read the converted values with getHCHO() / getHumidity() / getTemperature(). Note the formaldehyde
/// output is only valid once the sensor is within specifications (see isWithinSpecifications()).
class sfDevSFA40
{
  public:
    sfDevSFA40() : _theBus{nullptr}
    {
    }

    /// @brief Initialize the device driver with the given bus.
    /// @details Adopts the supplied bus and confirms an SFA40 is responding by reading and
    /// validating its serial number. The sensor must be in idle state (the power-on default) for
    /// this check, so call begin() before starting a measurement.
    /// @param theBus Pointer to the initialized bus object. If null, a bus set by a prior call is used.
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t begin(sfTkIBus *theBus = nullptr);

    /// @brief Check whether the SFA40 is connected and responding.
    /// @details Reads the serial number and validates its CRC. Requires the sensor to be in idle
    /// state (no measurement running).
    /// @return true if the device responds with a valid serial number, false otherwise.
    bool isConnected(void);

    // ========================= Measurement Control ========================

    /// @brief Start continuous measurement mode.
    /// @details Brings the sensor from idle into measurement mode, where it autonomously updates a
    /// new data point roughly every 0.5 s. readMeasurement() may only be called while this mode is
    /// running. The sensor needs ~10 minutes after power-on before the formaldehyde output is within
    /// specifications.
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t startMeasurement(void);

    /// @brief Stop continuous measurement mode and return the sensor to idle.
    /// @details Waits for the command's execution time before returning so a subsequent command (for
    /// example getSerialNumber()) is not issued too early.
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t stopMeasurement(void);

    /// @brief Read one measurement data point and cache the result.
    /// @details Reads the formaldehyde, humidity, temperature and status words, validates the CRC of
    /// each, and stores the raw values for retrieval via getHCHO(), getHumidity(), getTemperature(),
    /// and getStatus(). Continuous measurement mode must be running. The sensor updates internally
    /// every ~0.5 s; reading faster simply returns the most recent data point again.
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail on a CRC mismatch, or an error code on a
    /// communication failure.
    sfTkError_t readMeasurement(void);

    // ====================== Cached Measurement Values =====================

    /// @brief Get the formaldehyde concentration from the most recent readMeasurement().
    /// @details The formaldehyde output is forced to 0 during the first minute after power-on and is
    /// only within specifications after ~10 minutes (see isWithinSpecifications()).
    /// @return Formaldehyde concentration in parts per billion (ppb).
    float getHCHO(void);

    /// @brief Get the relative humidity from the most recent readMeasurement().
    /// @details This is the local humidity at the sensor board, used for internal compensation, and
    /// may differ from ambient humidity. The value is clamped to the 0–100 %RH range.
    /// @return Relative humidity in percent (%RH).
    float getHumidity(void);

    /// @brief Get the temperature from the most recent readMeasurement(), in degrees Celsius.
    /// @details This is the local temperature at the sensor board, used for internal compensation,
    /// and may differ from ambient temperature.
    /// @return Temperature in degrees Celsius (°C).
    float getTemperature(void);

    /// @brief Get the temperature from the most recent readMeasurement(), in degrees Fahrenheit.
    /// @return Temperature in degrees Fahrenheit (°F).
    float getTemperatureF(void);

    /// @brief Get the raw sensor status byte from the most recent readMeasurement().
    /// @return The raw status byte (see sfe_sfa40_status_bit_t for bit definitions).
    uint8_t getStatus(void);

    /// @brief Check whether the sensor is ready (status bit 0 clear).
    /// @details The sensor is not ready during the first minute after power-on.
    /// @return true if the sensor is ready, false otherwise.
    bool isReady(void);

    /// @brief Check whether the sensor is reading within specifications (status bit 1 clear).
    /// @details The sensor is not yet within specifications during the first ten minutes after
    /// power-on. The formaldehyde reading should not be trusted until this returns true.
    /// @return true if the sensor is within specifications, false otherwise.
    bool isWithinSpecifications(void);

    // ===================== Raw Cached Measurement Ticks ===================

    /// @brief Get the raw formaldehyde ticks from the most recent readMeasurement().
    /// @return Raw 16-bit formaldehyde value (ppb = ticks / 10).
    uint16_t getHCHORaw(void);

    /// @brief Get the raw humidity ticks from the most recent readMeasurement().
    /// @return Raw 16-bit humidity value.
    uint16_t getHumidityRaw(void);

    /// @brief Get the raw temperature ticks from the most recent readMeasurement().
    /// @return Raw 16-bit temperature value.
    uint16_t getTemperatureRaw(void);

    // ========================= Identity & Self-test =======================

    /// @brief Read the unique 48-bit serial number assigned by Sensirion.
    /// @details The sensor must be in idle state (no measurement running).
    /// @param serialNumber Output reference that receives the 48-bit serial number.
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail on a CRC mismatch, or an error code on failure.
    sfTkError_t getSerialNumber(uint64_t &serialNumber);

    /// @brief Start the sensor self-test.
    /// @details The self-test checks the integrity of the electrical connections to the sensor cell
    /// (for example to detect bad soldering) and runs for 5–6 minutes. It is recommended to run it
    /// once after the sensor has been soldered to the PCB. The sensor must be in idle state. Poll
    /// readSelfTestResult() (for example every 10 s) until the test reports a result.
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t startSelfTest(void);

    /// @brief Read the self-test status / result.
    /// @details Call after startSelfTest(). A raw result of 0xFFFF means the test is still running;
    /// 0x0000 means it finished and passed; any other value means it finished and failed.
    /// @param result Output reference that receives the raw 16-bit self-test result.
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail on a CRC mismatch, or an error code on failure.
    sfTkError_t readSelfTestResult(uint16_t &result);

  protected:
    /// @brief Send a 16-bit command to the sensor (most significant byte first).
    /// @param command The 16-bit command code.
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t sendCommand(uint16_t command);

    /// @brief Send a command and read back a sequence of CRC-protected 16-bit words.
    /// @details Each word arrives as two data bytes (MSB first) followed by a CRC-8 byte. The CRC of
    /// every word is validated before the word is stored.
    /// @param command The 16-bit command code that requests the data.
    /// @param words Output buffer that receives @p numWords 16-bit words.
    /// @param numWords Number of 16-bit words to read (no more than kMaxWords).
    /// @param readDelayMs Optional delay, in milliseconds, between sending the command and reading.
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail on a CRC mismatch, or an error code on failure.
    sfTkError_t readWords(uint16_t command, uint16_t *words, uint8_t numWords, uint32_t readDelayMs = 0);

    /// @brief Compute the Sensirion CRC-8 over a buffer of data bytes.
    /// @details Polynomial 0x31, initial value 0xFF, no final XOR.
    /// @param data Pointer to the data bytes.
    /// @param length Number of bytes to include in the checksum.
    /// @return The 8-bit checksum.
    static uint8_t computeCRC8(const uint8_t *data, size_t length);

    sfTkIBus *_theBus; ///< Pointer to the communication bus device.

    // --- Cached raw values from the most recent readMeasurement() ---
    uint16_t _hchoTicks = 0;        ///< Raw formaldehyde value (ppb = ticks / 10).
    uint16_t _humidityTicks = 0;    ///< Raw relative humidity value.
    uint16_t _temperatureTicks = 0; ///< Raw temperature value.
    uint8_t _status = 0;            ///< Raw sensor status byte.

    ///////////////////////////////////////////////////////////////////////////
    // I2C Addressing
    ///////////////////////////////////////////////////////////////////////////
    static const uint8_t kI2CAddress = 0x5D;        ///< 7-bit I2C address of the SFA40 (fixed).
    static const uint8_t kGeneralCallAddress = 0x00; ///< I2C general call address used for soft reset.

    ///////////////////////////////////////////////////////////////////////////
    // Command Codes
    ///////////////////////////////////////////////////////////////////////////
    static const uint16_t kCommandStartMeasurement = 0x00AC; ///< Start continuous measurement.
    static const uint16_t kCommandReadMeasurement = 0xC0EB;  ///< Read measurement data (12 bytes).
    static const uint16_t kCommandStopMeasurement = 0x50D2;  ///< Stop continuous measurement.
    static const uint16_t kCommandReadSerialNumber = 0x02CE; ///< Read the 48-bit serial number.
    static const uint16_t kCommandStartSelfTest = 0x060A;    ///< Start the self-test.
    static const uint16_t kCommandReadSelfTestData = 0xC0EB; ///< Read self-test status / result.
    static const uint8_t kCommandSoftReset = 0x06;           ///< Soft reset (sent to the general call address).

    ///////////////////////////////////////////////////////////////////////////
    // CRC Parameters
    ///////////////////////////////////////////////////////////////////////////
    static const uint8_t kCrcPolynomial = 0x31;    ///< CRC-8 polynomial.
    static const uint8_t kCrcInitialValue = 0xFF;  ///< CRC-8 initial value.

    ///////////////////////////////////////////////////////////////////////////
    // Conversion Constants
    ///////////////////////////////////////////////////////////////////////////
    static constexpr float kHchoScale = 0.1f;         ///< Formaldehyde: ppb = ticks * 0.1.
    static constexpr float kTicksFullScale = 65535.0f; ///< Full-scale tick count for RH / temperature.
    static constexpr float kHumiditySlope = 125.0f;    ///< Humidity conversion slope.
    static constexpr float kHumidityOffset = -6.0f;    ///< Humidity conversion offset (%RH).
    static constexpr float kTemperatureSlopeC = 175.0f; ///< Temperature conversion slope (°C).
    static constexpr float kTemperatureOffsetC = -45.0f; ///< Temperature conversion offset (°C).
    static constexpr float kTemperatureSlopeF = 315.0f;  ///< Temperature conversion slope (°F).
    static constexpr float kTemperatureOffsetF = -49.0f; ///< Temperature conversion offset (°F).

    ///////////////////////////////////////////////////////////////////////////
    // Self-test Result Values
    ///////////////////////////////////////////////////////////////////////////
    static const uint16_t kSelfTestRunning = 0xFFFF; ///< Self-test still running.
    static const uint16_t kSelfTestPassed = 0x0000;  ///< Self-test finished and passed.

    ///////////////////////////////////////////////////////////////////////////
    // Timing (execution times, in milliseconds)
    ///////////////////////////////////////////////////////////////////////////
    static const uint32_t kStopMeasurementDelayMs = 25; ///< Execution time for stop measurement.
    static const uint32_t kSoftResetDelayMs = 25;       ///< Execution time for soft reset.

    ///////////////////////////////////////////////////////////////////////////
    // Buffer Sizing
    ///////////////////////////////////////////////////////////////////////////
    static const uint8_t kBytesPerWord = 3; ///< Two data bytes plus one CRC byte.
    static const uint8_t kMaxWords = 4;     ///< Largest response (read measurement) is four words.
};
