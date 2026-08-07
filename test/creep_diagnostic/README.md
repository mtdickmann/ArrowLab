# Creep Diagnostic Design Notes

This permanent developer diagnostic characterises each load-cell/HX711 channel independently. It is intentionally separate from normal spine, mass and FOC workflows.

## State model

For each channel ArrowLab persists two independent facts:

- baseline evidence captured and acknowledged;
- valid counts-per-gram calibration for the current firmware version.

The operator chooses one channel before entering its workflow. A channel reset clears only that side. Calibration and baseline states are never inferred from the other side.

The acquisition engine moves through `WaitingForHost`, `Taring`, `AwaitingLoad` (loaded runs), `Running`, `AwaitingSave` and `Complete`. A load run is rejected unless the selected channel has both its baseline flag and calibration.

## Sampling

Every completed run contains 62 samples:

- t=0;
- t=10 s;
- t=30 s through t=1800 s in 30-second steps.

The early 10-second point supports comparison with established load-cell creep procedures. Firmware elapsed time is authoritative; host timestamps are supporting metadata.

Recorded fields are boot ID, run ID, sample index, side, run type, entered mass, elapsed milliseconds, raw count, zeroed count, calculated grams, calibration factor and host UTC timestamp.

Raw and zeroed counts are the primary evidence. Gram conversion is supporting information. No run recalibrates itself and no creep correction is currently applied.

## Reliable serial protocol (v2)

The PC sends `HELLO` and one-second heartbeats. Firmware refuses to start acquisition until a current handshake exists.

All 62 samples are retained in ESP32 RAM. Live serial output is convenient but not trusted as the sole record. At completion firmware replays the entire buffered run. The PC deduplicates `(boot_id, run_id, sample_index)`, flushes the CSV and sends an ACK containing the expected sample count. Only that ACK changes a zero run to `BASE OK`.

If the serial link fails, acquisition continues in RAM and the logger reconnects/replays. The buffer does not survive an ArrowLab power loss. Starting another run is blocked until the current buffer is acknowledged or deliberately cancelled.

Routine high-rate HX711 debug output is suppressed so it cannot starve protocol traffic.

## Persistence and firmware changes

ESP32 Preferences stores calibration factor/reference/version and the baseline-complete flag under separate Left/Right keys.

Calibration loads only when the stored firmware version matches `Version.h`; a firmware revision therefore requires recalibration. Baseline evidence may remain because it is a historical diagnostic record rather than a measurement conversion factor.

Legacy CSV files remain external evidence only. Firmware does not claim to read
or import a PC file, so there is no baseline-import bypass in the UI. A new or
reset channel earns `BASE OK` only through a complete acknowledged run.

## Why active runs cannot pause

Creep continues while load remains applied. Pausing acquisition would create an unmeasured interval and a misleading time axis. `CANCEL` invalidates the active run; recovery is a full restart. Waiting between complete runs is unrestricted.

## First-characterisation campaign

For each side:

1. mandatory bare-rest zero baseline;
2. deliberate platform tare and calibration;
3. approximately 20 g load;
4. approximately 1000 g load;
5. optional approximately 1800 g engineering reference if mechanically stable and within capacity.

Intermediate masses are added only if the anchors show meaningful mass dependence. This is an engineering investigation, not accredited OIML conformity testing.
