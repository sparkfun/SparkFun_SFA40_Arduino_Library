/*
  Example 03 - Self-test

  The SFA40 has a built-in self-test that checks the integrity of the electrical connections
  between the user interface and the sensor cell. It is useful for detecting problems such as
  bad soldering, and Sensirion recommends running it once after the sensor is soldered to a PCB.

  The self-test runs autonomously for 5 to 6 minutes. This sketch starts it and then polls for
  the result every 10 seconds until the test reports that it has finished.

  Result interpretation:
    0xFFFF : still running (keep waiting)
    0x0000 : finished and passed
    other  : finished and failed

  Note: The self-test must be started from idle state. begin() leaves the sensor idle.

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

// The self-test reports this value while it is still running.
const uint16_t kSelfTestStillRunning = 0xFFFF;
const uint16_t kSelfTestPassed = 0x0000;

void setup()
{
    Serial.begin(115200);
    Serial.println("SparkFun SFA40 Example 3 - Self-test");

    Wire.begin();

    while (mySensor.begin() == false)
    {
        Serial.println("SFA40 not connected, check your wiring!");
        delay(1000);
    }

    Serial.println("SFA40 connected!");

    // Start the self-test. This puts the sensor into a special measurement mode.
    if (mySensor.startSelfTest() != ksfTkErrOk)
    {
        Serial.println("Failed to start self-test. Halting.");
        while (1)
            ;
    }

    Serial.println("Self-test started. This takes 5 to 6 minutes, please wait...");

    // Poll for the result every 10 seconds until the test finishes.
    uint16_t result = kSelfTestStillRunning;
    while (result == kSelfTestStillRunning)
    {
        delay(10000);

        if (mySensor.readSelfTestResult(result) != ksfTkErrOk)
        {
            Serial.println("Failed to read self-test result!");
            continue;
        }

        if (result == kSelfTestStillRunning)
            Serial.println("  ...still running");
    }

    // Report the outcome.
    if (result == kSelfTestPassed)
    {
        Serial.println("Self-test PASSED.");
    }
    else
    {
        Serial.print("Self-test FAILED. Result code: 0x");
        Serial.println(result, HEX);
    }
}

void loop()
{
    // Nothing to do here.
}
