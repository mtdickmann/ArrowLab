# Creep Test Logger — Operator Procedure

ArrowLab's hidden developer tool measures zero stability and time-under-load creep independently for the Left and Right load-cell/HX711 channels.

Each run records raw count, tare-adjusted count and supporting gram conversion at t=0, t=10 seconds, then every 30 seconds through 30 minutes. Raw counts are the primary evidence.

## PC preparation

Only one program may own ArrowLab's serial port. Close PlatformIO Serial Monitor before starting the logger.

On the normal PlatformIO development PC, use PlatformIO's Python:

    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" --version
    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" -c "import serial; print(serial.__version__)"
    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" tools\capture_creep.py COM3

Replace `COM3` when required. If PlatformIO's Python is missing, repair PlatformIO. A standalone PC may use current Python 3 plus `python -m pip install pyserial`; do not assume the Windows `py` launcher exists.

The logger creates:

    calibration\diagnostics\creep_LHS_YYYYMMDD_HHMMSS.csv

or the equivalent `RHS` filename. The side is assigned when the first
diagnostic sample arrives. Keep one logger session to one selected channel.

Leave the logger open for the selected channel's runs. The logger and firmware exchange heartbeats; ArrowLab will not start a run without a real PC handshake.

## Enter the diagnostic

1. On Home, long-press the ArrowLab title to reveal Developer mode.
2. Open Settings -> Diagnostics -> Load-cell creep test.
3. Select Left or Right. All state and actions now refer only to that channel.

Left and Right are deliberately independent. A replacement or retest on one side does not erase the other side.

## New or reset channel

The required order is:

1. `ZERO BASE`
2. `TARE`
3. `CAL` when calibration is required or deliberately being renewed. After a
   platform tare, the first CAL press opens actual reference-mass entry. Place
   that weight when instructed; the second CAL press starts the fixed 30-second
   stabilization and calibration.
4. one or more `LOAD TEST` runs

The instruction line states the next operator action. The state line says what
ArrowLab currently knows, and the progress bar shows acquisition or calibration
settling. Timing details live here and under Help rather than competing with the
next action on the working screen.

### 1. Zero baseline

1. Remove the calibration platform and every added weight. Leave the fixed arrow-rest hardware installed; its mass is part of the normal mechanism and does not need to be measured.
2. Press `ZERO BASE` and confirm.
3. ArrowLab verifies the PC logger, automatically tares the selected channel and starts the 30-minute run.
4. Do not pause or disturb the mechanism.
5. At 30 minutes ArrowLab buffers the complete run, replays it to the PC and waits for the PC save acknowledgement.
6. `BASE OK` appears only after all 62 samples are acknowledged as saved.

If USB disconnects, leave ArrowLab powered. The run continues in RAM and is replayed after the logger reconnects. Do not start another run until the current run reports saved.

### 2. Calibration

1. Fit the empty calibration platform to the selected channel.
2. Press `TARE` and confirm.
3. Press `CAL`, enter the known reference mass, and confirm the entry.
4. Place that reference mass centrally when instructed.
5. When `CAL READY` appears, press `CAL` again.
6. Leave the weight untouched during the 30-second stabilization and automatic
   calibration sampling.

During stabilization the screen counts down and shows progress. `CAL READY`
means load presence is confirmed and the second CAL action may be pressed.
`CAL OK` and a green, still-actionable CAL button mean a factor is already
stored. Normal Calibration remains independently available at any time; the
diagnostic screen merely reuses the same tare and calibration routines.

Calibration stores counts-per-gram for the selected channel in ESP32 non-volatile storage. It survives normal power cycles. A firmware version change deliberately invalidates it so the user must recalibrate; the other channel remains independent.

The zero-baseline record and calibration factor are different things. A baseline characterises drift. Calibration converts counts to grams. Calibration performed here uses the same `CalibrationController` as the normal Settings > Calibration screen and is therefore stored persistently for the selected channel; the diagnostic does not own a second calibration implementation.

### 3. Loaded run

1. With the empty calibration platform fitted, press `SET MASS` and enter the separately measured added mass (maximum 1850 g).
2. Keep that mass off the platform.
3. Press `LOAD TEST` and confirm. ArrowLab performs a fresh diagnostic tare.
4. When instructed, place the mass centrally. Five consecutive readings above the load threshold start timing automatically.
5. Leave the mechanism untouched until the PC acknowledges the complete run.
6. Remove the load and repeat for any further masses.

Recommended first evidence set per channel is the mandatory 0 g baseline, an approximately 20 g arrow-range run and an approximately 1000 g run. The 1806 g engineering reference may then be used if the platform is stable and the total physical load remains within the 2 kg channel capacity.

## Resetting one channel

`RESET` clears only the selected channel's stored baseline flag and calibration. Use it after replacing a load cell/HX711 or when deliberately restarting that side's characterisation. The other side is untouched.

## Cancel, recovery and finish

- `CANCEL` is for an interrupted active acquisition. Remove the load and repeat the entire run; physical creep cannot be paused honestly.
- Between acknowledged runs, the operator may wait indefinitely or close/restart the PC logger.
- `DONE` emits `SESSION_COMPLETE`, which closes the PC CSV cleanly, and returns to the selected-channel workflow.
- `Ctrl+C` closes the logger manually. Completed/acknowledged rows remain in the CSV.
- Do not power-cycle ArrowLab while it holds an unacknowledged RAM buffer; RAM recovery cannot survive loss of power.

## Pitfalls

- Keep PlatformIO Serial Monitor closed during logging.
- Prevent the PC from sleeping during an active run. Automatic reconnect handles a serial interruption, but sleep timing is still undesirable test evidence.
- Do not upload firmware during a run.
- Do not edit or smooth the original CSV.
- Record bench bumps, shifted masses, temperature changes or other disturbances.
- Generated `creep_*.csv` files are ignored by Git. Deliberately selected evidence may be added with `git add -f <file>` after review.

## Analysis boundary

Firmware preserves unsmoothed diagnostic evidence. Operational readings use a
small robust filter, but no fixed time-based creep correction is applied because
the observed drift changes both magnitude and direction between runs. See
`docs/Creep_Data_Analysis.md` for the evidence and resulting measurement policy.
