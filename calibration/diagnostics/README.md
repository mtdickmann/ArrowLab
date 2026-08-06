# Calibration diagnostic data

ArrowLab developer diagnostic CSV files are written here by tools/capture_creep.py.

Generated `creep_*.csv` datasets are ignored by Git by default and remain on the acquisition PC.

Raw datasets may still be committed when they form part of a documented calibration/creep investigation. Review and identify the selected evidence file, then add it deliberately with `git add -f calibration/diagnostics/<filename>`. Do not overwrite original acquisition files; derived or cleaned datasets should use a new filename.
