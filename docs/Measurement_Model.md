# ArrowLab Measurement Model

## Purpose

ArrowLab uses event-based measurement rather than displaying an indefinitely
wandering live conversion. The model separates four facts that must not be
conflated:

- **raw conversion**: the latest signed HX711 count;
- **zero reference**: the raw condition deliberately declared to be zero by
  TARE for the current physical setup;
- **calibration factor K**: signed counts per gram, stored independently for
  Left and Right;
- **held result**: the last accepted physical load state shown to the user.

Tare is temporary and must be established after every power-up before weighing.
K is persistent across ordinary power cycles and changes only after deliberate
calibration, reset, or firmware-version invalidation.

## Equations

Calibration uses the known reference mass after the fixed 30-second settling
period:

    K = (loaded_raw - zero_reference_raw) / reference_mass_g

K may be positive or negative because either electrical signal direction is
valid.

For an accepted load change:

    delta_g = (new_state_raw - pre_change_tracked_raw) / K
    held_g  = previous_held_g + delta_g

The raw reference is tracked only while no genuine load change is occurring.
The displayed result is not repeatedly recalculated from a drifting absolute
zero.

## Operational state sequence

1. **Needs Tare** — no weighing result is valid.
2. **Taring** — average 20 fresh raw conversions and declare that condition
   zero. Existing K is retained.
3. **Tracking** — hold the accepted display while a slow private raw tracker
   follows creep and drift.
4. **Acquiring Change** — after four consecutive raw samples exceed the
   provisional 300-count change threshold, freeze the pre-change tracker and
   acquire the new state.
5. **Accept** — use a robust average of the final samples after at least two
   stable seconds, or at the ten-second maximum. Apply only the before/after
   difference to the held result, then resume tracking.

Removal is an ordinary negative load event. A small residual while moving back
toward zero is clamped to exactly zero within the provisional larger of 0.5 g
or 0.2% of the previous held load. This rule prevents known unloading recovery
from presenting a removed object as a remaining mass; it does not auto-zero
while an unchanging object is present.

## Why this is not cosmetic smoothing

The display is a sample-and-hold representation of accepted physical states.
Background tracking changes the raw reference used for the *next* detected
step; it never edits an already accepted displayed mass. A genuine addition or
removal freezes tracking before its difference is measured, so a small static
mass is not gradually erased.

The 30-second calibration interval and maximum 10-second operational interval
have different jobs. Thirty seconds produces a repeatable K from the known
reference. Ten seconds limits how long the operator waits for an ordinary
weighing result.

## Diagnostics boundary

Creep diagnostics intentionally do not use operational tare, K, tracking,
filtering, held grams, or zero-baseline flags. Every run captures its own short
raw reference and records:

- absolute raw count;
- that run's raw reference;
- raw delta from that reference;
- entered mass as metadata.

This keeps the evidence suitable for analysing the measurement method itself.
Running a diagnostic must not modify the operational zero or stored K.

## Validation status

The state engine has deterministic host tests for tare, calibration anchoring,
slow drift, unloading, 20 g and 50 g changes, re-tare, persistent-K restore and
both sensor polarities. Detection, stability and zero-return thresholds remain
provisional until they pass the real dual-HX711 hardware trial.
