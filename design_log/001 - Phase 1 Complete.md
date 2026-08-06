# Phase 1 Complete

## Date
2026-08-04

## Hardware
- ESP32-S3 display operational
- Touch operational
- Dual HX711 modules operational
- Two 2 kg load cells connected

## Firmware
- LVGL UI operational
- Dual raw sensor acquisition
- Live updates
- Status reporting

## GPIO assignments

Left HX711
DT  -> GPIO17
SCK -> GPIO10

Right HX711
DT  -> GPIO11
SCK -> GPIO12

## Lessons Learned

- Terminal blocks on the display were invaluable.
- Always perform continuity checks after crimping JST connectors.
- DT/SCK swapped produces a "waiting for HX711" condition.
- Vendor hardware proved reliable after wiring issues were resolved.

## Next Phase

- Zero each sensor
- Calibration
- Convert to grams
- Combined load
- Begin spine calculations

## Milestone M001 – Project Established

**Date:** 2026-08-04

- Independent Git repository created
- GitHub repository linked
- Dual HX711 sensors operational
- Phase 1 completed