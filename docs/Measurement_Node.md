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

## Controlled 30-minute hardware capture

Use the terminal logger when comparing load cells, HX711 modules, cassettes and
bench-mounted assemblies. Close PlatformIO Serial Monitor first because only
one program may own the WROOM COM port.

From the ArrowLab project directory run:

```text
"%USERPROFILE%\.platformio\penv\Scripts\python.exe" tools\capture_measurement_node.py COM3
```

Replace `COM3` if Windows assigns a different port. The logger asks for:

- the load-cell number;
- the HX711 number;
- the cassette number, or `NONE` for a bench-mounted load cell;
- WROOM GPIO set A (DT 4 / SCK 5) or B (DT 6 / SCK 7); and
- the applied test mass in grams, where `0` means no added test load.

After the variables are confirmed, connect the identified assembly and press
Enter. There is no second Start command. The first valid HX711 data received on
the chosen GPIO set is the connection event and starts the run automatically.

The logger retains the first 20 raw conversions as `CONNECT` rows and uses
their mean as the raw reference. `TIMED` rows are then recorded at 0 seconds,
10 seconds, 30 seconds and every 30 seconds through 30 minutes. This produces
the same 62-point timed series used by the original ArrowLab creep diagnostic,
while also preserving the connection burst for short-term noise analysis.

CSV files are written to `calibration/diagnostics/` with the load cell, HX711,
cassette and GPIO pair in the filename. Every CSV row repeats those identifiers
and the applied mass, so a file remains self-describing if it is renamed.

`Ctrl+C` closes the serial port and retains a partial CSV. A complete run closes
the CSV and serial port automatically after the 1800-second reading.

## Planned permanent link

After independent HX711/load-cell behaviour is established, the measurement
node will own both HX711 interfaces and future timing-sensitive physical I/O.
It will exchange completed measurements and commands with the VIEWE processor
over I2C. This returns VIEWE GPIO10-13 to the onboard SD-card interface and
keeps GPIO17 as the only reserved spare VIEWE GPIO.

The planned inter-processor link is SDA, SCL and common GND. During USB-powered
development the two boards must not have their separate 3.3 V outputs tied
together.
