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
2. Five consecutive filtered zero-referenced readings above 250,000 counts
   confirm that a substantial load is present.
3. The second CAL action starts the fixed 30-second settling interval.
4. After settling, 20 fresh events are accumulated. Each event contributes the
   current 15-sample robust filtered, zero-referenced result.
5. Their arithmetic mean is divided by the entered reference mass:

       K = average_filtered_zeroed_counts / reference_mass_g

6. K, reference mass and firmware compatibility version are saved independently
   for Left and Right in WROOM NVS.
7. At that instant the held display is anchored to the entered reference mass
   and the private tracker is anchored to the same current filtered raw state.

K is signed; either load-cell signal polarity is valid.

## Ordinary weighing and held result

While the physical state is unchanged, a slow private raw tracker follows
drift and creep without altering the displayed mass. Four consecutive fresh
conversions at least 300 counts from that tracker begin a change acquisition.

The new state is accepted at a fixed ten-second endpoint. A robust average of
the final 20 raw samples is compared with the frozen pre-change tracker. Only
this raw difference is converted through K and added to the held result. Using
one fixed endpoint prevents otherwise identical placements from being captured
at arbitrary times between two and ten seconds as the load cell settles.

## Audit finding: capture definitions differ

The current implementation intentionally has different timing purposes, but
it also uses three different capture definitions:

- TARE: mean of 20 direct raw conversions;
- CAL: mean of 20 rolling 15-sample robust-filter outputs after 30 seconds;
- ordinary load event: robust average of the final raw acquisition window at a
  fixed 10-second endpoint.

The former 2–10 second early-exit window was a credible contributor to the
observed behaviour in which the calibration weight was exactly right at
calibration completion but differed after removal/replacement. Ordinary load
events now use a fixed 10-second endpoint so repeatability can be assessed
without that variable capture time. The calibration display is still
explicitly anchored to the entered mass, whereas a replacement result is an
independent measurement through K.

That explanation is not yet proof of the complete error source. Load-cell
mechanics, recovery, creep and sample-rate differences can contribute. The
fixed ten-second operational endpoint is therefore a controlled one-variable
change; calibration capture, K, tare, persistence and drift tracking are
unchanged. Repeated removal/replacement tests on the same channel must verify
whether it improves repeatability before any further metrology change.

## Revision compatibility

v0.2.1 changes the firmware compatibility version. Existing v0.2.0 calibration
records are therefore rejected by design. Upload both processors, then perform
one fresh Left and Right calibration. Future documentation-only releases should
not bump `Version.h` unless deliberate K invalidation is intended.
