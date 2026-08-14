# Spine and SAS Test

## What ArrowLab measures

ArrowLab provides two deliberately separate tests:

- **Spine Test** captures one orientation and reports one equivalent spine
  value.
- **SAS Test** captures four orientations spaced 90 degrees apart and reports
  every result, the stiffest result, the weakest result, the range and the
  spine-around-shaft difference.

Lower numerical spine values are stiffer. Higher values are weaker. One spine
point is 0.001 inch of equivalent deflection, so a result range from 341 to 350
is a nine-point range and an SAS difference of 0.009 inch.

The optional marked-spine comparison is informational. The operator may enter
the value printed on the shaft. ArrowLab then shows the numerical and percentage
difference; it does not declare PASS or FAIL because no acceptance tolerance has
yet been adopted.

## Method boundary

The familiar static method applies an 880 g centre load and measures
deflection over a fixed span. ArrowLab's mechanics do the inverse: the plunger
stops at a fixed 12.7 mm (0.500 inch) deflection and WROOM measures the additional
force required to reach that point. The displayed result is therefore an
**ATA-derived equivalent spine**, calculated under the assumed linear elastic
relationship:

    equivalent_spine = 500 * 880 / applied_force_g

An applied force of 880 g at the 12.7 mm stop therefore reports 500 spine.
This wording is intentional: the instrument does not claim that its inverse
force method is a literal execution or certification of the published method.

The public reference history is:

- ATA Technical Guidelines, with the current 2025 revision available to ATA
  members from <https://archerytrade.org/technical-guidelines/>;
- ASTM F2031-05(2014), *Measurement of Arrow Shaft Static Spine*, which defined
  spine-around-shaft variation from four readings at 90 degree spacing but was
  withdrawn in 2023 without replacement:
  <https://store.astm.org/f2031-05r14.html>.

The exact ArrowLab implementation and firmware version must accompany exported
results; a standards name alone is not a substitute for that method statement.

## Automatic operator sequence

WROOM owns capture timing. VIEWE only presents its state.

1. The operator opens Spine Test or SAS Test with both channels live and
   calibrated.
2. Both supports must be empty. Press **START TEST**.
3. WROOM automatically tares Left and then Right. This is deliberately started
   by the operator so an arrow left on the supports cannot be silently zeroed.
4. At **READ**, place the arrow on both supports and keep it still. ArrowLab
   captures and displays its combined resting mass.
5. At **PUSH**, press the plunger firmly to the physical 12.7 mm stop.
6. At **HOLD**, maintain the stop. WROOM accepts the result only after the force
   remains inside the configured stability band for three seconds.
7. At **CAPTURED / RELEASE**, release fully. WROOM requires a confirmed release
   before another capture can begin.
8. A Spine Test ends after one capture. An SAS Test requests a 90 degree rotation
   and repeats the PUSH/HOLD/RELEASE sequence until all four positions exist.

The arrow's resting mass is not treated as bending force:

    applied_force = loaded_combined_force - resting_arrow_mass

The result is captured from WROOM's instantaneous calibrated force path. VIEWE's
calm held display is not used as the stability detector.

## SAS output

For four captured equivalent spine values:

- **STIFF** is the lowest numerical value;
- **WEAK** is the highest numerical value and is the conservative worst case;
- **RANGE** is `WEAK - STIFF` in spine points;
- **SAS** is the same difference expressed in inches (`RANGE / 1000`).

If a marked spine was entered, the comparison uses the conservative weak/worst
value. A positive delta means the measured arrow is weaker than marked; a
negative delta means it is stiffer.

## Developer tuning

The automatic-capture thresholds live in `include/ArrowLab.conf`. They are
development controls, not ordinary user settings:

- hold time and force stability band;
- minimum force that starts a spine hold;
- release threshold and confirmation time;
- arrow-present threshold and arrow-mass stability time.

Changing these values requires rebuilding and uploading WROOM. The HMI receives
the resulting state and progress over protocol version 2.

## Icons and visual assets

The first home screen uses LVGL's built-in symbols as temporary icons. Final PNG
icons supplied for ArrowLab can be converted to compiled LVGL image assets by
the PlatformIO build and stored in VIEWE flash. An SD card is not required for
that normal path. Asset size will be reviewed before adding large backgrounds
or multiple theme variants.
