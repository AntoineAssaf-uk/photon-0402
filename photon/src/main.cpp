/*
 * Photon minimal feeder test-bed firmware
 * Section 7 - stripped starting point
 *
 * This firmware intentionally removes:
 * - Photon feeder protocol
 * - RS485 packet layer
 * - FeederFloor / OneWire address logic
 * - motor feed algorithms
 * - peeler logic
 *
 * It keeps:
 * - pin definitions
 * - UART test interface
 * - LED heartbeat
 * - optional software jump to STM32 system bootloader
 */

#include <Arduino.h>

#include "define.h"
#include "bootloader.h"

#ifndef VERSION_STRING
#define VERSION_STRING "section7-minimal-unknown"
#endif

#define TEST_BAUD_RATE 115200

// Original Photon code used HardwareSerial(PA10, PA9).
// In STM32 Arduino this is RX, TX.
HardwareSerial TestSerial(PA10, PA9);

static uint32_t heartbeatLastMs = 0;
static bool heartbeatState = false;

static char rxLine[64];
static uint8_t rxIndex = 0;

static void setRgb(bool r, bool g, bool b) {
    digitalWrite(LED_R, r ? HIGH : LOW);
    digitalWrite(LED_G, g ? HIGH : LOW);
    digitalWrite(LED_B, b ? HIGH : LOW);
}

static void safeMotorOff() {
    // First test-bed rule:
    // do not move anything until each motor function is tested explicitly.
    digitalWrite(DRIVE1, LOW);
    digitalWrite(DRIVE2, LOW);
    digitalWrite(PEEL1, LOW);
    digitalWrite(PEEL2, LOW);

    // Conservative default: keep motor enable low.
    // We will confirm actual enable polarity later before motor tests.
    digitalWrite(MOTOR_ENABLE, LOW);
}

static void printHelp() {
    TestSerial.println();
    TestSerial.println("PHOTON_MINIMAL_TESTBED");
    TestSerial.print("VERSION=");
    TestSerial.println(VERSION_STRING);
    TestSerial.println();
    TestSerial.println("Commands:");
    TestSerial.println("  ?       help");
    TestSerial.println("  v       version");
    TestSerial.println("  led r   red LED");
    TestSerial.println("  led g   green LED");
    TestSerial.println("  led b   blue LED");
    TestSerial.println("  led off LEDs off");
    TestSerial.println("  boot    jump to STM32 system bootloader");
    TestSerial.println();
}

static bool equalsCommand(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static void handleCommand(char *cmd) {
    // Trim leading spaces.
    while (*cmd == ' ' || *cmd == '\t') {
        cmd++;
    }

    // Trim trailing CR/LF/spaces.
    size_t len = strlen(cmd);
    while (len > 0 &&
           (cmd[len - 1] == '\r' ||
            cmd[len - 1] == '\n' ||
            cmd[len - 1] == ' ' ||
            cmd[len - 1] == '\t')) {
        cmd[len - 1] = '\0';
        len--;
    }

    if (equalsCommand(cmd, "?")) {
        printHelp();
    }
    else if (equalsCommand(cmd, "v")) {
        TestSerial.print("VERSION=");
        TestSerial.println(VERSION_STRING);
    }
    else if (equalsCommand(cmd, "led r")) {
        setRgb(true, false, false);
        TestSerial.println("OK LED RED");
    }
    else if (equalsCommand(cmd, "led g")) {
        setRgb(false, true, false);
        TestSerial.println("OK LED GREEN");
    }
    else if (equalsCommand(cmd, "led b")) {
        setRgb(false, false, true);
        TestSerial.println("OK LED BLUE");
    }
    else if (equalsCommand(cmd, "led off")) {
        setRgb(false, false, false);
        TestSerial.println("OK LED OFF");
    }
    else if (equalsCommand(cmd, "boot")) {
        TestSerial.println("OK BOOTLOADER");
        TestSerial.flush();
        delay(100);
        reboot_into_bootloader();
    }
    else if (len == 0) {
        // Ignore empty line.
    }
    else {
        TestSerial.print("ERR UNKNOWN COMMAND: ");
        TestSerial.println(cmd);
    }
}

static void pollSerial() {
    while (TestSerial.available() > 0) {
        char c = (char) TestSerial.read();

        if (c == '\n' || c == '\r') {
            rxLine[rxIndex] = '\0';
            handleCommand(rxLine);
            rxIndex = 0;
        }
        else {
            if (rxIndex < sizeof(rxLine) - 1) {
                rxLine[rxIndex++] = c;
            }
            else {
                rxIndex = 0;
                TestSerial.println("ERR LINE TOO LONG");
            }
        }
    }
}

static void heartbeat() {
    uint32_t now = millis();

    if (now - heartbeatLastMs >= 500) {
        heartbeatLastMs = now;
        heartbeatState = !heartbeatState;

        // Blue heartbeat while idle.
        digitalWrite(LED_B, heartbeatState ? HIGH : LOW);
    }
}

void setup() {
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);

    pinMode(SW1, INPUT_PULLUP);
    pinMode(SW2, INPUT_PULLUP);

    pinMode(MOTOR_ENABLE, OUTPUT);
    pinMode(DRIVE1, OUTPUT);
    pinMode(DRIVE2, OUTPUT);
    pinMode(PEEL1, OUTPUT);
    pinMode(PEEL2, OUTPUT);

    safeMotorOff();

    setRgb(false, true, false);
    delay(200);
    setRgb(false, false, false);

    TestSerial.begin(TEST_BAUD_RATE);
    delay(100);

    printHelp();
}

void loop() {
    heartbeat();
    pollSerial();
}