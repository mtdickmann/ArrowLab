# Creep Test Logger — Operator Procedure

ArrowLab's hidden developer tool records raw HX711 behaviour independently for
Left and Right. It is an evidence logger, not part of normal weighing maths.

## PC preparation

Only one program may own the serial port. Close PlatformIO Serial Monitor, then
from the ArrowLab repository run:

    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" --version
    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" -c "import serial; print(serial.__version__)"
    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" tools\capture_creep.py COM3

Replace `COM3` if required. If PlatformIO's Python is absent, repair PlatformIO.
A standalone PC may use Python 3 plus `python -m pip install pyserial`; do not
assume the Windows `py` launcher exists.

The logger creates an LHS- or RHS-labelled file under
`calibration\diagnostics`. Keep one PC logger session to one selected channel.
Generated creep CSVs are ignored by Git unless deliberately force-added as
reviewed evidence.

## Enter the diagnostic

1. On Home, long-press the ArrowLab title to reveal Developer mode.
2. Open Settings -> Diagnostics -> Load-cell creep test.
3. Select Left or Right.
4. Start the PC logger and wait for `PC LOGGER CONNECTED`.

Channel selection is explicit. No baseline, tare or calibration prerequisite is
imposed on a raw diagnostic run.

## Zero run

1. Remove all added test masses. Leave the normal fixed hardware in place.
2. Press `ZERO BASE` and confirm.
3. ArrowLab captures a private 20-sample raw reference, then records t=0, t=10
   seconds and every 30 seconds through 30 minutes.
4. Do not touch or pause the mechanism.
5. Wait until the PC acknowledges all 62 buffered samples.

`RUN SAVED` confirms that the complete evidence reached the PC. Firmware does
not persist a baseline flag because the CSV—not an ESP state bit—is the record.

## Loaded run

1. Remove the test mass and allow the mechanism to return to its unloaded
   physical condition.
2. Press `SET MASS` and enter the separately measured added mass, up to 1850 g.
3. Keep the mass off the platform and press `LOAD TEST`.
4. ArrowLab captures a private 20-sample unloaded raw reference.
5. When instructed, place the entered mass centrally. Five consecutive samples
   beyond 2,000 counts start the clock automatically.
6. Leave the mechanism untouched until the PC acknowledges all 62 samples.
7. Remove the mass and repeat for any other desired load.

The entered grams are metadata. Firmware records only absolute raw count, the
run's raw reference and raw delta. It neither applies nor changes operational K.

Useful anchors are zero, an arrow-range mass near 20 g, approximately 1000 g,
and the 1806 g engineering reference when mechanically safe within the 2 kg
cell capacity. Further masses are optional and should answer a specific
question rather than lengthen the campaign by default.

## Cancel, recovery and finish

- `CANCEL` invalidates an active acquisition; creep cannot honestly be paused.
- Firmware retains the complete current run in RAM and replays it after a serial
  reconnect. The buffer does not survive loss of ESP32 power.
- Between acknowledged runs, waiting or restarting the PC logger is safe.
- `DONE` emits `SESSION_COMPLETE` and closes the current CSV cleanly.
- `Ctrl+C` manually stops the PC logger; acknowledged rows remain in the CSV.

Keep the PC awake, do not upload firmware during a run, and record any bump,
temperature change or shifted mass. Preserve original CSV evidence unsmoothed.

## Protocol v3 fields

    boot_id,run_id,sample_index,side,run_type,test_mass_g,
    elapsed_ms,raw_count,run_reference_raw,delta_count,host_timestamp_utc

See `docs/Creep_Data_Analysis.md` and `docs/Measurement_Model.md` for the
analysis and the operational design decision derived from it.
