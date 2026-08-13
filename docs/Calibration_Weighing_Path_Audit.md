# Calibration and Weighing Path Audit — v0.2.1

## Scope and freeze point

This audit records the implemented metrology path without changing its maths.
The dual-processor ownership boundary and event-based weighing model are frozen
at v0.2.1 while repeatability is characterised on hardware.

The WROOM is the only measurement authority. The VIEWE issues deliberate TARE
and CAL commands and displays returned state; it does not calculate a second
zero, K or mass value.

## Raw acquisition path

1. Each HX711 conversion is read unscaled as a signed `long` by
   `LoadCellChannel`.
2. The fresh raw conversion is passed once to the corresponding
   `MeasurementChannel`.
3. The same fresh-reading event advances the corresponding independent
   `CalibrationController` workflow.
4. Left and Right execute the same classes and constants. There is no separate
   Left formula or Right formula.

## Tare capture

TARE averages 20 consecutive, unfiltered HX711 conversions. That arithmetic
mean becomes `tareReference`. Tare clears the held result and initializes the
private drift tracker at the new zero. A deliberate re-tare retains an existing
K.

Tare is session state and is not stored across power cycles.

## Calibration capture and K

1. The user supplies the reference mass and fits the loaded calibration
   platform after a deliberate tare.
2. Five consecutive fresh HX711 conversions at least 2,000 counts from the
   completed tare confirm that a deliberate load is present. The physical
   arming gate deliberately uses the direct raw difference, rather than the
   event/display filter, so that filtering cannot conceal a placed reference
   weight. The threshold remains well above the observed few-hundred-count
   unloaded noise without assuming every valid load-cell/HX711 pair has the
   original prototype's sensitivity.
3. The second CAL action starts the fixed 30-second settling interval.
4. After settling, 20 fresh events are accumulated. Each event contributes the
   current 15-sample robust filtered, zero-referenced result.
5. Their arithmetic mean is divided by the entered reference mass:

       K = average_filtered_zeroed_counts / reference_mass_g

6. K, reference mass and firmware compatibility version are saved independently
   for Left and Right in WROOM NVS.
7. At that instant the held display is anchored to the entered reference mass
   and the private tracker is anchored to the same current filtered raw state.

While waiting for the reference load, CAL remains disabled. It is re-enabled
only after the raw load gate has been satisfied. TARE deliberately abandons any
pending calibration setup, establishes a new physical zero and retains the last
valid K. Only K and its reference mass persist in WROOM NVS; `AwaitingLoad` and
all other workflow stages are transient.

After load detection, the final confirmation offers `EDIT MASS` as well as
`CALIBRATE`, so the reference remains correctable until the 30-second operation
actually begins.

K is signed; either load-cell signal polarity is valid.

## Ordinary weighing and held result

While the physical state is unchanged, a slow private raw tracker follows
drift and creep without altering the displayed mass. Four consecutive fresh
conversions at least 300 counts from that tracker begin a change acquisition.

The new state is accepted after at least two seconds when its final 20 raw
samples span no more than 300 counts, or at the ten-second maximum. A robust
average of that final window is compared with the frozen pre-change tracker.
Only this raw difference is converted through K and added to the held result.

## Audit finding: capture definitions differ

The current implementation intentionally has different timing purposes, but
it also uses three different capture definitions:

- TARE: mean of 20 direct raw conversions;
- CAL: mean of 20 rolling 15-sample robust-filter outputs after 30 seconds;
- ordinary load event: robust average of the final raw acquisition window after
  2–10 seconds.

This is a credible explanation for the observed behaviour in which the
calibration weight is exactly right at calibration completion, differs after
removal/replacement, and may later return toward the calibrated value. The
calibration display is explicitly anchored to the entered mass, whereas the
replacement result is independently measured through K using the shorter event
window.

That explanation is not yet proof of the complete error source. Load-cell
mechanics, recovery, creep and sample-rate differences can contribute. No
calibration or weighing algorithm is changed by this audit. The next controlled
test should log timestamps and the exact raw values used for tare mean,
calibration span/K and the subsequent removal/replacement acquisitions on the
same channel.

## Revision compatibility

v0.2.1 changes the firmware compatibility version. Existing v0.2.0 calibration
records are therefore rejected by design. Upload both processors, then perform
one fresh Left and Right calibration. Future documentation-only releases should
not bump `Version.h` unless deliberate K invalidation is intended.
