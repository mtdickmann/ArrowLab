# ArrowLab Measurement Node

ArrowLab uses an ESP32-S3-WROOM-1 N16R8 as its dedicated metrology processor.
The VIEWE ESP32-S3 is the HMI: display, touch, navigation and user commands.
The WROOM owns both HX711 interfaces, all measurement maths, tare, calibration
and persistent calibration factors.

This boundary is intentional. No HX711 conversion, tare offset, calibration
factor or held mass is calculated independently on the display processor.

## VIEWE pin-map correction

Only the active supported-board definition is authoritative. The disabled
generic `esp_panel_board_custom_conf.h` template does not describe this board
and must not be used to identify pin conflicts. In the active VIEWE
UEDX48270043E-WB-A definition, GPIO8 is RGB DATA0. GPIO18 is not claimed by
the software driver, but the board hardware routes it to the GT911 touch
interrupt, so ArrowLab does not use it for this link. The current hardware
trial deliberately reuses GPIO11 and GPIO12, formerly the stable right-HX711
pair, for the dedicated measurement-node UART.

The HX711s nevertheless remain on the WROOM. This keeps metrology, display and
touch workloads isolated and gives the instrument one measurement authority.

## Production wiring

### HX711s to WROOM

| Function | WROOM pin |
| --- | ---: |
| Left HX711 DT | GPIO4 |
| Left HX711 SCK | GPIO5 |
| Right HX711 DT | GPIO6 |
| Right HX711 SCK | GPIO7 |
| Both HX711 VCC | WROOM 3.3 V |
| Both HX711 GND | WROOM GND |

### VIEWE to WROOM

The measurement link uses a dedicated full-duplex UART. It does not share the
GT911 touch-controller I2C bus.

| Signal | VIEWE | WROOM |
| --- | ---: | ---: |
| VIEWE TX -> WROOM RX | GPIO12 | GPIO8 |
| WROOM TX -> VIEWE RX | GPIO11 | GPIO9 |
| Reference | GND | GND |

Confirm the GPIO labels before soldering; do not infer a pad from physical
position alone. The two signal wires are crossed by function: TX always goes
to the other processor's RX.

During initial development both boards are powered from their own USB cables.
Connect only the two UART signals and common GND between them. Do **not** join
their 3.3 V or 5 V rails while both USB supplies are connected. The WROOM
alone supplies the two HX711 modules.

The current UART configuration is:

- 115200 baud, 8 data bits, no parity, one stop bit;
- VIEWE: RX GPIO11, TX GPIO12;
- WROOM: RX GPIO8, TX GPIO9;
- status packets are sent by the WROOM every 50 ms;
- checked binary protocol version 1;
- invalid, truncated or checksum-failed packets are ignored;
- loss of valid packets for 1.5 seconds raises the persistent `NODE OFFLINE`
  fault on the HMI.

## Firmware ownership

The WROOM runs the same production classes previously validated on the VIEWE:

- `LoadCellChannel` reads signed raw HX711 conversions;
- `MeasurementChannel` owns tare, drift tracking, change acquisition and the
  held result;
- `CalibrationController` owns load detection, the 30-second stabilization,
  calibration sampling and K calculation;
- `InstrumentStorage` stores independent Left and Right K values in WROOM NVS.

The VIEWE sends only deliberate commands: TARE, prepare calibration with the
entered reference mass, and start calibration. It receives raw evidence,
held readings, units, calibration stages, progress and health flags. The
existing calibration screen therefore behaves the same while execution moves
to the metrology processor.

Calibration factors previously stored on the VIEWE are not copied to the
WROOM. Perform one fresh calibration on each side after installing this
architecture. Thereafter K survives ordinary WROOM power cycles and is
invalidated by the existing firmware-version compatibility rule.

## Build and upload

Two firmware uploads are required.

1. Select `ARROWLAB_MEASUREMENT_S3` and upload to the WROOM COM port.
2. Select `BOARD_VIEWE_UEDX48270043E_WB_A` and upload to the VIEWE COM port.
3. Remove power, connect the crossed UART signals and common GND, then power
   both boards. Their power-up order does not matter; the WROOM continuously
   publishes status packets and the VIEWE synchronizes when valid packets
   arrive.
4. Confirm the Home screen reports both channels online.
5. Open Settings -> Calibration, fit the platform, TARE and calibrate Left and
   Right using the normal guided procedure.

Command-line equivalents:

```text
pio run -e ARROWLAB_MEASUREMENT_S3 -t upload
pio run -e BOARD_VIEWE_UEDX48270043E_WB_A -t upload
```

When both boards are attached, always confirm which COM port belongs to which
processor before uploading.

## Bench diagnostic history

The `tools/capture_measurement_node.py` logger and the self-describing WROOM
CSV files remain engineering evidence and a reusable hardware-isolation tool.
The controlled schedule is connection samples followed by 0 s, 10 s, 30 s and
every 30 seconds through 30 minutes. It established that the isolated WROOM
results were stable and that GPIO4/5 and GPIO6/7 behaved equivalently.

The production measurement-node firmware emits `AL_NODE,DATA` raw lines only
after the logger sends `STREAM ON`; the tool sends `STREAM OFF` when it closes.
Normal operation therefore carries no continuous USB-printing workload. This
diagnostic stream never changes tare, K or the UART result supplied to the HMI.
