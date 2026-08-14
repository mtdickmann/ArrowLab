# Changelog

## v0.2.3 DEV

### Fixed
- Spine/SAS arrow mass now uses a simultaneous live Left + Right reading and a
  genuine stability window instead of independently published held snapshots
- An interrupted or partial plunge now resets the three-second hold and
  returns automatically to PUSH
- Removing the arrow before capture returns the live resting load to zero and
  restarts arrow detection without cancelling the test

### Added
- Explicit plunger-zero confirmation at the top of the arrow before the
  operator begins the fixed 12.7 mm plunge
- Active RESTART and CANCEL controls throughout the automatic test
- Separate configurable arrow-mass stability and resting-baseline tracking
  bands
- Protocol version 3 commands for plunger-zero confirmation and attempt restart

## v0.2.2 DEV

### Added
- Separate single-position Spine Test and four-position SAS Test
- WROOM-owned automatic READ / PUSH / HOLD / CAPTURE / RELEASE state machine
- Sequential empty Left/Right tare and automatic resting-arrow mass capture
- Stable-force capture with mandatory release between indexed measurements
- ATA-derived equivalent spine calculation and four-position stiff/weak/range
  reporting
- Optional marked-spine difference and percentage comparison without an
  unagreed PASS/FAIL judgement
- Temporary LVGL-symbol home cards ready for later compiled PNG assets
- Deterministic host test for the automatic spine state machine and formula

### Changed
- Advanced the VIEWE/WROOM checked binary protocol to version 2 for spine
  status and commands
- Moved Weigh's Left and Right tare controls into the result card
- Replaced the temporary list-style Home menu with a compact icon-card grid

## v0.2.1 DEV

### Added
- Shared development settings for operational weighing time, primary mass unit
  and displayed decimal places
- Compact secondary conversions between grams, grains and avoirdupois ounces
  beneath each calibrated mass reading
- Independent display precision for grams, grains and ounces
- Session-wide tap-to-cycle primary mass unit
- Dedicated Weigh screen with automatic Left, Right or combined cassette
  selection and deliberate per-cassette tare controls

### Changed
- Restored practical operational response with a configurable one-second
  default while retaining the separate fixed 30-second calibration interval
- Kept WROOM grams as the metrology value and confined unit conversion to the
  VIEWE presentation layer
- Kept combined weighing in a small tested aggregation module so later Spine
  and whole-arrow features can reuse accepted held results

## v0.2.0 DEV

### Added
- Production dual-processor architecture with WROOM-owned metrology
- Versioned, checksummed I2C command/status protocol
- Shared GT911/measurement-node I2C host on the VIEWE
- Persistent cross-screen measurement-node link fault reporting
- Host test for valid and corrupted protocol packets

### Changed
- Moved HX711 acquisition, tare, event-based measurement, calibration and
  calibration-factor persistence from VIEWE to the WROOM
- Retained the existing guided calibration UI as an HMI for WROOM operations
- Corrected VIEWE pin ownership: GPIO10-13 are RGB data and GPIO17 is RGB DE
- Reclassified the original VIEWE HX711 wiring as a superseded prototype

## v0.1.0 DEV

### Added
- Dual HX711 support
- LVGL interface
- Version framework
- GitHub repository
- Independent deliberate tare controls for Left and Right
- Per-channel reference-mass calibration and gram conversion
- Load-triggered calibration settling gate
- Tare/calibration status colour cues
- Structured LoadCellChannel measurement module
- Home / Settings navigation with calibration status
- Hidden developer-mode reveal and Diagnostics entry
- Calibration stabilization countdown and progress feedback
- Home load-cell health/fault status separate from calibration validity
- Enlarged long-press-only developer reveal touch target
- Persistent cross-screen load-cell fault strip
- Explicit 2-second developer reveal hold timing
- Hidden 30-minute creep diagnostic logger with side/mass workflow
- Numeric diagnostic mass keypad and automatic load-triggered timing
- USB serial diagnostic CSV protocol and host capture tool
- Repeatable creep diagnostic test procedure
- Mandatory per-channel zero baseline before loaded diagnostic runs
- Explicit diagnostic FINISH / CSV session completion
- Dedicated 10-second creep reference sample in every diagnostic run
- Local-by-default diagnostic CSV ignore convention
- Identified WROOM hardware-isolation CSV logger with controlled 30-minute
  sampling

### Fixed
- GT911 initialization issue
- Right HX711 DT/SCK wiring
- Raw unit label alignment
- Left HX711 DT GPIO documentation
- WROOM logger now waits for its COM port and creates the CSV before connection
