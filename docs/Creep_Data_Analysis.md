# Creep and Jitter Evidence — 2026-08-07

This note records the first quantitative decision made from ArrowLab's retained
creep evidence. Raw CSV files remain unmodified and are the authoritative
records. Reproduce the summary with:

    python tools/analyze_creep.py <csv> [<csv> ...]

The script supports the raw-only protocol-v3 format and the retained legacy
formats. It excludes the
t=0 transition sample from trend/noise calculations, uses a Theil-Sen robust
slope, and reports median first/last five-minute windows plus MAD-derived noise.

## Evidence used

- complete legacy Left/Right 0 g, 21 g and 999.8 g runs;
- later Left and Right zero runs, including the complete legacy protocol-v2
  Right run;
- complete legacy protocol-v2 Right 1761 g run.

No equivalent high-load Left run was supplied. High-load Left/Right
symmetry is therefore not claimed.

## Material results

| Channel/run | 30-minute movement | Robust residual noise | Observation |
|---|---:|---:|---|
| Left 21 g | -0.020 g | 0.039 g | Small movement at arrow-range load |
| Right 21 g | -0.157 g | 0.046 g | Direction differs from later Right runs |
| Left 999.8 g | +0.189 g | 0.045 g | One gross multi-million-count transient |
| Right 999.8 g | -0.178 g | 0.033 g | Negative drift in this session |
| Right 1761 g | +0.285 g | 0.048 g | Positive drift in a later session |

Zero runs also changed sign and rate between sessions. The Right 1761 g t=0
sample was still far from the settled load; by t=10 seconds it was close to the
entered mass. This validates both preserving early evidence and refusing to use
the transition sample as a normal measurement.

## Decision

A universal time correction is rejected. Drift is not repeatable enough in
sign or rate, so subtracting a fixed curve would sometimes increase error.

Operational firmware therefore uses event-based sample-and-hold measurement,
not a universal time correction and not an indefinitely wandering live grams
conversion. A private slow raw tracker follows drift only while no load change
is present. A confirmed step freezes the pre-change tracker, acquires a robust
new raw state for no more than ten seconds, and applies only that difference to
the held result.

The measurement policy is:

1. deliberately tare the exact working configuration after each power-up;
2. retain persistent per-channel K across ordinary power cycles;
3. track only slow background raw movement while the accepted display is held;
4. freeze tracking before a confirmed addition or removal is measured;
5. acquire the new state for at least two stable seconds and at most ten
   seconds;
6. retain diagnostics as unprocessed absolute raw/reference/delta evidence;
7. do not apply a fixed elapsed-time creep curve.

This tracker is not unrestricted auto-zero. It is suspended as soon as a
candidate step is detected, so an unchanging small real mass is held rather
than gradually erased. See `docs/Measurement_Model.md` for the equations,
provisional thresholds and state ownership.

For a spine force near the 880 g ATA load, a few tenths of a gram accumulated
over 30 minutes is negligible relative to the applied load and the actual
measurement will last seconds. For a roughly 20 g total-arrow mass, 0.1 g is
material, so immediate tare and a short stable capture are mandatory.

## Remaining validation

- build and upload the feature branch with PlatformIO;
- recalibrate both channels because v0.2.0 intentionally invalidates older K;
- verify held 999.8 g does not wander during several minutes of static load;
- verify removal returns to zero and 999.8 g can be reapplied repeatedly;
- compare the 113.8 g secondary mass and an arrow-range mass;
- tune the provisional change/stability thresholds only from observed hardware
  transitions.
