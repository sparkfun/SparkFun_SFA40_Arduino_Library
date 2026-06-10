/*
  Example 02 - Serial Number

  Every SFA40 has a unique 48-bit serial number assigned by Sensirion during production.
  This sketch reads it and prints it as a 12-digit hexadecimal value, which matches the
  serial number laser-marked on the metal cap of the sensor.

  Note: The serial number can only be read while the sensor is in idle state (no measurement
  running). begin() leaves the sensor idle, so this sketch reads it before starting any
  measurement.

  SparkFun Electronics
  Date: 2026
  SparkFun code, firmware, and software is released under the MIT License.
    Please see LICENSE.md for further details.

  Hardware Connections:
  IoT RedBoard --> SFA40
  QWIIC --> QWIIC

  Open the Serial Monitor at 115200 baud.

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/
*/

#include <SparkFun_SFA40.h>

SfeSFA40ArdI2C mySensor;

// Print an unsigned value as a fixed number of hexadecimal digits, with leading zeros.
void printHexPadded(uint32_t value, uint8_t digits)
{
    for (int8_t shift = (digits - 1) * 4; shift >= 0; shift -= 4)
    {
        uint8_t nibble = (value >> shift) & 0x0F;
        Serial.print(nibble, HEX);
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("SparkFun SFA40 Example 2 - Serial Number");

    Wire.begin();

    while (mySensor.begin() == false)
    {
        Serial.println("SFA40 not connected, check your wiring!");
        delay(1000);
    }

    Serial.println("SFA40 connected!");

    // Read the unique 48-bit serial number.
    uint64_t serialNumber = 0;
    if (mySensor.getSerialNumber(serialNumber) != ksfTkErrOk)
    {
        Serial.println("Failed to read serial number!");
        return;
    }

    // A uint64_t cannot be printed directly on all platforms, so split it into the upper
    // 16 bits and lower 32 bits and print each as zero-padded hex (4 + 8 = 12 digits).
    uint32_t high = (uint32_t)(serialNumber >> 32) & 0xFFFF;
    uint32_t low = (uint32_t)(serialNumber & 0xFFFFFFFF);

    Serial.print("Serial number: 0x");
    printHexPadded(high, 4);
    printHexPadded(low, 8);
    Serial.println();
    Serial.println("(This should match the serial number laser-marked on the sensor cap.)");
}

void loop()
{
    // Nothing to do here.
}
