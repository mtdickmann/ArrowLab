#!/usr/bin/env python3
"""Summarise ArrowLab creep CSV files without modifying raw evidence."""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
import statistics


@dataclass(frozen=True)
class Sample:
    source: str
    run_id: int
    side: str
    run_type: str
    test_mass_g: float
    elapsed_s: float
    raw_count: int
    zeroed_count: int
    calculated_g: float
    calibration_factor: float


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Summarise ArrowLab legacy and protocol-v2 creep CSV files."
    )
    parser.add_argument("csv", nargs="+", type=Path, help="CSV files to analyse")
    return parser.parse_args()


def load_samples(path: Path) -> list[Sample]:
    samples: list[Sample] = []

    with path.open(newline="", encoding="utf-8-sig") as stream:
        for row in csv.DictReader(stream):
            run_type = row.get("run_type")
            mass = float(row["test_mass_g"])
            if not run_type:
                run_type = "ZERO" if mass == 0.0 else "LOAD"

            samples.append(
                Sample(
                    source=path.name,
                    run_id=int(row["run_id"]),
                    side=row["side"].upper(),
                    run_type=run_type.upper(),
                    test_mass_g=mass,
                    elapsed_s=int(row["elapsed_ms"]) / 1000.0,
                    raw_count=int(row["raw_count"]),
                    zeroed_count=int(row["zeroed_count"]),
                    calculated_g=float(row["calculated_g"]),
                    calibration_factor=float(row["calibration_factor"]),
                )
            )

    return samples


def median(values: list[float]) -> float:
    return float(statistics.median(values))


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]

    position = fraction * (len(ordered) - 1)
    lower = int(position)
    upper = min(lower + 1, len(ordered) - 1)
    weight = position - lower
    return ordered[lower] * (1.0 - weight) + ordered[upper] * weight


def theil_sen_slope(samples: list[Sample]) -> float:
    slopes: list[float] = []
    for left_index, left in enumerate(samples):
        for right in samples[left_index + 1 :]:
            elapsed = right.elapsed_s - left.elapsed_s
            if elapsed > 0.0:
                slopes.append(
                    (right.zeroed_count - left.zeroed_count) / elapsed
                )
    return median(slopes) if slopes else 0.0


def summarise(samples: list[Sample]) -> dict[str, float | int | str]:
    ordered = sorted(samples, key=lambda sample: sample.elapsed_s)

    # Sample zero is captured at the acquisition transition. ZERO emits it as
    # exactly zero immediately after tare, while LOAD may still be settling.
    # Preserve it in the raw evidence but exclude it from trend/noise metrics.
    analysis_samples = [sample for sample in ordered if sample.elapsed_s > 0.0]

    slope_counts_s = theil_sen_slope(analysis_samples)
    intercept = median(
        [
            sample.zeroed_count - slope_counts_s * sample.elapsed_s
            for sample in analysis_samples
        ]
    )
    residuals = [
        sample.zeroed_count
        - (intercept + slope_counts_s * sample.elapsed_s)
        for sample in analysis_samples
    ]
    residual_median = median(residuals)
    residual_mad = median(
        [abs(value - residual_median) for value in residuals]
    )
    robust_sigma_counts = residual_mad * 1.4826
    outlier_limit = max(robust_sigma_counts * 8.0, 500.0)
    gross_outliers = sum(
        abs(value - residual_median) > outlier_limit
        for value in residuals
    )

    first_window = [
        sample.zeroed_count
        for sample in analysis_samples
        if sample.elapsed_s <= 300.0
    ]
    last_window = [
        sample.zeroed_count
        for sample in analysis_samples
        if sample.elapsed_s >= 1500.0
    ]
    factor_values = [
        sample.calibration_factor
        for sample in ordered
        if sample.calibration_factor != 0.0
    ]
    factor = median(factor_values) if factor_values else 0.0
    drift_counts = median(last_window) - median(first_window)

    return {
        "source": ordered[0].source,
        "run_id": ordered[0].run_id,
        "side": ordered[0].side,
        "run_type": ordered[0].run_type,
        "mass_g": ordered[0].test_mass_g,
        "samples": len(ordered),
        "duration_s": ordered[-1].elapsed_s,
        "factor_counts_g": factor,
        "drift_counts": drift_counts,
        "drift_g": drift_counts / factor if factor else 0.0,
        "slope_counts_min": slope_counts_s * 60.0,
        "slope_g_min": slope_counts_s * 60.0 / factor if factor else 0.0,
        "robust_sigma_counts": robust_sigma_counts,
        "robust_sigma_g": robust_sigma_counts / factor if factor else 0.0,
        "residual_p90_counts": percentile(residuals, 0.95)
        - percentile(residuals, 0.05),
        "gross_outliers": gross_outliers,
        "zeroed_min": min(sample.zeroed_count for sample in analysis_samples),
        "zeroed_max": max(sample.zeroed_count for sample in analysis_samples),
    }


def main() -> int:
    args = parse_args()
    grouped: dict[tuple[str, int, str, str, float], list[Sample]] = {}

    for path in args.csv:
        for sample in load_samples(path):
            key = (
                sample.source,
                sample.run_id,
                sample.side,
                sample.run_type,
                sample.test_mass_g,
            )
            grouped.setdefault(key, []).append(sample)

    headings = (
        "source",
        "run",
        "side",
        "type",
        "mass_g",
        "n",
        "duration_s",
        "drift_counts",
        "drift_g",
        "slope_counts_min",
        "sigma_counts",
        "sigma_g",
        "p90_counts",
        "gross_outliers",
    )
    print(",".join(headings))

    for key in sorted(grouped):
        result = summarise(grouped[key])
        print(
            ",".join(
                [
                    str(result["source"]),
                    str(result["run_id"]),
                    str(result["side"]),
                    str(result["run_type"]),
                    f"{result['mass_g']:.3f}",
                    str(result["samples"]),
                    f"{result['duration_s']:.1f}",
                    f"{result['drift_counts']:.1f}",
                    f"{result['drift_g']:.4f}",
                    f"{result['slope_counts_min']:.3f}",
                    f"{result['robust_sigma_counts']:.2f}",
                    f"{result['robust_sigma_g']:.4f}",
                    f"{result['residual_p90_counts']:.1f}",
                    str(result["gross_outliers"]),
                ]
            )
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
