# ArrowLab Creep Diagnostic

This developer test characterises time-under-load creep independently for the Left and Right load-cell/HX711 channels.

It is intentionally retained as a permanent service/development test.

## What is recorded

Firmware emits one data row at t=0 and then every 30 seconds for 30 minutes.

Each normal completed run therefore contains 61 data rows.

Recorded fields:

- diagnostic run ID
- side
- operator-entered test mass in grams
- firmware elapsed time in milliseconds
- HX711 raw count
- tare-adjusted/zeroed count
- calculated grams when a calibration factor exists
- active calibration factor
- host UTC timestamp added by the capture tool

Run ID increments for every started run, including cancelled/retried runs, so repeated masses remain unambiguous in later analysis.

Raw and zeroed counts are the primary diagnostic evidence. Converted grams are supporting information.

The diagnostic does not recalibrate itself for each test mass.

## Recommended campaign

Run the following independently on Left and Right:

1. Zero baseline: normal fixed arrow-rest hardware only; no calibration platform and no added mass.
2. Approximately 20 g.
3. Approximately 50 g.
4. Approximately 100 g.
5. Approximately 250 g.
6. Approximately 500 g.
7. Approximately 1000 g.

For every loaded run, enter the actual measured mass rather than the nominal target above.

Complete the zero baseline on both Left and Right first. The firmware keeps SET MASS and LOAD TEST disabled until both baselines have completed; the diagnostic engine independently rejects a loaded run if either baseline is missing. A cancelled or incomplete zero run does not satisfy this gate.

For the first characterisation campaign, perform calibration after both raw zero baselines and before the first loaded run. Each side must be deliberately tared again with the empty calibration platform fitted. The automatic diagnostic tare is isolated from calibration state and cannot satisfy this requirement. Calibration provides supporting gram conversion while raw and zeroed counts remain the primary creep evidence.

With this current seven-run-per-channel campaign, acquisition time is approximately seven hours plus handling time. The permanent diagnostic does not require this exact number of loaded masses: after the mandatory zero baseline for each tested channel, the operator may run as many or as few loaded datasets as the investigation requires.

## PC capture

The firmware streams diagnostic records over the normal 115200-baud USB serial connection.

The capture script requires Python 3 plus `pyserial`.

### Preferred development-PC interpreter

PlatformIO installs and manages its own Python environment. On the normal Windows ArrowLab development PC, use that interpreter instead of requiring a second global Python installation.

Verify Python and `pyserial`:

    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" --version
    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" -c "import serial; print(serial.__version__)"

From the ArrowLab repository root:

    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" tools\capture_creep.py COM7

Replace COM7 with the display's actual serial port.

### Standalone PC / missing interpreter

If the PlatformIO environment is expected but `penv\Scripts\python.exe` is missing, repair/reinstall PlatformIO.

If the diagnostic capture PC intentionally does not have PlatformIO, install current Python for Windows with the official Python Install Manager from https://www.python.org/downloads/ or the Microsoft Store. Open a new terminal and run:

    python --version
    python -m pip install pyserial
    python -c "import serial; print(serial.__version__)"
    python tools\capture_creep.py COM7

The Windows `py` launcher is not assumed by ArrowLab documentation; on a rebuilt PC it may be absent even when PlatformIO's private Python is present.

The capture tool ignores ordinary firmware debug messages and writes only AL_DIAG data rows.

By default CSV files are written to:

    calibration/diagnostics/

Start the PC capture before starting the first run. It may remain open for the whole campaign and append all Left/Right runs into one timestamped CSV. Use FINISH on ArrowLab after the desired datasets are complete; the SESSION_COMPLETE event closes the host capture cleanly. Ctrl+C remains available as a manual stop.

Do not have PlatformIO Serial Monitor open on the same COM port while the capture tool is running. Only one process should own the port.

## Zero baseline UI sequence

1. Reveal Developer mode.
2. Settings -> Diagnostics.
3. Explicitly select Left or Right. No diagnostic run may begin from an implicit/default side.
4. Remove calibration platform and all added weight so only the normal fixed arrow-rest hardware remains.
5. Press START ZERO.
6. Confirm START.
7. ArrowLab performs a fresh tare.
8. Logging begins automatically after tare completes.
9. Leave the instrument untouched for 30 minutes.
10. Repeat the zero baseline for the other side.
11. Do not proceed to mass entry or loaded testing until both sides report their zero baseline complete.

## Loaded UI sequence

1. Select Left or Right.
2. Press SET MASS and enter the actual mass in grams.
3. Fit the calibration platform to the selected side.
4. Keep the test weight OFF the platform.
5. Press START LOAD and confirm START.
6. ArrowLab performs a fresh tare with the empty platform fitted.
7. When the screen asks for the weight, place the entered mass centrally on the platform.
8. ArrowLab requires five consecutive fresh samples above the diagnostic load threshold.
9. The 30-minute clock and logging start automatically when the load is confirmed.
10. Leave the jig untouched until RUN COMPLETE.
11. Remove the weight and prepare the next run.

## Diagnostic load trigger

Loaded diagnostic runs currently use a 2,000-count zero-adjusted trigger and require five consecutive fresh samples.

This threshold is separate from the much higher calibration-reference trigger. It exists so the approximately 20 g diagnostic mass can start a run while remaining comfortably above the observed approximately +/-200-count zero noise.

## Test discipline

- Do not touch the jig during an active run.
- Keep environmental conditions as consistent as practical.
- Record unusual events separately if the bench is bumped, power changes, or a weight visibly moves.
- Do not intentionally compensate, smooth or edit the raw dataset before analysis.
- Preserve incomplete/failed runs when useful; they may still reveal recovery or handling behaviour.
