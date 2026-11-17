# CC1120 RF link test

Arduino transmit/receive test for a Seeed Studio XIAO ESP32-S3 connected over SPI to a TI CC1120 radio. The original sketch configures a 434 MHz link and sends `HELLO kuku` about once per second.

## Open and run

1. Install Espressif's `esp32` board package in Arduino IDE. The recovered build used version **3.3.12**.
2. Open `cc1120_rf_link/cc1120_rf_link.ino` and select **XIAO_ESP32S3**.
3. Set `#define ROLE_TX 1` and upload to the transmitter.
4. Change that line to `#define ROLE_TX 0` and upload to the receiver.
5. Open the Serial Monitor at **115200 baud** to see initialization and packet diagnostics.

Both radios need the same radio configuration. The sketch contains the original register values, calibration, and FIFO handling.

## SPI wiring used by the sketch

| CC1120 signal | XIAO pin | ESP32-S3 GPIO |
| --- | --- | --- |
| SCLK | D8 | GPIO7 |
| MISO | D9 | GPIO8 |
| MOSI | D10 | GPIO9 |
| CS | D3 | GPIO4 |
| RESET | D2 | GPIO3 |

## Compile from the command line

With the `esp32:esp32` board package installed:

```sh
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 cc1120_rf_link
```

## Recovery

Recovered from Arduino's cached C++ output for `sketch_oct2a`, last modified October 2, 2026. The original unsaved `.ino` was no longer present. Recovery removed Arduino-generated line directives, the injected `Arduino.h` include, and generated function declarations; the radio code and behavior were preserved.

Both transmitter and receiver modes compile successfully with Arduino CLI 1.5.1 and `esp32:esp32` 3.3.12. The regenerated transmitter C++ matches the cached source after excluding line directives and blank lines.

Hardware transmission and reception have not been tested as part of this recovery.
