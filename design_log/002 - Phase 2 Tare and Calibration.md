# Phase 2 - Tare and Calibration

## Date
2026-08-06

## Status
Calibration is implemented and under hardware validation. Both HX711/load-cell channels have repeatedly calibrated independently and produced consistent converted gram readings.

Spine calculations must not be added until calibration, repeatability, drift and measurement timing are considered reliable.

## Measurement architecture

- `main.cpp` is the top-level coordinator. It does not own HX711 acquisition, tare maths or calibration maths.
- `src/measurement/LoadCellChannel.*` owns the state and behaviour of one HX711/load-cell channel.
- Left and Right are independent channels with independent tare state and calibration factors.
- The UI presents state and raises user actions; measurement maths does not belong in the UI.
- Future measurement modes such as spine, whole-arrow mass and FOC will consume calibrated Left/Right loads rather than accessing HX711 hardware directly.

## Tare conventions

Two different zero concepts are intentionally distinguished:

1. Startup automatic zero
   - Used internally to establish usable zero-adjusted raw readings.
   - Does not mean the user has established the correct physical measurement setup.
   - Must not produce `TARE OK`.

2. Deliberate user tare
   - Requires an explicit TARE action and confirmation for the selected side.
   - The calibration platform must be fitted to the selected side when preparing a calibration tare.
   - Only successful completion of this deliberate tare produces `TARE OK`.

Left and Right may be tared in either order.

A new tare changes the temporary zero offset but does not erase a valid calibration factor. This is required because later measurement modes will have different physical configurations. For example, whole-arrow mass/FOC operation will tare the normal jig without calibration platforms or calibration weights while continuing to use the established counts-to-grams factors.

## Calibration conventions

- Calibration is independent for Left and Right.
- The current development reference mass is 999.8 g.
- 999.8 g is not a firmware-defined universal calibration mass. It is the current user's measured reference.
- The measurement API already accepts a supplied reference mass.
- A future calibration screen will provide numeric keypad entry for the user's actual reference mass.
- A practical minimum reference mass of approximately 500 g is currently planned, subject to further validation.
- Calibration uses 20 fresh samples to determine average zero-adjusted counts for the supplied reference mass.
- Calibration factor is stored as signed counts per gram so either load-cell signal direction is supported.
- After successful calibration the channel displays grams.
- Re-taring does not invalidate the calibration factor.
- Calibration factors are currently RAM-only and therefore lost on power cycle.

### Planned persistence rule

When persistent storage is implemented:

- Left and Right calibration factors are stored independently.
- Calibration survives normal power off/on.
- Normal tare operations do not invalidate calibration.
- Calibration remains valid until the user deliberately recalibrates or the firmware is updated.
- Stored calibration must be associated with the firmware version from `Version.h`.
- A firmware-version mismatch invalidates the stored calibration and requires recalibration.
- Home and the Calibration screen must visibly show whether calibration is valid or required.
- ArrowLab must not assume a factory calibration.

## Calibration load detection and settling

Calibration must not become available merely because an arbitrary amount of time has passed since tare. The reference mass must actually be present.

Current calibration gating:

1. The selected side must have a deliberate `TARE OK`.
2. A significant zero-adjusted load of at least 250,000 raw counts must be detected.
3. The threshold must be exceeded for 5 consecutive fresh HX711 samples.
4. Only then does the 30-second calibration settling timer start.
5. If the load drops below the detection threshold, the timer is reset.
6. CAL becomes available only after the full settling interval.
7. The measurement layer enforces the rule independently of the disabled/enabled state of the UI button.

### Why 250,000 counts?

Initial hardware observation is approximately one million counts for a mass near 1 kg. A 500,000-count trigger would therefore be too close to the expected signal from a roughly 500 g minimum calibration mass and could fail on a less-sensitive channel.

250,000 counts is still orders of magnitude above the observed near-zero raw noise while leaving useful margin for channel-to-channel sensitivity differences.

This raw threshold is necessary before first calibration because a trustworthy counts-to-grams factor does not yet exist.

## Status and colour conventions

Status uses text and colour together. Colour is a quick visual cue, not the only source of state information.

- Orange: action/attention required or operation not yet valid.
- Green: operation/state currently valid or successfully completed.
- `TARE OK` / `TARE REQ` remains visible in text.
- `CAL OK` / calibration-required state remains visible in text.
- TARE and CAL buttons adopt the same orange/green status convention.

At the current development stage, green CAL means a valid calibration in the current powered session. After persistent storage is implemented, green calibration status will mean the stored calibration is valid for the running firmware.

## Units

- Grams are the canonical measurement unit.
- Planned quick display choices are `g`, `gr` (grain) and `oz`.
- Unit conversion is display-only; calibration and internal measurement remain based on grams.
- Kilograms are not currently planned as a display unit because ArrowLab's useful measurement range does not justify them.

## Current hardware observations

The following are engineering observations, not yet final specifications:

- Both channels repeatedly calibrate consistently.
- The Right channel currently appears somewhat more stable than the Left channel.
- Fast displayed jitter is approximately 0.2 g peak-to-peak in typical testing.
- A time-under-load drift/creep effect has been observed.
- Under prolonged load the indicated mass has moved by roughly +0.4 to +0.5 g after many minutes in some tests.
- A tared channel carrying only its calibration platform remained much closer to zero while the loaded channel continued to drift.
- A temporary approximately 132 g household reference produced readings close to its expected value on both channels, but it is not a calibrated reference standard.
- The calibrated 113.8 g secondary verification weight remains the intended independent verification reference.

## Drift and filtering policy

Jitter and drift must not be treated as the same problem.

- Short-term noise/jitter may later be reduced by statistically defensible filtering or averaging.
- Slow under-load drift/creep must not be hidden by display smoothing.
- No mathematical creep compensation will be added until the behaviour has been measured repeatedly at multiple known loads and over controlled time intervals.
- A fixed measurement timing protocol is preferred before adding model-based compensation.
- Future testing should record readings at controlled intervals such as 0 s, 10 s, 30 s, 60 s, 2 min, 5 min and 10 min.
- If creep proves repeatable per channel, a bounded per-channel correction may be considered.
- Calibration should eventually have a limited valid action window after settling so a reference mass cannot remain loaded indefinitely before calibration is accepted.
- Ordinary arrow mass/FOC measurements should use a controlled settle/sample/freeze window rather than indefinite live readings when a final measurement is required.

## Next validation steps

1. Verify the load-triggered 30-second CAL lockout on hardware.
2. Verify the timer resets if the reference load is removed.
3. Characterise zero stability and under-load creep independently for Left and Right.
4. Verify both calibrated channels using the 113.8 g secondary reference.
5. Decide the measurement averaging/stability method from observed data.
6. Implement persistent calibration storage and firmware-version invalidation.
7. Only after scale behaviour is reliable, proceed toward combined load and later spine calculations.
