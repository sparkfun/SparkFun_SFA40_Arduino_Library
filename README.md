![SparkFun Qwiic Formaldehyde Sensor - SFA40](docs/images/gh-banner-2025-arduino-sfa40.png "SparkFun Qwiic Formaldehyde Sensor - SFA40")

# SparkFun Qwiic Formaldehyde Sensor - SFA40

Arduino Library for the SparkFun Qwiic Formaldehyde Sensor (SFA40)

![GitHub License](https://img.shields.io/github/license/sparkfun/SparkFun_SFA40_Arduino_Library)
![Release](https://img.shields.io/github/v/release/sparkfun/SparkFun_SFA40_Arduino_Library)
![Release Date](https://img.shields.io/github/release-date/sparkfun/SparkFun_SFA40_Arduino_Library)
![Documentation - build](https://img.shields.io/github/actions/workflow/status/sparkfun/SparkFun_SFA40_Arduino_Library/build-deploy-ghpages.yml?label=doc%20build)
[![Compile Test](https://github.com/sparkfun/SparkFun_SFA40_Arduino_Library/actions/workflows/test-compile-sketch.yml/badge.svg)](https://github.com/sparkfun/SparkFun_SFA40_Arduino_Library/actions/workflows/test-compile-sketch.yml)
![GitHub issues](https://img.shields.io/github/issues/sparkfun/SparkFun_SFA40_Arduino_Library)

The SparkFun Qwiic Formaldehyde Sensor puts the Sensirion SFA40 miniature electrochemical formaldehyde (HCHO) sensor on a Qwiic-enabled breakout, making indoor air-quality measurement as simple as plugging in a cable. The SFA40 is highly selective to formaldehyde with very low cross-sensitivity to common indoor gases such as ethanol, and is factory-calibrated to report a digital concentration directly in parts per billion (ppb).

This library provides an easy-to-use interface to the SFA40 over I2C, built on the [SparkFun Toolkit](https://github.com/sparkfun/SparkFun_Toolkit). It handles the Sensirion command protocol for you — including CRC validation of every reading — and returns calibrated formaldehyde, relative humidity, and temperature values.

## Functionality

The library exposes the full SFA40 command set:

- Calibrated formaldehyde concentration in ppb (measurement range 0–2000 ppb)
- On-board relative humidity and temperature (used internally to compensate the formaldehyde signal)
- Continuous measurement start / stop control
- Sensor warm-up status reporting (ready / within specifications)
- Unique 48-bit serial number readout (matches the laser marking on the sensor)
- Built-in self-test for verifying the sensor's electrical connections (e.g. after soldering)
- Soft reset via the I2C general call
- Automatic CRC-8 validation of every received data word

> [!NOTE]
> The SFA40 needs to warm up after power-on. For the first minute the sensor is not ready and the formaldehyde output is forced to 0. Between roughly 1 and 10 minutes the sensor is ready but not yet within specifications. After about 10 minutes the formaldehyde reading is within specifications. See the status helpers below and Example 04.

## Hardware Connections

The sensor connects via I2C using either a Qwiic connector or by using the plated through-hole headers. The SFA40 uses a single fixed 7-bit I2C address of `0x5D`.

| Pin / Header | Use | Notes |
| -- | -- | -- |
| Qwiic / I2C | Power + communication | Standard 3.3V Qwiic connection |

The SFA40 supports I2C standard mode (100 kHz) and fast mode (400 kHz).

## Using the Library

### Installation

Install through the Arduino Library Manager by searching for **SparkFun SFA40**, or download this repository as a ZIP and add it via *Sketch > Include Library > Add .ZIP Library*. This library depends on the [SparkFun Toolkit](https://github.com/sparkfun/SparkFun_Toolkit), which the Library Manager will offer to install alongside it.

### Getting Started

The I2C interface to the sensor is provided by the `SfeSFA40ArdI2C` class. Declare a sensor object:

```c++
#include <SparkFun_SFA40.h>

// Declare our sensor object
SfeSFA40ArdI2C mySensor;
```

In `setup()`, start I2C and call `begin()`. `begin()` confirms the device is present by reading and validating its serial number, and leaves the sensor in idle state:

```c++
Wire.begin();

while (mySensor.begin() == false)
{
    Serial.println("SFA40 not connected, check your wiring!");
    delay(1000);
}
```

To read measurement data, bring the sensor into continuous measurement mode with `startMeasurement()`. The sensor then updates a new data point internally about every 0.5 seconds:

```c++
mySensor.startMeasurement();
```

### Reading Measurements

Call `readMeasurement()` to fetch a fresh data point (this validates the CRC of every value), then read the converted values:

```c++
if (mySensor.readMeasurement() == ksfTkErrOk)
{
    Serial.print("HCHO (ppb): ");
    Serial.print(mySensor.getHCHO(), 1);
    Serial.print("  Humidity (%RH): ");
    Serial.print(mySensor.getHumidity(), 1);
    Serial.print("  Temperature (C): ");
    Serial.println(mySensor.getTemperature(), 1);
}
```

`getTemperatureF()` returns the temperature in degrees Fahrenheit. The raw, unconverted tick values are also available via `getHCHORaw()`, `getHumidityRaw()`, and `getTemperatureRaw()`.

Reading once per second is plenty — reading faster than the sensor's ~0.5 s internal update simply returns the most recent data point again.

### Checking the Warm-up Status

Each measurement carries a status that reports the sensor's readiness. Use it to decide when the formaldehyde value can be trusted:

```c++
if (!mySensor.isReady())
    Serial.println("Sensor warming up...");          // first ~1 minute
else if (!mySensor.isWithinSpecifications())
    Serial.println("Stabilizing, not yet in spec");  // first ~10 minutes
else
    Serial.println("Formaldehyde reading is valid");
```

`getStatus()` returns the raw status byte if you need it.

### A Note on Return Values and Error Handling

Most library methods return a SparkFun Toolkit error code (`ksfTkErrOk` on success, a negative value on failure). For the data-reading commands, this also covers CRC validation. A corrupted reading returns an error rather than bad data. For simple sketches you can ignore the return value:

```c++
mySensor.readMeasurement(); // assume good data
float hcho = mySensor.getHCHO();
```

For robust applications, check it:

```c++
if (mySensor.readMeasurement() != ksfTkErrOk)
{
    // Handle the communication / CRC error
}
else
{
    // The cached values hold a valid reading
}
```

### Reading the Serial Number

Every SFA40 has a unique 48-bit serial number that matches the value laser-marked on the sensor cap. It must be read while the sensor is idle (no measurement running):

```c++
uint64_t serialNumber = 0;
mySensor.getSerialNumber(serialNumber);
```

### Self-test

The self-test checks the integrity of the electrical connections to the sensor cell and is recommended once after the sensor is soldered to a PCB. It runs for 5–6 minutes; poll for the result until it is no longer `0xFFFF` (still running). A result of `0x0000` means the test passed:

```c++
mySensor.startSelfTest();

uint16_t result = 0xFFFF;
while (result == 0xFFFF)
{
    delay(10000);
    mySensor.readSelfTestResult(result);
}

Serial.println(result == 0x0000 ? "Self-test passed" : "Self-test failed");
```

### Soft Reset

`reset()` returns the sensor to its power-on state. The SFA40 reset is issued as an I2C general call, so it affects all devices on the bus that respond to a general-call reset:

```c++
mySensor.reset();
```

## Examples

The library ships with a set of examples that build from the basics to more advanced features:

- [Example 01 - Basic Reading](examples/Example01_BasicReading/Example01_BasicReading.ino) — the minimal sketch to read formaldehyde, humidity, and temperature
- [Example 02 - Serial Number](examples/Example02_SerialNumber/Example02_SerialNumber.ino) — read the sensor's unique 48-bit serial number
- [Example 03 - Self-test](examples/Example03_SelfTest/Example03_SelfTest.ino) — run the built-in self-test and report the result
- [Example 04 - Sensor Status](examples/Example04_SensorStatus/Example04_SensorStatus.ino) — warm-up-aware reading that only trusts the formaldehyde value once it is within specifications

## Documentation

API documentation is generated with Doxygen and published to GitHub Pages from the `main` branch.

## Products That Use This Library

- SparkFun Qwiic Formaldehyde Sensor - SFA40

## Contributing

If you would like to contribute to this library, please report issues and submit pull requests against the GitHub repository.

## License

This product is open source! Please see [LICENSE.md](LICENSE.md) for more information.

- Your friends at SparkFun
