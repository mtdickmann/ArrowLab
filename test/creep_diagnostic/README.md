# Creep Diagnostic Design Notes

This maintained developer diagnostic characterises each raw load-cell/HX711
channel independently. It cannot alter operational tare or calibration.

## State model

The acquisition engine moves through `WaitingForHost`, `CapturingReference`,
`AwaitingLoad` for loaded runs, `Running`, `AwaitingSave` and `Complete`.

Every run owns a private 20-sample raw reference. Zero runs begin immediately
after that capture. Loaded runs then wait for five consecutive raw changes above
2,000 counts. No baseline flag is persisted; the acknowledged CSV is the
evidence record. Operational calibration never gates a raw run.

## Sampling and protocol v3

Every complete run contains 62 samples: t=0, t=10 seconds, and t=30 through
t=1800 seconds in 30-second steps. Records contain absolute raw count, run raw
reference and raw delta. Entered mass is metadata; calculated grams and
operational K are deliberately absent.

The PC sends `HELLO` and one-second heartbeats. Firmware buffers all samples in
RAM, replays them at completion, and waits for an ACK with the expected count.
The host deduplicates `(boot_id, run_id, sample_index)` before saving. Serial
loss is recoverable while the ESP32 remains powered.

The deterministic host test proves that a loaded run can enter acquisition
without operational tare, calibration or prior baseline evidence.

## Why runs cannot pause

Creep continues while load remains applied. Pausing would create an unmeasured
interval and false time axis. `CANCEL` invalidates the run; waiting between
complete runs is unrestricted.
