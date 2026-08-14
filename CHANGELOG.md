# Changelog

## v0.2.1 DEV

### Added
- Shared development settings for operational weighing time, primary mass unit
  and displayed decimal places
- Compact secondary conversions between grams, grains and avoirdupois ounces
  beneath each calibrated mass reading

### Changed
- Restored practical operational response with a configurable one-second
  default while retaining the separate fixed 30-second calibration interval
- Kept WROOM grams as the metrology value and confined unit conversion to the
  VIEWE presentation layer

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
