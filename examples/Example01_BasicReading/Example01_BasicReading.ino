/*
  Example 01 - Basic Reading

  The simplest way to read the SparkFun Qwiic Formaldehyde Sensor (SFA40). This sketch
  starts continuous measurement, then reads the formaldehyde concentration, relative
  humidity, and temperature about once per second.

  Note: The SFA40 needs to warm up after power-on. The formaldehyde output is forced to 0
  for the first minute, and is only within specifications after about 10 minutes. See
  Example04 for a status-aware version that reports the warm-up state.

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
    // Start serial right away so we can report what is happening.
    Serial.begin(115200);
    Serial.println("SparkFun SFA40 Example 1 - Basic Reading");

    // Start I2C communication.
    Wire.begin();

    // Attempt to connect to the sensor. Keep trying so the message is not missed if the
    // Serial Monitor is opened late.
    while (mySensor.begin() == false)
    {
        Serial.println("SFA40 not connected, check your wiring!");
        delay(1000);
    }

    Serial.println("SFA40 connected!");

    // Bring the sensor out of idle and into continuous measurement mode.
    if (mySensor.startMeasurement() != ksfTkErrOk)
    {
        Serial.println("Failed to start measurement. Halting.");
        while (1)
            ;
    }

    // Wait for the first data point to become available (the sensor updates every ~0.5 s).
    delay(1000);

    Serial.println("HCHO (ppb)\tHumidity (%RH)\tTemperature (C)");
}

void loop()
{
    // Read a fresh data point. This validates the CRC of every value before storing it.
    if (mySensor.readMeasurement() == ksfTkErrOk)
    {
        Serial.print(mySensor.getHCHO(), 1);
        Serial.print("\t\t");
        Serial.print(mySensor.getHumidity(), 1);
        Serial.print("\t\t");
        Serial.println(mySensor.getTemperature(), 1);
    }
    else
    {
        Serial.println("Failed to read measurement!");
    }

    // The SFA40 updates internally about every 0.5 s; reading once per second is plenty.
    delay(1000);
}
