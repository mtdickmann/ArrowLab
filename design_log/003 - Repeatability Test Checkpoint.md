# ArrowLab Repeatability Test Checkpoint

## Date and stop point

2026-08-14, approximately 02:00 Europe/Berlin.

Work is paused for hardware testing. Do not change the measurement algorithm,
calibration workflow, processor link or pin assignments until the fixed
ten-second experiment below has been tested and reported.

## Proven hardware architecture

The dual-processor architecture is established and working:

- VIEWE ESP32-S3: display, touch, navigation and user commands only.
- ESP32-S3-WROOM-1 N16R8: both HX711 channels, tare, calibration, K,
  event-based measurement and calibration persistence.
- Left HX711 on WROOM: DT GPIO4, SCK GPIO5.
- Right HX711 on WROOM: DT GPIO6, SCK GPIO7.
- Production inter-processor link: one-wire half-duplex UART at 115200 baud.
- VIEWE GPIO17 (J7) connects to WROOM GPIO8, plus common GND.
- A 4.7 kOhm pull-up connects the shared signal node to WROOM 3.3 V.
- The signal jumper must be removed while uploading either processor and
  reconnected afterward.

GPIO11/GPIO12 on the VIEWE are only the proven two-wire fallback and conflict
with onboard SD signals. They are not the production connection and must not be
proposed again without new evidence invalidating the working GPIO17/GPIO8
one-wire link.

## Calibration state

The known-good calibration workflow has been restored and works independently
for Left and Right. A recent apparent failure in which the reference weight did
not release the CAL button was not a firmware defect: the load-cell connection
had been disturbed and was not properly seated in the breadboard.

Current rules remain:

- TARE is deliberate, per side and temporary for the current powered session.
- K and the entered reference mass are stored per side in WROOM NVS.
- K survives normal power cycles and a new tare.
- Calibration uses a fixed 30-second loaded stabilization followed by its
  existing filtered sample capture.
- Calibration completion anchors the displayed value to the entered reference
  mass; later removal/replacement is an independent measurement through K.

Do not reopen the recent calibration-load-gate investigation unless the fault
can be reproduced with both load-cell connections physically verified.

## Actual unresolved problem

After a successful calibration with the 999.8 g reference, displayed zero can
remain stable, but removing and replacing the same weight sometimes returns a
slightly different value, typically about 0.1 to 0.2 g low or high. The value
may later recover. The purpose of the current work is repeatable, defensible
weighing without a nervous wandering display or dishonest precision.

Recorded creep evidence does not support one universal time correction:

- an older approximately 999.8 g run moved downward by about 0.20 g after its
  early settled region;
- an approximately 1761 g run moved upward by about 0.28 g over 30 minutes;
- in the 1761 g run the recorded 10-second and 30-second values differed by
  only about 0.002 g.

Therefore K timing, tare, creep tracking and ordinary acquisition must not all
be changed simultaneously.

## Current controlled experiment

Branch: `agent/event-based-measurement`

Published GitHub commit: `5e15bb6 Use fixed ten-second measurement capture`

The equivalent local development commit was `139e2da`; the SHA differs because
the connected GitHub publisher recreated the same tree after local HTTPS push
credentials were unavailable.

Only ordinary event acquisition changed:

- previously, an apparently stable load could lock at any point from 2 to 10
  seconds;
- experimentally, every detected load change now uses one fixed 10-second
  endpoint and a robust average of its final 20 raw samples;
- tare, 30-second calibration, K calculation, persistence, UI, one-wire link
  and background drift tracking are unchanged;
- no version bump was made and stored K was not invalidated.

Ten seconds is explicitly not accepted as the final user experience. It is a
diagnostic experiment to determine whether variable capture timing caused the
observed reweighing spread. The intended production target remains a stable,
repeatable result in approximately two seconds, comparable to a competent
counting scale.

## Morning test procedure

Only the WROOM firmware changed.

1. Pull `agent/event-based-measurement`.
2. Remove the VIEWE-WROOM signal jumper.
3. Clean, build and upload `ARROWLAB_MEASUREMENT_S3` to the WROOM.
4. Reconnect the signal jumper after upload.
5. Power normally and confirm both channels are online.
6. Deliberately tare and freshly calibrate the test side with 999.8 g.
7. Remove and replace the same reference weight about ten times without
   retaring or recalibrating between repetitions.
8. Allow every removal and placement to complete its ten-second acquisition.
9. Record each locked result and whether zero returns correctly.

Decision after the test:

- If repeatability is not materially better, revert the fixed ten-second
  experiment and instrument the exact raw endpoints used for K and each event.
- If repeatability improves, use that evidence to design a proper fixed or
  adaptive approximately two-second acquisition. Do not retain ten seconds as
  production behaviour.

No spine calculations or further feature work should be added until this
repeatability decision is made.
