# Creep Diagnostic - Operator Procedure

This is the short operator procedure for ArrowLab's hidden developer creep test.

For implementation detail and test rationale see `test/creep_diagnostic/README.md` and the Phase 2 design log.

## Purpose

Measure zero stability and time-under-load creep independently for Left and Right before any software compensation is designed.

A normal run lasts 30 minutes and records at t=0, t=10 seconds and every 30 seconds from t=30 seconds through t=30 minutes. The explicit 10-second point preserves the early reference reading used by established load-cell creep procedures while retaining the original 30-second campaign cadence.

## 1. Prepare firmware and PC capture

1. Pull the current ArrowLab firmware.
2. Clean, build and upload with PlatformIO.
3. Wait for upload to finish.
4. Stop PlatformIO Serial Monitor if it is open. Only one program may own the serial port.
5. Open a separate PowerShell/Windows Terminal.

### Python prerequisite

The PC capture tool requires Python 3 and the `pyserial` module.

For an ArrowLab development PC with PlatformIO installed, use PlatformIO's own Python environment. A second system-wide Python installation is not required.

Verify it first:

    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" --version
    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" -c "import serial; print(serial.__version__)"

If both commands succeed, run the capture tool from the ArrowLab repository root with:

    "%USERPROFILE%\.platformio\penv\Scripts\python.exe" tools\capture_creep.py COM3

Replace COM3 if ArrowLab uses another port.

If the PlatformIO Python path is missing or broken, repair/reinstall PlatformIO before firmware-development work. If this is a standalone diagnostic PC without PlatformIO, install current Python for Windows using the official Python Install Manager from https://www.python.org/downloads/ or the Microsoft Store. Then open a new terminal and verify/install the serial dependency:

    python --version
    python -m pip install pyserial
    python -c "import serial; print(serial.__version__)"

The standalone launch command is then:

    python tools\capture_creep.py COM3

Do not assume that the Windows `py` command exists: a rebuilt machine may have no global Python launcher even while PlatformIO's private Python works.

6. Leave the capture window running. It prints the exact CSV filename when it starts.
7. Prevent the PC from sleeping/hibernating during an active run. The monitor/display may turn off.

Default CSV location:

    calibration\diagnostics\creep_YYYYMMDD_HHMMSS.csv

Start PC capture before the zero baselines and leave it running through baseline, calibration and loaded runs.

## 2. Mandatory zero baselines

For the formal creep campaign, complete both zero baselines before calibration or any loaded run.

1. Return to Home and long-press the ArrowLab title to reveal Developer mode.
2. Open Settings -> Diagnostics.
3. Explicitly select LEFT. Diagnostics starts with no side selected.
4. Remove calibration platforms, weights and other added hardware. Leave only the normal fixed arrow-rest hardware.
5. Select ZERO BASE and confirm.
6. ArrowLab performs a fresh tare and automatically starts the 30-minute acquisition.
7. Do not touch or disturb the instrument during the active run.
8. Wait for RUN COMPLETE.
9. Select RIGHT and repeat the same bare-rest ZERO BASE procedure.
10. Wait for the Right RUN COMPLETE.

A cancelled or incomplete zero run does not count as a completed baseline. SET MASS and LOAD TEST remain disabled until both Left and Right baselines have completed.

## 3. Calibrate ArrowLab

With both zero baselines complete and PC capture still running:

1. Return to Settings -> Calibration.
2. Fit the empty calibration platform to Left.
3. Press Left TARE and confirm; a diagnostic tare does not satisfy this calibration-platform tare requirement.
4. Place the reference mass centrally, wait for the settling gate, then complete Left CAL.
5. Move the empty calibration platform to Right.
6. Press Right TARE and confirm.
7. Place the reference mass centrally, wait for the settling gate, then complete Right CAL.
8. Return to Settings -> Diagnostics.

The baseline completion state remains valid for the current powered diagnostic session. Diagnostic tares and calibration-platform tares are deliberately separate states. Calibration establishes the counts-per-gram factors used as supporting information during the loaded runs; raw and zeroed counts remain the primary creep evidence.

## 4. Loaded creep run

1. Select the required side.
2. Select SET MASS and enter the actual measured test mass in grams.
3. Fit the calibration platform to the selected side.
4. Keep the entered test mass OFF the platform.
5. Select LOAD TEST and confirm.
6. ArrowLab performs a fresh tare with the empty platform fitted.
7. When instructed, place the entered mass centrally on the platform.
8. Load detection automatically starts the 30-minute clock; operator stopwatch timing is not required.
9. Do not touch the jig during the active run.
10. Wait for RUN COMPLETE.
11. Remove the test mass.
12. Repeat for any further masses.

Current first-characterisation mass targets per channel:

- 0 g mandatory baseline
- approximately 20 g
- approximately 1000 g

The low and high anchor masses should be completed first. Inspect those results before deciding whether approximately 50 g, 100 g, 250 g and 500 g intermediate runs are needed. This avoids spending hours collecting redundant datasets while preserving the option to fill useful gaps.

Always enter the actual measured mass, not the nominal target.

## 5. Pause, cancel and resume

### Between completed runs

The session is naturally idle between runs. No test clock is running and no diagnostic data rows are generated.

The operator may take an indefinite break before starting the next dataset provided the instrument and capture PC remain powered and the PC does not sleep.

### Interrupted active run

Do not pause an active creep run. Creep continues physically while a load remains applied, so pausing data acquisition would create an invalid gap in the time history.

Use CANCEL, remove the load and restart that run from the beginning.

Every started run has a unique run ID so cancelled/repeated masses remain distinguishable in the CSV.

### Overnight shutdown or power cycle

Use FINISH and treat the next powered period as a new diagnostic session.

On the next session:

1. Start a new PC capture/CSV.
2. Record fresh Left and Right zero baselines.
3. Recalibrate both channels.
4. Continue with the remaining loaded datasets.

Separate sessions can be combined during PC analysis without pretending that a power/thermal cycle did not occur.

## 6. Finish the test session

After all desired Left and Right datasets are complete:

1. Select FINISH.
2. ArrowLab emits SESSION_COMPLETE.
3. The PC capture tool flushes and closes the CSV cleanly.
4. Preserve the original CSV unchanged for analysis.

Generated `creep_*.csv` files are ignored by Git by default and remain on the acquisition PC. A deliberately selected evidence dataset may still be added explicitly with `git add -f <filename>` after it has been reviewed and documented.

Ctrl+C in the capture terminal is the manual emergency stop if FINISH is unavailable.

## Important pitfalls

- Do not open PlatformIO Serial Monitor while the CSV capture tool owns the port.
- Do not upload firmware during an acquisition session.
- Do not allow the PC to sleep during an active run.
- Do not put the calibration/test mass on the platform before the fresh tare instruction is complete.
- Do not smooth, edit or discard raw readings before analysis.
- Record any bench bump, moved weight, power event or other unusual disturbance.
- More unique masses are not a substitute for repeatability. After the first mass sweep, repeated runs at selected masses may be more informative than adding many new mass values.

## Analysis

The ESP32 is responsible for disciplined data acquisition, not exploratory curve fitting.

PC analysis will compare zero drift, creep versus time and mass, Left/Right behaviour, repeatability, slopes/curve shape and candidate correction models. Excel may be used for inspection, but compensation decisions must be based on the preserved raw datasets and quantitative analysis.

## High-load reference run

After the exploratory low/high anchor runs, ArrowLab may perform one engineering reference run per channel using a procedure informed by OIML R 60 creep testing:

1. Use a total physical load of approximately 1,800 g on the selected 2 kg channel. The calibration platform and every other item supported by the active end contribute to this total. Choose the added test piece so the combined physical load is approximately 1,800 g, but enter only the separately measured added test-piece mass in SET MASS.
2. Keep the environment near 20 C and as stable as practical; record the approximate room temperature rather than claiming laboratory control.
3. Briefly apply and remove the reference load three times to preload the mechanical assembly. These are conditioning cycles, not three 30-minute acquisitions.
4. Return to the minimum-load condition and allow the assembly to rest for one hour.
5. Start one normal 30-minute loaded acquisition. The CSV includes t=0, the explicit 10-second reference point, 20 minutes and 30 minutes.
6. Repeat the procedure for the other channel because Left and Right are independent load-cell/HX711 systems.

This is an engineering comparison to an established load-cell procedure, not a substitute for accredited type evaluation. Raw and zeroed counts remain the primary evidence.
