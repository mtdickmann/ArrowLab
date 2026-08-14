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
interrupt, so ArrowLab does not use it for this link. GPIO17 is exposed at
VIEWE connector J7 and is not claimed by display or touch.
It is the production measurement-link signal. The proven GPIO11/GPIO12 pair is
retained only as a compile-time fallback; alternative pin-outs are engineering
options rather than parallel connections.

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

### VIEWE to WROOM — production link

The measurement link is a dedicated half-duplex, single-wire UART. Both
processors route UART1 RX and TX to the same open-drain pad. The VIEWE polls
for every reply, so the processors do not drive the wire simultaneously.

| Signal | VIEWE | WROOM |
| --- | ---: | ---: |
| Bidirectional UART data | GPIO17 (J7) | GPIO8 |
| Reference | GND | GND |

Confirm the GPIO labels before soldering; do not infer a pad from physical
position alone. There is one data conductor, not crossed TX/RX conductors. Fit
a 4.7 kOhm pull-up between WROOM 3.3 V and the shared GPIO8/GPIO17 signal node.

During initial development both boards are powered from their own USB cables.
Connect only the single UART signal and common GND between them. Do **not** join
their 3.3 V or 5 V rails while both USB supplies are connected. The WROOM
alone supplies the two HX711 modules.

J7 also exposes 5 V, GPIO0 and GND. GPIO0 is a boot-strapping signal and is not
assigned to ArrowLab. J7 5 V may support a future single-inlet power design,
but it is not part of the present wiring and must not be joined to the WROOM
until that architecture is deliberately designed and validated.

The current UART configuration is:

- 115200 baud, 8 data bits, no parity, one stop bit;
- VIEWE: shared RX/TX GPIO17;
- WROOM: shared RX/TX GPIO8;
- VIEWE polls at 50 ms intervals and the WROOM replies after a guarded
  turnaround;
- checked binary protocol version 1;
- invalid, truncated or checksum-failed packets are ignored;
- loss of valid packets for 1.5 seconds raises the persistent `NODE OFFLINE`
  fault on the HMI.

## Firmware configuration

`include/ArrowLab.conf` is the printer.cfg-style, human-editable source of truth
for installation and development choices shared by both processors. Set the
numeric `CONFIGURED_MEASUREMENT_LINK_MODE` documented in that file, then rebuild
and upload **both** the VIEWE and WROOM. Invalid values are rejected at compile
time. The implementation and strongly typed pin-selection logic deliberately
remain in `include/ArrowLabConfig.h`; they are not user settings. Do not connect
more than one of the following link options at a time.

| Mode | VIEWE pins | WROOM pins | Consequence |
| --- | --- | --- | --- |
| `0` | RX 11, TX 12 | RX 8, TX 9 | Proven fallback; VIEWE onboard SD MOSI/SCK are occupied |
| `1` | RX/TX 17 | RX/TX 8 | **Production**; one signal wire plus GND and a 4.7 kOhm pull-up to 3.3 V |
| `2` | RX 44, TX 43 | RX 8, TX 9 | **Untested**; electrically shared with VIEWE UART0/CH340 |

The one-wire mode is not the two-wire protocol with a conductor removed. The
VIEWE becomes bus master and requests each status packet. Both UART pads are
explicitly open-drain, and the WROOM never broadcasts without a request. This
prevents electrical contention. The external pull-up is mandatory.

ArrowLab currently pins Arduino-ESP32 3.1.1. That core predates its later
same-pin UART support: assigning identical RX and TX pins through
`HardwareSerial` does not retain both GPIO-matrix routes. ArrowLab therefore
uses the shared `OneWireUart` helper after `HardwareSerial::begin()` to set the
pad open-drain and explicitly route both UART1 TX and UART1 RX to it. Do not
replace that helper with a second ordinary pin assignment; doing so can report
a successful write while leaving the shared wire permanently high.

`CONFIGURED_DEVELOPER_MODE_DEFAULT_ENABLED` in `ArrowLab.conf` controls whether
Diagnostics is visible at boot. `false` preserves the production long-press
reveal; `true` makes the developer menu immediately visible while firmware is
under test.

Three additional development controls avoid firmware edits while operational
weighing behaviour and presentation are being evaluated:

- `CONFIGURED_OPERATIONAL_WEIGHING_TIME_MS` controls only the acquisition time
  after an ordinary mass change. It does not alter the fixed 30-second
  calibration interval.
- `CONFIGURED_PRIMARY_MASS_UNIT` selects `g`, `gr` or avoirdupois `oz` as the
  large reading. The other two units remain visible underneath as reference
  conversions.
- `CONFIGURED_MASS_DECIMAL_PLACES` controls the displayed precision from zero
  to three decimal places. It does not change raw acquisition, K or the
  internal mass calculation.

Because the acquisition setting is consumed by the WROOM and the display
settings by the VIEWE, rebuild and upload both processors after changing this
shared file.

For the GPIO17 production link, place the 4.7 kOhm resistor between WROOM
3.3 V and the shared GPIO8/GPIO17 signal node; it is a pull-up, not a series
resistor. Provide a removable jumper or plug in the signal conductor. Disconnect
that signal while uploading the processors separately, then power both off,
reconnect it and power the system normally. The shared ground may remain fitted.

GPIO11 and GPIO12 are respectively MOSI and SCK for the VIEWE onboard SD slot.
They were proven as the two-wire fallback, but are not connected in the
production build. WROOM GPIO9 is likewise unused by production mode. Two
additional WROOM GPIOs are to be reserved for the proposed third 100 g
load-cell/HX711 channel; their final pin assignments will be made only when
those peripherals are implemented.

### GPIO43/GPIO44 untested option and test safety

GPIO43/GPIO44 has not been bench validated for ArrowLab and is not an approved
production or fallback connection.

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
