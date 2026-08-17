# 67

A physical throw-and-catch party game built on the [Waveshare ESP32-S3-Touch-AMOLED-2.06](https://www.waveshare.com/esp32-s3-touch-amoled-2.06.htm).

Throw the device to someone. While it's in the air, the screen flickers through random numbers. The instant it's caught, the number locks in — 1 in 5 times it lands on **67** and the screen flashes bright red. Whoever's holding it then loses.

## How it works

The onboard QMI8658 accelerometer detects true freefall (measured acceleration reads ~0g when genuinely airborne, since gravity and the sensor accelerate together) to know when the device has been thrown, and a sharp acceleration spike to detect the catch. No buttons, no touch input — just motion.

## Hardware

- Waveshare ESP32-S3-Touch-AMOLED-2.06 (410×502 AMOLED, ESP32-S3R8, QMI8658 6-axis IMU)

It's a bare dev board with a glass touchscreen, not a toy built for hard impacts — treat throws as a gentle toss/pass rather than a real throw until you know how it holds up.

## Build & flash

Arduino IDE or `arduino-cli`, ESP32 board package ≥3.2.0 ([espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)), plus Waveshare's `Arduino_GFX`, `SensorLib`, and `Mylibrary` (pin definitions) from their [official demo repo](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-2.06).

Board settings:
- PSRAM: OPI PSRAM
- USB CDC On Boot: Enabled
- Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)

```
arduino-cli compile -u -p <port> -b "esp32:esp32:esp32s3:PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB,FlashSize=32M" .
```

## Tuning

| Constant | Meaning |
|---|---|
| `FREEFALL_G` | acceleration magnitude below this counts as airborne |
| `FREEFALL_MIN_MS` | how long it must stay airborne to count as a real throw, not a jostle |
| `CATCH_G` | acceleration spike above this while airborne counts as a catch |
| `AIRBORNE_TIMEOUT_MS` | forces a reveal if no clean catch-spike registers (soft lob, etc.) |
| `FLICKER_MS` | how often the number changes while airborne |
| `LOSE_CHANCE` | 1 in this many catches lands on 67 |
