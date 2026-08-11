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
interrupt, so ArrowLab does not use it for this link. The proven hardware
configuration deliberately reuses GPIO11 and GPIO12, formerly the stable
right-HX711 pair, for the dedicated measurement-node UART. Alternative link
pin-outs are compile-time engineering options rather than parallel
connections.

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

### VIEWE to WROOM — proven default

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

## Firmware configuration

`include/ArrowLab.conf` is the printer.cfg-style, human-editable source of truth
for installation and development choices shared by both processors. Select
`CONFIGURED_MEASUREMENT_LINK_MODE` there, then rebuild and upload **both** the
VIEWE and WROOM. The implementation and pin-selection logic deliberately remain
in `include/ArrowLabConfig.h`; they are not user settings. Do not connect more
than one of the following link options at a time.

| Mode | VIEWE pins | WROOM pins | Consequence |
| --- | --- | --- | --- |
| `VieweGpio11And12` | RX 11, TX 12 | RX 8, TX 9 | Proven default; VIEWE onboard SD MOSI/SCK are occupied |
| `VieweGpio17OneWire` | RX/TX 17 | RX/TX 8 | Experimental half-duplex; one signal wire plus GND and a 4.7 kOhm pull-up to 3.3 V |
| `VieweGpio43And44` | RX 44, TX 43 | RX 8, TX 9 | Experimental; electrically shared with VIEWE UART0/CH340 |

The one-wire mode is not the two-wire protocol with a conductor removed. The
VIEWE becomes bus master and requests each status packet. Both UART pads are
explicitly open-drain, and the WROOM never broadcasts without a request. This
prevents electrical contention. The external pull-up is mandatory.

`CONFIGURED_DEVELOPER_MODE_DEFAULT_ENABLED` in `ArrowLab.conf` controls whether
Diagnostics is visible at boot. `false` preserves the production long-press
reveal; `true` makes the developer menu immediately visible while firmware is
under test.

For GPIO17 one-wire development, place the 4.7 kOhm resistor between WROOM
3.3 V and the shared GPIO8/GPIO17 signal node; it is a pull-up, not a series
resistor. Provide a removable jumper or plug in the signal conductor. Disconnect
that signal while uploading the processors separately, then power both off,
reconnect it and power the system normally. The shared ground may remain fitted.

GPIO11 and GPIO12 are respectively MOSI and SCK for the VIEWE onboard SD slot.
The slot is therefore unavailable only while the proven default link is
selected. If that remains the production link, an external SD module can be
placed on spare WROOM GPIOs later. Two additional WROOM GPIOs are also to be
reserved for the proposed third 100 g load-cell/HX711 channel; their final pin
assignments will be made only when those peripherals are implemented.

### GPIO43/GPIO44 test safety

Closing a serial monitor releases its Windows COM-port handle but does **not**
electrically disconnect the onboard CH340. GPIO44 is the VIEWE UART0 receive
line and is already driven by CH340 TX whenever that interface is powered.
Connecting WROOM TX to it at the same time creates two push-pull transmitters
on one conductor and risks damage.

For a controlled GPIO43/GPIO44 trial:

1. Leave the inter-processor signal wires disconnected while uploading both
   firmwares.
2. Disconnect the VIEWE USB/CH340 interface completely.
3. Power the VIEWE through a board-approved supply that does not energize the
   CH340 data interface.
4. Connect VIEWE TX 43 to WROOM RX 8, WROOM TX 9 to VIEWE RX 44, and common
   GND.
5. Test from the touchscreen. VIEWE PC logging is unavailable in this mode.
6. Disconnect the inter-processor wires before reconnecting VIEWE USB for the
   next upload.

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

Opening a PlatformIO monitor does not make its COM port part of the measurement
link. It merely gives the PC process exclusive access to that USB serial port.
Press `Ctrl+C` or close the monitor terminal to release the port for another
program. The WROOM firmware and the VIEWE-WROOM link continue running.

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
