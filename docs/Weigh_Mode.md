# Weigh Mode

## Purpose

Weigh is the ordinary mass-measurement screen. It reuses the WROOM-owned tare,
calibration factor and event-based held results; the VIEWE does not perform a
second measurement calculation.

## Before weighing

1. The intended cassette must be live and calibrated.
2. Remove the item being weighed.
3. Press `TARE L` or `TARE R` deliberately for every cassette to be used in
   the current powered session.
4. Confirm only when that cassette is empty and mechanically stable.

Calibration factor K survives ordinary power cycles. Tare does not, because it
belongs to the current physical cassette setup.

## Automatic source selection

The large card identifies the accepted load source:

- `LEFT CASSETTE` — only Left has an accepted non-zero load;
- `RIGHT CASSETTE` — only Right has an accepted non-zero load;
- `LEFT + RIGHT` — both have accepted loads; the displayed mass is their sum;
- `NO LOAD` — at least one cassette is ready but neither has an accepted load;
- `NOT READY` — neither cassette is both calibrated and deliberately tared.

The selection uses held measurement events rather than noisy live samples, so
there is no arbitrary gram threshold that would conceal a future lightweight
part. A cassette that is not ready is never silently included in the sum.

## Units

Tap the large reading on Weigh or Calibration to cycle the global current
session through grams, grains and avoirdupois ounces. The two non-primary units
remain visible underneath.

Power-up starts with `CONFIGURED_PRIMARY_MASS_UNIT` from `ArrowLab.conf`.
Presentation precision is configured independently with:

- `CONFIGURED_GRAMS_DECIMAL_PLACES`;
- `CONFIGURED_GRAINS_DECIMAL_PLACES`;
- `CONFIGURED_OUNCES_DECIMAL_PLACES`.

The current development defaults are two decimals for grams, no decimals for
grains and three decimals for ounces. These settings affect display only.

## Boundary with future features

Weigh provides the reusable accepted Left, Right and combined masses required
by later whole-arrow and Spine workflows. It does not yet calculate spine,
deflection, centre of gravity or FOC.
