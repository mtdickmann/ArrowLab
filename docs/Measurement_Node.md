# ArrowLab Measurement Node

ArrowLab uses a second ESP32-S3 as the dedicated measurement and physical-I/O
processor. The VIEWE display ESP32-S3 remains responsible for UI, touch,
storage, networking and high-level application logic.

The first firmware stage is intentionally a raw HX711 diagnostic. It is used
to test load cells and HX711 modules without the VIEWE board, LVGL,
calibration, filtering, held readings or drift compensation influencing the
result.

## Development board

- Module: ESP32-S3-WROOM-1 N16R8
- Flash: 16 MB
- PSRAM: 8 MB
- PlatformIO environment: `ARROWLAB_MEASUREMENT_S3`
- Serial monitor: 115200 baud

## Raw diagnostic wiring

Power the S3-WROOM development board from USB. Power each HX711 from the
WROOM's 3.3 V and GND pins. Do not connect the VIEWE display during this test.

| Function | WROOM GPIO |
| --- | ---: |
| Left HX711 DT | 4 |
| Left HX711 SCK | 5 |
| Right HX711 DT | 6 |
| Right HX711 SCK | 7 |
| HX711 VCC | 3.3 V |
| HX711 GND | GND |

One channel may be connected and tested by itself; a disconnected DT input is
held high so it does not report false data.

## Build and upload

The normal ArrowLab display environment remains the project's default. In
PlatformIO Project Tasks choose `ARROWLAB_MEASUREMENT_S3` when building or
uploading the WROOM firmware.

Command-line equivalents are:

```text
pio run -e ARROWLAB_MEASUREMENT_S3
pio run -e ARROWLAB_MEASUREMENT_S3 -t upload
pio device monitor -b 115200
```

When both ESP32 boards are attached to the PC, verify the intended COM port
before uploading. Do not assume the WROOM will use the same COM number after
reconnection.

## Diagnostic output

Each fresh conversion is printed without filtering:

```text
AL_NODE,DATA,millis,side,raw,delta
```

`raw` is the signed HX711 ADC count. `delta` is the difference from that
channel's current diagnostic baseline. The first valid sample automatically
becomes the initial baseline.

Serial commands:

- `BL` / `BR` - make the latest Left/Right raw value the baseline.
- `BB` - set both baselines from the latest readings.
- `RL` / `RR` - clear one baseline; the next sample becomes the new baseline.
- `RB` - clear both baselines.
- `?` - show command help.

These diagnostic baselines are not ArrowLab tare values and are never stored.
The purpose of this mode is to expose the hardware honestly.

## Planned permanent link

After independent HX711/load-cell behaviour is established, the measurement
node will own both HX711 interfaces and future timing-sensitive physical I/O.
It will exchange completed measurements and commands with the VIEWE processor
over I2C. This returns VIEWE GPIO10-13 to the onboard SD-card interface and
keeps GPIO17 as the only reserved spare VIEWE GPIO.

The planned inter-processor link is SDA, SCL and common GND. During USB-powered
development the two boards must not have their separate 3.3 V outputs tied
together.
