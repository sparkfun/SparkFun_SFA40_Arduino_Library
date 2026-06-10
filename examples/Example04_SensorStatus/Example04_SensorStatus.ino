/*
  Example 04 - Sensor Status (Warm-up Aware)

  The SFA40 needs time to settle after power-on, and each measurement carries a status byte
  that reports the warm-up state:

    - For the first ~1 minute the sensor is "not ready" and the formaldehyde output is 0.
    - Between ~1 and ~10 minutes the sensor is ready but "not yet within specifications."
    - After ~10 minutes the sensor is ready and within specifications.

  This sketch reads the status with every measurement and only trusts the formaldehyde value
  once the sensor reports it is within specifications. It also shows how to use reset() to
  return the sensor to a known state at startup.

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

void setup()
{
    Serial.begin(115200);
    Serial.println("SparkFun SFA40 Example 4 - Sensor Status");

    Wire.begin();

    while (mySensor.begin() == false)
    {
        Serial.println("SFA40 not connected, check your wiring!");
        delay(1000);
    }

    Serial.println("SFA40 connected!");

    // Soft reset returns the sensor to its power-on state, so the warm-up timing below starts
    // fresh. The reset is sent as an I2C general call and affects all devices that respond to it.
    mySensor.reset();

    if (mySensor.startMeasurement() != ksfTkErrOk)
    {
        Serial.println("Failed to start measurement. Halting.");
        while (1)
            ;
    }

    delay(1000);

    Serial.println("Reading... (formaldehyde is only valid once 'within spec')");
}

void loop()
{
    if (mySensor.readMeasurement() != ksfTkErrOk)
    {
        Serial.println("Failed to read measurement!");
        delay(1000);
        return;
    }

    // Always safe to read humidity and temperature.
    Serial.print("RH: ");
    Serial.print(mySensor.getHumidity(), 1);
    Serial.print(" %  Temp: ");
    Serial.print(mySensor.getTemperature(), 1);
    Serial.print(" C  |  ");

    // Decide whether the formaldehyde reading can be trusted based on the status.
    if (!mySensor.isReady())
    {
        Serial.println("Sensor warming up (not ready)...");
    }
    else if (!mySensor.isWithinSpecifications())
    {
        Serial.print("Stabilizing (HCHO ~");
        Serial.print(mySensor.getHCHO(), 1);
        Serial.println(" ppb, not yet within spec)");
    }
    else
    {
        Serial.print("HCHO: ");
        Serial.print(mySensor.getHCHO(), 1);
        Serial.println(" ppb");
    }

    delay(1000);
}
