# Creep and Jitter Evidence — 2026-08-07

This note records the first quantitative decision made from ArrowLab's retained
creep evidence. Raw CSV files remain unmodified and are the authoritative
records. Reproduce the summary with:

    python tools/analyze_creep.py <csv> [<csv> ...]

The script supports the legacy and protocol-v2 column layouts. It excludes the
t=0 transition sample from trend/noise calculations, uses a Theil-Sen robust
slope, and reports median first/last five-minute windows plus MAD-derived noise.

## Evidence used

- complete legacy Left/Right 0 g, 21 g and 999.8 g runs;
- later Left and Right zero runs, including the complete protocol-v2 Right run;
- complete protocol-v2 Right 1761 g run.

No equivalent protocol-v2 high-load Left run was supplied. High-load Left/Right
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

Operational firmware instead uses a 15-sample trimmed mean for display and
calibration sampling. With a full window the three highest and three lowest
samples are removed before averaging. This is deliberately small enough to
respond quickly while strongly rejecting an isolated HX711 spike.

The measurement policy is:

1. tare in the exact working configuration immediately before measurement;
2. wait for a stable short window;
3. capture/hold the result promptly rather than treating a 30-minute loaded
   value as equally valid;
4. retain diagnostic CSV values raw for future analysis;
5. do not auto-zero, because real low masses could be silently absorbed;
6. do not apply creep compensation without repeatable temperature-aware data.

For a spine force near the 880 g ATA load, a few tenths of a gram accumulated
over 30 minutes is negligible relative to the applied load and the actual
measurement will last seconds. For a roughly 20 g total-arrow mass, 0.1 g is
material, so immediate tare and a short stable capture are mandatory.

## Remaining validation

- perform the square-one normal Calibration workflow on both channels;
- verify CAL can deliberately be rerun after a fresh tare;
- verify the diagnostic workflow independently on Left and Right;
- compare a known small secondary mass after the operational filter;
- collect a high-load Left run only if later evidence shows that it would alter
  a design decision.
