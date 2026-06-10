/**
 * @file sfDevSFA40.cpp
 * @brief Implementation file for the SparkFun SFA40 Formaldehyde Sensor Driver.
 *
 * @details
 * This file implements the sfDevSFA40 class methods for controlling and reading data from the
 * Sensirion SFA40 formaldehyde sensor. The driver provides a comms-agnostic interface using the
 * SparkFun Toolkit and implements Sensirion's command-based I2C protocol, including CRC-8
 * validation of every received data word.
 *
 * @author SparkFun Electronics
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * @see https://github.com/sparkfun/SparkFun_SFA40_Arduino_Library
 */

#include "sfDevSFA40.h"

// ========================= Setup & Identity ===============================

sfTkError_t sfDevSFA40::begin(sfTkIBus *theBus)
{
    // Adopt the supplied bus if one was provided; otherwise keep any bus set by a prior begin().
    if (theBus != nullptr)
        _theBus = theBus;

    // We need a bus to talk to.
    if (_theBus == nullptr)
        return ksfTkErrBusNotInit;

    // Confirm an SFA40 is actually present and responding correctly before continuing. The serial
    // number read also validates the CRC, so a successful read is strong evidence of a real SFA40.
    if (!isConnected())
        return ksfTkErrBusNoResponse;

    return ksfTkErrOk;
}

bool sfDevSFA40::isConnected(void)
{
    uint64_t serialNumber = 0;
    return getSerialNumber(serialNumber) == ksfTkErrOk;
}

// ========================= Measurement Control ============================

sfTkError_t sfDevSFA40::startMeasurement(void)
{
    return sendCommand(kCommandStartMeasurement);
}

sfTkError_t sfDevSFA40::stopMeasurement(void)
{
    sfTkError_t rc = sendCommand(kCommandStopMeasurement);
    if (rc != ksfTkErrOk)
        return rc;

    // The sensor needs time to return to idle before it will accept another command.
    sftk_delay_ms(kStopMeasurementDelayMs);
    return ksfTkErrOk;
}

sfTkError_t sfDevSFA40::readMeasurement(void)
{
    // Read four CRC-protected words: formaldehyde, humidity, temperature, and status.
    uint16_t words[kMaxWords] = {0};
    sfTkError_t rc = readWords(kCommandReadMeasurement, words, kMaxWords);
    if (rc != ksfTkErrOk)
        return rc;

    _hchoTicks = words[0];
    _humidityTicks = words[1];
    _temperatureTicks = words[2];

    // The status word carries the 8-bit status in its most significant byte; the least significant
    // byte is reserved and always 0.
    _status = (uint8_t)(words[3] >> 8);

    return ksfTkErrOk;
}

// ====================== Cached Measurement Values =========================

float sfDevSFA40::getHCHO(void)
{
    return (float)_hchoTicks * kHchoScale;
}

float sfDevSFA40::getHumidity(void)
{
    float humidity = kHumidityOffset + kHumiditySlope * (float)_humidityTicks / kTicksFullScale;

    // The conversion can produce values slightly outside the physical range; clamp per datasheet.
    if (humidity < 0.0f)
        humidity = 0.0f;
    else if (humidity > 100.0f)
        humidity = 100.0f;

    return humidity;
}

float sfDevSFA40::getTemperature(void)
{
    return kTemperatureOffsetC + kTemperatureSlopeC * (float)_temperatureTicks / kTicksFullScale;
}

float sfDevSFA40::getTemperatureF(void)
{
    return kTemperatureOffsetF + kTemperatureSlopeF * (float)_temperatureTicks / kTicksFullScale;
}

uint8_t sfDevSFA40::getStatus(void)
{
    return _status;
}

bool sfDevSFA40::isReady(void)
{
    return (_status & (1 << SFA40_STATUS_NOT_READY)) == 0;
}

bool sfDevSFA40::isWithinSpecifications(void)
{
    return (_status & (1 << SFA40_STATUS_NOT_WITHIN_SPEC)) == 0;
}

// ===================== Raw Cached Measurement Ticks =======================

uint16_t sfDevSFA40::getHCHORaw(void)
{
    return _hchoTicks;
}

uint16_t sfDevSFA40::getHumidityRaw(void)
{
    return _humidityTicks;
}

uint16_t sfDevSFA40::getTemperatureRaw(void)
{
    return _temperatureTicks;
}

// ========================= Identity & Self-test ===========================

sfTkError_t sfDevSFA40::getSerialNumber(uint64_t &serialNumber)
{
    // The serial number arrives as three 16-bit words: part1 (MSW), part2, part3 (LSW).
    uint16_t words[3] = {0};
    sfTkError_t rc = readWords(kCommandReadSerialNumber, words, 3);
    if (rc != ksfTkErrOk)
        return rc;

    serialNumber = ((uint64_t)words[0] << 32) | ((uint64_t)words[1] << 16) | (uint64_t)words[2];
    return ksfTkErrOk;
}

sfTkError_t sfDevSFA40::startSelfTest(void)
{
    return sendCommand(kCommandStartSelfTest);
}

sfTkError_t sfDevSFA40::readSelfTestResult(uint16_t &result)
{
    return readWords(kCommandReadSelfTestData, &result, 1);
}

// ============================ Protected Helpers ===========================

sfTkError_t sfDevSFA40::sendCommand(uint16_t command)
{
    if (_theBus == nullptr)
        return ksfTkErrBusNotInit;

    // Commands are transmitted most significant byte first. Build the buffer explicitly so the
    // wire order does not depend on the bus byte-order setting.
    uint8_t commandBytes[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};
    return _theBus->writeData(commandBytes, sizeof(commandBytes));
}

sfTkError_t sfDevSFA40::readWords(uint16_t command, uint16_t *words, uint8_t numWords, uint32_t readDelayMs)
{
    if (_theBus == nullptr)
        return ksfTkErrBusNotInit;

    if (words == nullptr || numWords == 0 || numWords > kMaxWords)
        return ksfTkErrInvalidParam;

    // The command word is sent (MSB first) as the "register address", after which the response is
    // read back in the same transaction sequence.
    uint8_t commandBytes[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};

    uint8_t buffer[kMaxWords * kBytesPerWord] = {0};
    size_t numBytes = (size_t)numWords * kBytesPerWord;
    size_t readBytes = 0;

    sfTkError_t rc = _theBus->readRegister(commandBytes, sizeof(commandBytes), buffer, numBytes, readBytes, readDelayMs);
    if (rc != ksfTkErrOk)
        return rc;

    if (readBytes != numBytes)
        return ksfTkErrBusUnderRead;

    // Each word is two data bytes followed by a CRC-8 of those two bytes.
    for (uint8_t i = 0; i < numWords; i++)
    {
        const uint8_t *group = &buffer[i * kBytesPerWord];

        if (computeCRC8(group, 2) != group[2])
            return ksfTkErrFail;

        words[i] = ((uint16_t)group[0] << 8) | (uint16_t)group[1];
    }

    return ksfTkErrOk;
}

uint8_t sfDevSFA40::computeCRC8(const uint8_t *data, size_t length)
{
    uint8_t crc = kCrcInitialValue;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ kCrcPolynomial);
            else
                crc = (uint8_t)(crc << 1);
        }
    }

    return crc;
}
