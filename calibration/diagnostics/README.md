# Calibration diagnostic data

ArrowLab developer diagnostic CSV files are written here by tools/capture_creep.py.
Automatic names include `LHS` or `RHS`, for example
`creep_RHS_20260807_100313.csv`, so independent channels cannot be mistaken in
the evidence folder.

Generated `creep_*.csv` datasets are ignored by Git by default and remain on the acquisition PC.

Raw datasets may still be committed when they form part of a documented calibration/creep investigation. Review and identify the selected evidence file, then add it deliberately with `git add -f calibration/diagnostics/<filename>`. Do not overwrite original acquisition files; derived or cleaned datasets should use a new filename.

Protocol-v2 CSV files include boot ID, run ID and sample index. Those fields let the PC logger replay and de-duplicate a complete ESP32 RAM buffer after a serial interruption. A baseline is not marked complete on the instrument until the PC has flushed and acknowledged every expected row.
